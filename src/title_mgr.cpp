#include "title_mgr.hpp"
#include <cassert>
#include <fmt/format.h>
#include <zip.h>
#include "utils.hpp"

std::filesystem::path TitleId::getMetaPath(const std::string& name) const
{
    auto storage =
        (title_type == TitleType_MLC ? "storage_mlc" : "storage_usb");
    auto title_id_hi = title_id.substr(0, 8);
    auto title_id_lo = title_id.substr(8, 8);

    return fmt::format("/{}/usr/title/{}/{}/meta/{}", storage, title_id_hi,
                       title_id_lo, name);
}

TitleMgr::TitleMgr() :
    m_title_cache(),
    m_titles(),
    m_ftp(
        [this](const std::string& msg)
        {
            m_error = msg;
            fmt::print(stderr, "{}\n", msg);
        })
{
    m_ftp.SetTimeout(10);
}
TitleMgr::~TitleMgr()
{
    cleanup();
}

auto TitleMgr::downloadMetaFile(const TitleId& title_id,
                                const std::string& name)
    -> Result<std::vector<char>, WiiuConnexionError, MetaDirMissingFileError>
{
    auto path = title_id.getMetaPath(name).string();

    std::lock_guard<std::mutex> lock(m_file_cache_lock);
    if (m_file_cache.contains(path))
        return m_file_cache.at(path);

    std::vector<char> buff;
    CURLcode curl_ret;
    fmt::print("FTP: Downloading \"{}\"\n", path);
    if (!m_ftp.DownloadFile(path, buff, curl_ret))
    {
        if (curl_ret == CURLE_REMOTE_FILE_NOT_FOUND)
            return Unexpected(MetaDirMissingFileError{ name });
        return Unexpected(WiiuConnexionError{ m_error });
    }

    m_file_cache.emplace(path, buff);

    return buff;
}

auto TitleMgr::uploadMetaFile(const TitleId& title_id, const std::string& name,
                              const void* data, size_t data_size)
    -> Error<WiiuConnexionError>
{

    struct Ctx
    {
        const u8* data;
        size_t size;
        size_t pos;
    } cur_ctx = {
        .data = reinterpret_cast<const u8*>(data),
        .size = data_size,
        .pos = 0,
    };

    auto path = title_id.getMetaPath(name).string();
    auto callback = [](void* ptr, size_t size, size_t nmemb,
                       void* usr) -> size_t
    {
        auto* ctx = reinterpret_cast<Ctx*>(usr);
        size_t actual_read = std::min(ctx->size - ctx->pos, nmemb * size);
        std::memcpy(ptr, ctx->data + ctx->pos, actual_read);
        ctx->pos += actual_read;
        return actual_read;
    };

    fmt::print("FTP: Uploading \"{}\"\n", path);
    if (!m_ftp.UploadFile(callback, &cur_ctx, path))
        return Unexpected(WiiuConnexionError{ m_error });

    std::lock_guard<std::mutex> lock(m_file_cache_lock);
    std::vector<char> buf(data_size);
    std::memcpy(buf.data(), data, data_size);
    m_file_cache.emplace(path, std::move(buf));

    return {};
}

auto TitleMgr::restoreBackup(const std::filesystem::path& zip_path,
                             ProgressReport* reporter)
    -> Error<ZipError, WiiuConnexionError>
{
    int err_code = 0;
    zip* z = zip_open(zip_path.string().c_str(), ZIP_RDONLY, &err_code);
    if (z == nullptr)
    {
        zip_error_t error;
        zip_error_init_with_code(&error, err_code);
        fmt::print(stderr, "zip_open: failed to open \"{}\" : {}\n",
                   zip_path.string(), zip_error_strerror(&error));
        zip_error_fini(&error);
        return Unexpected(ZipError{ zip_error_strerror(&error) });
    }

    size_t progress_i = 0;

    auto process_file =
        [this, z, reporter,
         &progress_i](const TitleId& title_id,
                      const std::string& name) -> Error<WiiuConnexionError>
    {
        auto path = fmt::format(
            "{}_{}/{}", title_id.title_type == TitleType_MLC ? "mlc" : "usb",
            title_id.title_id, name);

        if (reporter)
            reporter->reportStep(
                progress_i++,
                fmt::format("{}: {} : {}",
                            title_id.title_type == TitleType_MLC ? "MLC"
                                                                 : "USB",
                            title_id.title_id, name));

        zip_stat_t stat;
        if (zip_stat(z, path.c_str(), 0, &stat) == -1)
        {
            fmt::print(stderr, "Backup does not contain entry for {}\n", path);
            return {};
        }

        auto f = zip_fopen(z, path.c_str(), 0);

        assert((stat.valid & ZIP_STAT_SIZE) != 0);

        zip_uint64_t written = 0;

        std::vector<u8> buf(stat.size);

        while (written < stat.size)
        {
            auto cur = zip_fread(f, buf.data() + written, stat.size - written);
            assert(cur > 0);
            written += (zip_uint64_t)cur;
        }

        zip_fclose(f);

        return uploadMetaFile(title_id, name,
                              reinterpret_cast<const void*>(buf.data()),
                              buf.size());
    };

    if (reporter)
        reporter->setCount(m_titles.size() * 6);

    for (auto& title_id : m_titles)
    {
        PROPAGATE_VOID(process_file(title_id, "bootDrcTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootTvTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootLogoTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "iconTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootSound.btsnd"));
        PROPAGATE_VOID(process_file(title_id, "meta.xml"));
    }

    zip_close(z);

    std::lock_guard<std::mutex> lock(m_title_cache_lock);
    m_title_cache.clear();
    return {};
}

bool TitleMgr::isTitleDirty(const TitleId& title_id)
{
    std::lock_guard<std::mutex> lock(m_title_cache_lock);
    if (!m_title_cache.contains(title_id))
        return false;
    return m_title_cache.at(title_id).isDirty();
}

std::vector<TitleId> TitleMgr::getDirtyTitles()
{
    std::lock_guard<std::mutex> lock(m_title_cache_lock);
    std::vector<TitleId> ret;
    for (auto& [title_id, title] : m_title_cache)
    {
        if (title.isDirty())
            ret.push_back(title_id);
    }
    return ret;
}

auto TitleMgr::syncTitles(ProgressReport* reporter) -> Error<WiiuConnexionError>
{
    auto titles = getDirtyTitles();

    if (reporter)
        reporter->setCount(titles.size() * 5);

    size_t i = 0;

    for (auto& title_id : titles)
    {
        m_title_cache_lock.lock();
        auto& title = m_title_cache.at(title_id);
        m_title_cache_lock.unlock();

#define UPLOAD_FILE(name, expr)                                                \
    {                                                                          \
        if (reporter)                                                          \
            reporter->reportStep(                                              \
                i++, fmt::format("{}: {} : Uploading {}",                      \
                                 title_id.title_type == TitleType_MLC ? "MLC"  \
                                                                      : "USB", \
                                 title_id.title_id, name));                    \
        auto buf = expr;                                                       \
        auto ret = uploadMetaFile(title_id, name,                              \
                                  reinterpret_cast<const void*>(buf.data()),   \
                                  buf.size());                                 \
        if (!ret)                                                              \
            return ret;                                                        \
    }

        UPLOAD_FILE("bootSound.btsnd", title.sound().toBtsnd());
        UPLOAD_FILE("bootDrcTex.tga", title.drcTex().toWiiU(true));
        UPLOAD_FILE("bootTvTex.tga", title.tvTex().toWiiU(true));
        UPLOAD_FILE("bootLogoTex.tga", title.logoTex().toWiiU(false));
        UPLOAD_FILE("iconTex.tga", title.iconTex().toWiiU(false));

#undef UPLOAD_FILE

        title.setDirty(false);
    }
    return {};
}

auto TitleMgr::backupTitles(const std::filesystem::path& zip_path,
                            ProgressReport* reporter)
    -> Error<WiiuConnexionError>
{
    int err_code = 0;

    if (std::filesystem::exists(zip_path))
        std::filesystem::remove(zip_path);

    zip* z = zip_open(zip_path.string().c_str(), ZIP_CREATE, &err_code);

    size_t progress_i = 0;

    auto process_file =
        [this, &z, &progress_i,
         reporter](const TitleId& title_id,
                   const std::string& name) -> Error<WiiuConnexionError>
    {
        if (reporter)
            reporter->reportStep(
                progress_i++,
                fmt::format("{}: {} : Downloading {}",
                            title_id.title_type == TitleType_MLC ? "MLC"
                                                                 : "USB",
                            title_id.title_id, name));

        auto file = downloadMetaFile(title_id, name);
        if (!file)
        {
            PROPAGATE_VOID(std::visit(
                overloaded{
                    [](const WiiuConnexionError& x) -> Error<WiiuConnexionError>
                    { return Unexpected(x); },
                    [](const MetaDirMissingFileError&)
                        -> Error<WiiuConnexionError> { return {}; } },
                file.error()));
        }

        // buffer needs to outlive this scope, let libzip free it
        auto buf = std::malloc(file->size());
        std::memcpy(buf, file->data(), file->size());

        zip_error_t zip_err;
        auto src = zip_source_buffer_create(buf, file->size(), true, &zip_err);
        auto path = fmt::format(
            "{}_{}/{}", title_id.title_type == TitleType_MLC ? "mlc" : "usb",
            title_id.title_id, name);
        zip_file_add(z, path.c_str(), src, ZIP_FL_ENC_UTF_8);
        return {};
    };

    if (reporter)
        reporter->setCount(m_titles.size() * 7);

    for (auto& title_id : m_titles)
    {
        PROPAGATE_VOID(process_file(title_id, "bootDrcTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootTvTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootLogoTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "iconTex.tga"));
        PROPAGATE_VOID(process_file(title_id, "bootSound.btsnd"));
        PROPAGATE_VOID(process_file(title_id, "meta.xml"));

        if (reporter)
            reporter->reportStep(
                progress_i++,
                fmt::format("{}: {} : Writing ZIP",
                            title_id.title_type == TitleType_MLC ? "MLC"
                                                                 : "USB",
                            title_id.title_id));

        // flush zip
        zip_close(z);
        z = zip_open(zip_path.string().c_str(), 0, &err_code);
    }

    zip_close(z);
    return {};
}

static std::vector<std::string> splitLs(std::string s)
{
    size_t pos1 = 0;
    size_t pos2 = 0;

    // remove \r
    while ((pos2 = s.find("\r", pos1)) != std::string::npos)
    {
        s.erase(pos2, 1);
        pos1 = pos2;
    }

    pos1 = 0;
    pos2 = 0;

    std::vector<std::string> ret;
    while ((pos2 = s.find("\n", pos1)) != std::string::npos)
    {
        ret.push_back(s.substr(pos1, pos2 - pos1));
        pos1 = pos2 + 1;
    }

    return ret;
}

auto TitleMgr::ls(const std::filesystem::path& dir)
    -> Result<std::vector<std::string>, WiiuConnexionError>
{
    std::string ret;
    CURLcode curl_code;
    fmt::print("Listing dir \"{}\"\n", dir.string());
    if (!m_ftp.List(dir.string(), ret, curl_code))
    {
        return Unexpected(WiiuConnexionError{ m_error });
    }
    fmt::print("FTP: received:\n{}\n", ret);
    return splitLs(ret);
}

auto TitleMgr::connect(const std::string& ip) -> Error<WiiuConnexionError>
{
    {
        std::lock_guard<std::mutex> lock(m_title_cache_lock);
        m_title_cache.clear();
        std::lock_guard<std::mutex> lock2(m_file_cache_lock);
        m_file_cache.clear();
    }
    m_titles.clear();
    m_error.clear();

    if (!m_ftp.InitSession(ip.c_str(), 21, "anonymous", "",
                           embeddedmz::CFTPClient::FTP_PROTOCOL::FTP,
                           embeddedmz::CFTPClient::ENABLE_LOG))
    {
        return Unexpected(WiiuConnexionError{ m_error });
    }

    // fetch titles
    auto add_titles = [this](const std::string& path,
                             TitleType title_type) -> Error<WiiuConnexionError>
    {
        auto title_dir = PROPAGATE(ls(path));

        for (auto id_high : title_dir)
        {
            // Skip DLC titles
            if (id_high == "0005000c")
                continue;

            auto high_dir = PROPAGATE(ls(fmt::format("{}{}/", path, id_high)));
            for (auto id_low : high_dir)
            {
                const std::string title_id = id_high + id_low;
                m_titles.push_back({
                    .title_id = std::move(title_id),
                    .title_type = title_type,
                });
            }
        }
        return {};
    };

    PROPAGATE_VOID(add_titles("/storage_mlc/usr/title/", TitleType_MLC));
    PROPAGATE_VOID(add_titles("/storage_usb/usr/title/", TitleType_USB));

    return {};
}

auto TitleMgr::getTitle(const TitleId& title_id, ProgressReport* reporter)
    -> Result<TitleMeta*, WiiuConnexionError, ImageError, SoundError,
              MetaDirMissingFileError>
{
    {
        std::lock_guard<std::mutex> lock(m_title_cache_lock);
        if (m_title_cache.contains(title_id))
            return &m_title_cache.at(title_id);
    }

    size_t i = 0;
    if (reporter)
        reporter->setCount(5);

#define DOWNLOAD_FILE(name, parse_func)                                        \
    ({                                                                         \
        if (reporter)                                                          \
            reporter->reportStep(i++, name);                                   \
        auto buff = PROPAGATE(downloadMetaFile(title_id, name));               \
        PROPAGATE(parse_func(reinterpret_cast<const void*>(buff.data()),       \
                             buff.size()));                                    \
    })

    Image drc_tex = DOWNLOAD_FILE("bootDrcTex.tga", Image::fromWiiU);
    Image tv_tex = DOWNLOAD_FILE("bootTvTex.tga", Image::fromWiiU);
    Image logo_tex = DOWNLOAD_FILE("bootLogoTex.tga", Image::fromWiiU);
    Image icon_tex = DOWNLOAD_FILE("iconTex.tga", Image::fromWiiU);
    Sound btsnd = DOWNLOAD_FILE("bootSound.btsnd", Sound::fromBtsnd);
#undef DOWNLOAD_FILE

    auto meta =
        TitleMeta(std::move(drc_tex), std::move(tv_tex), std::move(logo_tex),
                  std::move(icon_tex), std::move(btsnd));

    {
        std::lock_guard<std::mutex> lock(m_title_cache_lock);
        m_title_cache.emplace(title_id, std::move(meta));
        return &m_title_cache.at(title_id);
    }
}

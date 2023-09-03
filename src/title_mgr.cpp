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
    m_cache(),
    m_titles(),
    m_ftp(
        [this](const std::string& msg)
        {
            m_error = msg;
            m_state = State_ConnectionFailed;
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
                                const std::string& name) const
    -> Expected<std::vector<char>,
                std::variant<WiiuConnexionError, MetaDirMissingFileError>>
{
    auto path = title_id.getMetaPath(name);
    std::vector<char> buff;
    CURLcode curl_ret;
    if (!m_ftp.DownloadFile(path.string(), buff, curl_ret))
    {
        if (curl_ret == CURLE_REMOTE_FILE_NOT_FOUND)
            return Unexpected(MetaDirMissingFileError{ name });
        return Unexpected(WiiuConnexionError{ m_error });
    }
    return buff;
}

auto TitleMgr::uploadMetaFile(const TitleId& title_id, const std::string& name,
                              const void* data, size_t data_size) const
    -> Expected<void, WiiuConnexionError>
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

    auto path = title_id.getMetaPath(name);
    auto callback = [](void* ptr, size_t size, size_t nmemb,
                       void* usr) -> size_t
    {
        auto* ctx = reinterpret_cast<Ctx*>(usr);
        size_t actual_read = std::min(ctx->size - ctx->pos, nmemb * size);
        std::memcpy(ptr, ctx->data + ctx->pos, actual_read);
        ctx->pos += actual_read;
        return actual_read;
    };

    fmt::print("FTP: Uploading \"{}\"\n", path.string());
    if (!m_ftp.UploadFile(callback, &cur_ctx, path.string()))
        return Unexpected(WiiuConnexionError{ m_error });

    return {};
}

void TitleMgr::restoreBackup(const std::filesystem::path& zip_path)
{
    m_cache.clear();

    int err = 0;
    zip* z = zip_open(zip_path.string().c_str(), ZIP_RDONLY, &err);

    auto process_file =
        [this, z](const TitleId& title_id, const std::string& name)
    {
        auto path = fmt::format(
            "{}_{}/{}", title_id.title_type == TitleType_MLC ? "mlc" : "usb",
            title_id.title_id, name);

        zip_stat_t stat;
        if (zip_stat(z, path.c_str(), 0, &stat) == -1)
        {
            fmt::print("Backup does not contain entry for {}\n", path);
            return;
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

        auto ret = uploadMetaFile(title_id, name,
                                  reinterpret_cast<const void*>(buf.data()),
                                  buf.size());
    };

    for (auto& title_id : m_titles)
    {
        process_file(title_id, "bootDrcTex.tga");
        process_file(title_id, "bootTvTex.tga");
        process_file(title_id, "bootLogoTex.tga");
        process_file(title_id, "iconTex.tga");
        process_file(title_id, "bootSound.btsnd");
    }

    zip_close(z);
}

bool TitleMgr::isTitleDirty(const TitleId& title_id) const
{
    if (!m_cache.contains(title_id))
        return false;
    return m_cache.at(title_id).isDirty();
}

std::vector<TitleId> TitleMgr::getDirtyTitles() const
{
    std::vector<TitleId> ret;
    for (auto& [title_id, title] : m_cache)
    {
        if (title.isDirty())
            ret.push_back(title_id);
    }
    return ret;
}

void TitleMgr::syncTitles()
{
    auto titles = getDirtyTitles();
    for (auto& title_id : titles)
    {
        auto& title = m_cache.at(title_id);

        // TODO: error handling
#define UPLOAD_FILE(name, expr)                                                \
    {                                                                          \
        auto buf = expr;                                                       \
        auto ret = uploadMetaFile(title_id, name,                              \
                                  reinterpret_cast<const void*>(buf.data()),   \
                                  buf.size());                                 \
        if (!ret)                                                              \
            std::abort();                                                      \
    }

        UPLOAD_FILE("bootSound.btsnd", title.sound().toBtsnd());
        UPLOAD_FILE("bootDrcTex.tga", title.drcTex().toWiiU(true));
        UPLOAD_FILE("bootTvTex.tga", title.tvTex().toWiiU(true));
        UPLOAD_FILE("bootLogoTex.tga", title.logoTex().toWiiU(false));
        UPLOAD_FILE("iconTex.tga", title.iconTex().toWiiU(false));

#undef UPLOAD_FILE

        title.setDirty(false);
    }
}

void TitleMgr::backupTitles(const std::filesystem::path& zip_path) const
{
    int err = 0;

    if (std::filesystem::exists(zip_path))
        std::filesystem::remove(zip_path);

    zip* z = zip_open(zip_path.string().c_str(), ZIP_CREATE, &err);

    auto process_file =
        [this, z](const TitleId& title_id, const std::string& name)
    {
        auto file = downloadMetaFile(title_id, name);
        if (!file)
            return;

        // buffer needs to outlive this scope, let libzip free it
        auto buf = std::malloc(file->size());
        std::memcpy(buf, file->data(), file->size());

        zip_error_t zip_err;
        auto src = zip_source_buffer_create(buf, file->size(), true, &zip_err);
        auto path = fmt::format(
            "{}_{}/{}", title_id.title_type == TitleType_MLC ? "mlc" : "usb",
            title_id.title_id, name);
        zip_file_add(z, path.c_str(), src, ZIP_FL_ENC_UTF_8);
    };

    for (auto& title_id : m_titles)
    {
        process_file(title_id, "bootDrcTex.tga");
        process_file(title_id, "bootTvTex.tga");
        process_file(title_id, "bootLogoTex.tga");
        process_file(title_id, "iconTex.tga");
        process_file(title_id, "bootSound.btsnd");
    }

    zip_close(z);
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

std::vector<std::string> TitleMgr::ls(const std::filesystem::path& dir)
{
    std::string ret;
    CURLcode curl_code;
    fmt::print("Listing dir \"{}\"\n", dir.string());
    if (!m_ftp.List(dir.string(), ret, curl_code))
    {
        if (curl_code != CURLE_OK)
            m_error = curl_easy_strerror(curl_code);
        return {};
    }
    fmt::print("FTP: received:\n{}\n", ret);
    return splitLs(ret);
}

Expected<void, WiiuConnexionError> TitleMgr::connect(const std::string& ip)
{
    m_state = State_Connected;
    m_cache.clear();
    m_titles.clear();
    m_error.clear();

    if (!m_ftp.InitSession(ip.c_str(), 21, "anonymous", "",
                           embeddedmz::CFTPClient::FTP_PROTOCOL::FTP,
                           embeddedmz::CFTPClient::ENABLE_LOG))
    {
        return Unexpected(WiiuConnexionError{ m_error });
    }

    // fetch titles
    auto add_titles = [this](const std::string& path, TitleType title_type)
    {
        auto title_dir = ls(path);

        for (auto id_high : title_dir)
        {
            // Skip DLC titles
            if (id_high == "0005000c")
                continue;

            auto high_dir = ls(fmt::format("{}{}/", path, id_high));
            for (auto id_low : high_dir)
            {
                const std::string title_id = id_high + id_low;
                m_titles.push_back({
                    .title_id = std::move(title_id),
                    .title_type = title_type,
                });
            }
        }
    };

    add_titles("/storage_mlc/usr/title/", TitleType_MLC);
    add_titles("/storage_usb/usr/title/", TitleType_USB);

    return {};
}

auto TitleMgr::getTitle(const TitleId& title_id)
    -> Expected<TitleMeta*, std::variant<WiiuConnexionError, ImageError,
                                         SoundError, MetaDirMissingFileError>>
{
    assert(m_state == State_Connected);

    if (m_cache.contains(title_id))
        return &m_cache.at(title_id);

#define DOWNLOAD_FILE(name, parse_func)                                        \
    ({                                                                         \
        auto path = title_id.getMetaPath(name);                                \
        std::vector<char> buff;                                                \
        CURLcode curl_ret;                                                     \
        fmt::print("FTP: Downloading \"{}\"\n", path.string());                \
        if (!m_ftp.DownloadFile(path.string(), buff, curl_ret))                \
        {                                                                      \
            if (curl_ret == CURLE_REMOTE_FILE_NOT_FOUND)                       \
                return Unexpected(MetaDirMissingFileError{ name });            \
            return Unexpected(WiiuConnexionError{ m_error });                  \
        }                                                                      \
        auto ret = parse_func(reinterpret_cast<const void*>(buff.data()),      \
                              buff.size());                                    \
        if (!ret)                                                              \
            return Unexpected(ret.error());                                    \
        std::move(ret.value());                                                \
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

    m_cache.emplace(title_id, std::move(meta));
    return &m_cache.at(title_id);
}

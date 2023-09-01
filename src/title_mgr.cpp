#include "title_mgr.hpp"
#include <cassert>
#include <fmt/format.h>
#include "utils.hpp"

TitleMgr::TitleMgr() :
    m_cache(),
    m_titles(),
    m_ftp(
        [this](const std::string& msg)
        {
            m_error = msg;
            m_state = State_ConnectionFailed;
            fmt::print("{}", msg);
        })
{
    m_ftp.SetTimeout(10);
}
TitleMgr::~TitleMgr()
{
    cleanup();
}

static std::vector<std::string> splitLs(const std::string& s)
{
    size_t pos1 = 0;
    size_t pos2 = 0;

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
    if (!m_ftp.List(dir.string(), ret, curl_code))
    {
        if (curl_code != CURLE_OK)
            m_error = curl_easy_strerror(curl_code);
        return {};
    }
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
    auto add_titles =
        [this](const std::filesystem::path& path, TitleType title_type)
    {
        auto title_dir = ls(path);

        for (auto id_high : title_dir)
        {
            auto high_dir = ls(path / id_high / "");
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

    auto storage =
        (title_id.title_type == TitleType_MLC ? "storage_mlc" : "storage_usb");
    auto title_id_hi = title_id.title_id.substr(0, 8);
    auto title_id_lo = title_id.title_id.substr(8, 8);
    auto root = std::filesystem::path("/");

    auto path =
        root / storage / "usr" / "title" / title_id_hi / title_id_lo / "meta";

#define DOWNLOAD_FILE(name, parse_func)                                        \
    ({                                                                         \
        std::vector<char> buff;                                                \
        CURLcode curl_ret;                                                     \
        if (!m_ftp.DownloadFile((path / name).string(), buff, curl_ret))       \
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

#pragma once

#include <string>
#include <unordered_map>
#include "title_meta.hpp"
#include "utils.hpp"
#define LINUX
#include <FTP/FTPClient.h>

enum TitleType
{
    TitleType_USB,
    TitleType_MLC,
};

struct TitleId
{
    std::string title_id;
    TitleType title_type;

    auto operator<=>(const TitleId& rhs) const = default;

    std::filesystem::path getMetaPath() const
    {
        auto storage =
            (title_type == TitleType_MLC ? "storage_mlc" : "storage_usb");
        auto title_id_hi = title_id.substr(0, 8);
        auto title_id_lo = title_id.substr(8, 8);
        auto root = std::filesystem::path("/");

        return root / storage / "usr" / "title" / title_id_hi / title_id_lo /
               "meta";
    }
};

template<>
struct std::hash<TitleId>
{
    std::size_t operator()(const TitleId& x) const noexcept
    {
        std::size_t h1 = std::hash<std::string>{}(x.title_id);
        std::size_t h2 = std::hash<int>{}(x.title_type);
        return h1 ^ (h2 << 1);
    }
};

struct WiiuConnexionError
{
    std::string error;
};

class TitleMgr
{
public:
    enum State
    {
        State_Disconnected,
        State_Connecting,
        State_Connected,
        State_ConnectionFailed,
    };

public:
    TitleMgr();
    ~TitleMgr();

    Expected<void, WiiuConnexionError> connect(const std::string& ip);

    void cleanup()
    {
        m_ftp.CleanupSession();
        m_error.clear();
        m_state = State_Disconnected;
        m_titles.clear();
        m_cache.clear();
    }

    bool connected() const { return m_state == State_Connected; }
    bool connecting() const { return m_state == State_Connecting; }
    bool disconnected() const { return m_state == State_Disconnected; }
    bool connectionFailed() const { return m_state == State_ConnectionFailed; }
    State state() const { return m_state; }
    const std::string& errorMsg() const { return m_error; }
    const std::vector<TitleId>& getTitles() { return m_titles; }

    auto getTitle(const TitleId& title_id)
        -> Expected<TitleMeta*,
                    std::variant<WiiuConnexionError, ImageError, SoundError,
                                 MetaDirMissingFileError>>;

    void backupTitles(const std::filesystem::path& zip) const;
    void restoreBackup(const std::filesystem::path& zip);

private:
    std::vector<std::string> ls(const std::filesystem::path& dir);
    auto downloadMetaFile(const TitleId& title_id,
                          const std::string& name) const
        -> Expected<std::vector<char>,
                    std::variant<WiiuConnexionError, MetaDirMissingFileError>>;

    auto uploadMetaFile(const TitleId& titlte_id, const std::string& name,
                        const void* data, size_t size) const
        -> Expected<void, WiiuConnexionError>;

private:
    std::unordered_map<TitleId, TitleMeta> m_cache;
    std::vector<TitleId> m_titles;
    State m_state = State_Disconnected;
    std::string m_error;
    embeddedmz::CFTPClient m_ftp;
};

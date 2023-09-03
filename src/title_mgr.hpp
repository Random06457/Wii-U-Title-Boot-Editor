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

    std::filesystem::path getMetaPath(const std::string& name = "") const;
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

struct ZipError
{
    std::string msg;
};

class TitleMgr
{
public:
    TitleMgr();
    ~TitleMgr();

    auto connect(const std::string& ip) -> Error<WiiuConnexionError>;

    void cleanup()
    {
        m_ftp.CleanupSession();
        m_error.clear();
        m_titles.clear();
        m_cache.clear();
    }

    const std::vector<TitleId>& getTitles() { return m_titles; }

    bool isTitleDirty(const TitleId& title_id) const;
    std::vector<TitleId> getDirtyTitles() const;
    auto syncTitles() -> Error<WiiuConnexionError>;
    auto getTitle(const TitleId& title_id)
        -> Result<TitleMeta*, WiiuConnexionError, ImageError, SoundError,
                  MetaDirMissingFileError>;

    auto backupTitles(const std::filesystem::path& zip) const
        -> Error<WiiuConnexionError>;
    auto restoreBackup(const std::filesystem::path& zip)
        -> Error<ZipError, WiiuConnexionError>;

private:
    auto ls(const std::filesystem::path& dir)
        -> Result<std::vector<std::string>, WiiuConnexionError>;
    auto downloadMetaFile(const TitleId& title_id,
                          const std::string& name) const
        -> Result<std::vector<char>, WiiuConnexionError,
                  MetaDirMissingFileError>;

    auto uploadMetaFile(const TitleId& titlte_id, const std::string& name,
                        const void* data, size_t size) const
        -> Error<WiiuConnexionError>;

private:
    std::unordered_map<TitleId, TitleMeta> m_cache;
    std::vector<TitleId> m_titles;
    std::string m_error;
    embeddedmz::CFTPClient m_ftp;
};

#pragma once

#include <string>
#include <unordered_map>
#include "title_meta.hpp"

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
    void connect(const std::string& ip);
    void clearError()
    {
        m_error.clear();
        m_state = State_Disconnected;
    }

    bool connected() const { return m_state == State_Connected; }
    bool connecting() const { return m_state == State_Connecting; }
    bool disconnected() const { return m_state == State_Disconnected; }
    bool connectionFailed() const { return m_state == State_ConnectionFailed; }
    State state() const { return m_state; }
    const std::string& errorMsg() const { return m_error; }

    const std::vector<TitleId>& getTitles();
    auto getTitle(const TitleId& title_id)
        -> std::expected<TitleMeta*, std::variant<ImageError, SoundError,
                                                  MetaDirMissingFileError>>;

private:
    std::unordered_map<TitleId, TitleMeta> m_cache;
    std::vector<TitleId> m_titles;
    bool m_is_title_list_fetched = false;
    State m_state = State_Disconnected;
    std::string m_error;
};

#include "title_mgr.hpp"
#include <cassert>

void TitleMgr::connect(const std::string&)
{
    m_state = State_Connected;
    m_cache.clear();
    m_titles.clear();
    m_is_title_list_fetched = false;
    m_error.clear();
}

const std::vector<TitleId>& TitleMgr::getTitles()
{
    assert(m_state == State_Connected);

    if (m_is_title_list_fetched)
        return m_titles;

    m_titles.clear();

    auto add_titles =
        [this](const std::filesystem::path& path, TitleType title_type)
    {
        auto title_dir = std::filesystem::directory_iterator(path);

        for (auto id_high : title_dir)
        {
            auto high_dir = std::filesystem::directory_iterator(id_high);
            for (auto id_low : high_dir)
            {
                const std::string title_id =
                    id_high.path().filename().string() +
                    id_low.path().filename().string();
                m_titles.push_back({
                    .title_id = std::move(title_id),
                    .title_type = title_type,
                });
            }
        }
    };

    add_titles("wiiu/storage_mlc/usr/title", TitleType_MLC);
    add_titles("wiiu/storage_usb/usr/title", TitleType_USB);

    return m_titles;
}

auto TitleMgr::getTitle(const TitleId& title_id) -> std::expected<
    TitleMeta*, std::variant<ImageError, SoundError, MetaDirMissingFileError>>
{
    assert(m_state == State_Connected);

    if (m_cache.contains(title_id))
        return &m_cache.at(title_id);

    auto meta = TitleMeta::fromDir(
        std::string("wiiu/storage_") +
        (title_id.title_type == TitleType_MLC ? "mlc" : "usb") + "/usr/title/" +
        title_id.title_id.substr(0, 8) + "/" + title_id.title_id.substr(8, 8) +
        "/meta");
    if (!meta)
        return std::unexpected(meta.error());

    m_cache.emplace(title_id, std::move(meta.value()));
    return &m_cache.at(title_id);
}

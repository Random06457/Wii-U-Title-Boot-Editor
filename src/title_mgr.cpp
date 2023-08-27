#include "title_mgr.hpp"

const std::vector<TitleId>& TitleMgr::getTitles()
{
    if (m_is_title_list_fetched)
        return m_titles;

    auto cache_dir = std::filesystem::directory_iterator("cache");
    m_titles.clear();

    for (auto dir : cache_dir)
    {
        m_titles.push_back({
            .title_id = dir.path().filename().string(),
            .title_type = TitleType_MLC,
        });
    }

    return m_titles;
}

auto TitleMgr::getTitle(const TitleId& title_id) -> std::expected<
    TitleMeta*, std::variant<ImageError, SoundError, MetaDirMissingFileError>>
{
    if (m_cache.contains(title_id))
        return &m_cache.at(title_id);

    auto meta = TitleMeta::fromDir("cache/" + title_id.title_id);
    if (!meta)
        return std::unexpected(meta.error());

    m_cache.emplace(title_id, std::move(meta.value()));
    return &m_cache.at(title_id);
}

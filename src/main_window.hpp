#pragma once
#include <memory>
#include <vector>
#include "gl_image.hpp"
#include "sound_player.hpp"
#include "title_meta.hpp"

class MainWindow
{
private:
    struct Selection
    {
        Selection(TitleMeta&& meta);

        TitleMeta meta;
        GlImage drc_tex;
        GlImage tv_tex;
        GlImage logo_tex;
        GlImage icon_tex;
        SoundPlayer player;

        int target_idx;
        int loop_sample;
    };

    void renderTitleList();
    void renderHeader();
    void renderSound();
    void renderTex();

    void loadMetaCache();

public:
    MainWindow();

    void render();

private:
    size_t m_selected_idx = SIZE_MAX;
    std::unique_ptr<Selection> m_curr_meta;
    std::vector<std::filesystem::path> m_meta_dirs;
    char m_ip[4 * 4];
    bool m_connected = false;
    bool m_is_ip_valid = true;
};

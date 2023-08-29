#pragma once
#include <memory>
#include <vector>
#include "file_dialog.hpp"
#include "gl_image.hpp"
#include "sound_player.hpp"
#include "title_meta.hpp"
#include "title_mgr.hpp"

class MainWindow
{
private:
    struct Selection
    {
        Selection(TitleMeta& meta);

        TitleMeta& meta;
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

    void showPopup(std::function<void()> func);
    void showError(const std::string& error);

public:
    MainWindow();

    void render();

private:
    long m_selected_idx = -1;
    std::unique_ptr<Selection> m_curr_meta;
    TitleMgr m_title_mgr;
    char m_ip[4 * 4];
    bool m_is_ip_valid = true;
    FileDialog m_file_dialog;
    std::function<void()> m_popup_func = nullptr;
    bool m_open_popup_req = false;
    int m_title_type = TitleType_MLC;
};

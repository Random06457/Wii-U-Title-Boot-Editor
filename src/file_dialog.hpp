#pragma once

#include <ImGuiFileDialog.h>
#include <functional>
#include <string>

class FileDialog
{
public:
    void open(const std::string& file_type,
              std::function<void(std::string)> callback);
    void render();

    void setWinFlags(ImGuiWindowFlags win_flags) { m_win_flags = win_flags; }
    void setDialogFlags(ImGuiFileDialogFlags dialog_flags)
    {
        m_dialog_flags = dialog_flags;
    }

private:
    bool m_opened = false;
    std::string m_file_type;
    std::function<void(std::string)> m_callback;
    ImGuiWindowFlags m_win_flags = 0;
    ImGuiFileDialogFlags m_dialog_flags = 0;
};

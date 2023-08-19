#include "file_dialog.hpp"

#include <ImGuiFileDialog.h>

void FileDialog::open(const std::string& file_type,
                      std::function<void(std::string)> callback)
{
    m_file_type = file_type;
    m_callback = callback;
    m_opened = true;
}

void FileDialog::render()
{
    if (!m_opened)
        return;

    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File",
                                            m_file_type.c_str(), ".", "", 1,
                                            nullptr, m_dialog_flags);
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", m_win_flags))
    {
        std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
        bool was_ok = ImGuiFileDialog::Instance()->IsOk();

        ImGuiFileDialog::Instance()->Close();
        m_opened = false;

        if (was_ok)
        {
            m_callback(file_path);
        }
    }
}

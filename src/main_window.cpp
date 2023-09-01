#include "main_window.hpp"
#include <cmath>
#include <fmt/format.h>
#include <imgui.h>
#include <ranges>
#include <string>
#include "macro.hpp"
#include "fs.hpp"
#include "title_meta.hpp"
#include "utils.hpp"

MainWindow::MainWindow()
{
    snprintf(m_ip, sizeof(m_ip), "192.168.0.10");
}

MainWindow::Selection::Selection(TitleMeta& title_meta) :
    meta(title_meta),
    drc_tex(meta.drcTex()),
    tv_tex(meta.tvTex()),
    logo_tex(meta.logoTex()),
    icon_tex(meta.iconTex()),
    target_idx(static_cast<int>(meta.sound().target())),
    loop_sample(static_cast<int>(meta.sound().loopSample()))
{
}

void MainWindow::renderSound()
{
    ImGui::Text("Boot Sound");
    ImGui::Spacing();
    const char* modes[] = { "TV", "DRC", "Both" };
    ImGui::Combo("Target", &m_curr_meta->target_idx, modes, ARRAY_COUNT(modes));

    ImGui::InputInt("Loop Sample", &m_curr_meta->loop_sample);

    ImGui::Spacing();

    if (m_player.isPlaying())
    {
        if (ImGui::Button("Pause"))
        {
            m_player.pause();
        }
    }
    else
    {
        if (ImGui::Button("Play"))
        {
            if (m_player.getCurrSample() == m_player.sound().sampleCount())
            {
                m_player.setCurrSample(0);
            }
            m_player.play();
        }
    }
    ImGui::SameLine();
    float int_part;
    ImGui::Text(
        "%02d:%02d:%02d / %02d:%02d:%02d", (int)m_player.getCurrTime() / 60,
        (int)m_player.getCurrTime() % 60,
        (int)(std::modf(m_player.getCurrTime(), &int_part) * 100),
        (int)m_player.sound().getDuration() / 60,
        (int)m_player.sound().getDuration() % 60,
        (int)(std::modf(m_player.sound().getDuration(), &int_part)) * 100);

    float player_pos = m_player.getCurrTime();
    if (ImGui::SliderFloat("## player time", &player_pos, 0.0f,
                           m_player.sound().getDuration(), "",
                           ImGuiSliderFlags_AlwaysClamp))
    {
        m_player.setCurrTime(player_pos);
    }

    for (size_t i = 0; i < m_player.sound().channels(); i++)
    {
        size_t curr_sample = m_player.getCurrSample();
        struct
        {
            SoundPlayer* x;
            size_t channel;
            size_t idx_off;
        } ctx0 = { &m_player, i, curr_sample };

        auto getter = [](void* data, int idx) -> float
        {
            auto ctx = reinterpret_cast<decltype(ctx0)*>(data);

            idx += static_cast<int>(ctx->idx_off);
            return ctx->x->sound().sampleNormalized((size_t)idx, ctx->channel);
        };

        size_t max_sample_count = m_player.sound().sampleCount() - curr_sample;
        size_t sample_count =
            std::min(m_player.sound().sampleRate() *
                         m_player.sound().bytesPerSample() / 10,
                     max_sample_count);

        ImGui::PlotLines(fmt::format("Channel {}", i).c_str(), getter, &ctx0,
                         (int)sample_count, 0, nullptr, -1, 1,
                         { 0.0F, 100.0f });
    }

    if (ImGui::Button("Import"))
    {
        m_file_dialog.setDialogFlags(ImGuiFileDialogFlags_Modal);
        m_file_dialog.open(
            ".wav",
            [this](const std::string& path)
            {
                auto wave = File::readAllBytes(path);
                SDL_AudioSpec spec;
                auto new_sound = Sound::fromWave(wave, &spec);
                if (!new_sound)
                {
                    showError("Invalid or corrupted sound");
                    return;
                }

                if (spec.channels != 2 || spec.freq != 44100 ||
                    spec.format != AUDIO_S16)
                {
                    auto fmt_str = [](SDL_AudioFormat fmt)
                    {
                        switch (fmt)
                        {
                            case AUDIO_U8: return "unsigned 8bit";
                            case AUDIO_S8: return "signed 8bit";
                            case AUDIO_U16: return "unsigned 16bit";
                            case AUDIO_S16: return "signed 16bit";
                            case AUDIO_S32: return "signed 32bit";
                            case AUDIO_F32: return "float 32bit";
                            default: return "Unkown";
                        }
                    };
                    showError(fmt::format(
                        "Warning: audio file is not in 2ch 44100hz signed 16bit"
                        " format.\nConversion that may decrease quality will "
                        "be performed.\nActual format: {}ch {}hz {}",
                        spec.channels, spec.freq, fmt_str(spec.format)));
                }

                m_player.pause();
                m_curr_meta->meta.sound() = std::move(*new_sound);

                m_player.setSound(&m_curr_meta->meta.sound());
            });
    }
    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        m_file_dialog.setDialogFlags(ImGuiFileDialogFlags_ConfirmOverwrite |
                                     ImGuiFileDialogFlags_Modal);
        m_file_dialog.open(".wav",
                           [this](const std::string& path)
                           {
                               auto wave = m_player.sound().toWave();
                               File::writeAllBytes(
                                   path,
                                   reinterpret_cast<const void*>(wave.data()),
                                   wave.size());
                           });
    }
}

static ImVec2 fitImage(ImVec2 src, ImVec2 dst)
{
    if (src.x > src.y)
        dst.y = dst.x * src.y / src.x;
    else
        dst.x = dst.y * src.x / src.y;
    return dst;
}

void MainWindow::renderTex()
{
    auto show_tex = [this](GlImage& gl_img, Image& img, const std::string& name,
                           ImVec2 size)
    {
        if (ImGui::BeginPopupContextItem(name.c_str()))
        {
            if (ImGui::Selectable("Import"))
            {
                m_file_dialog.setDialogFlags(ImGuiFileDialogFlags_Modal);
                m_file_dialog.open(
                    ".png,.jpg,.jpeg",
                    [this, &img, &gl_img](const std::string& path)
                    {
                        auto img_file = File::readAllBytes(path);
                        auto new_img = Image::fromStb(img_file);

                        if (!new_img)
                        {
                            showError("Invalid or unsupported file format!");
                            return;
                        }

                        if (new_img->width() != img.width() ||
                            new_img->height() != img.height())
                        {
                            showError(
                                fmt::format("Invalid Size: must be {}x{}.",
                                            img.width(), img.height()));
                            return;
                        }

                        img = std::move(*new_img);
                        gl_img = img;
                    });
            }
            if (ImGui::Selectable("Save"))
            {
                m_file_dialog.setDialogFlags(
                    ImGuiFileDialogFlags_ConfirmOverwrite |
                    ImGuiFileDialogFlags_Modal);
                m_file_dialog.open(".png,.jpg,.jpeg",
                                   [&img](const std::string& path)
                                   { img.saveAsPng(path); });
            }

            ImGui::EndPopup();
        }

        ImGui::Text("%s", name.c_str());
        if (ImGui::ImageButton(gl_img.imTexId(),
                               fitImage(gl_img.imSize(), size)))
        {
            ImGui::OpenPopup(name.c_str());
        }
    };

    {

        ImGui::BeginChild("## Big Textures", { 340, 0 });
        show_tex(m_curr_meta->tv_tex, m_curr_meta->meta.tvTex(), "TV Texture",
                 { 300, 300 });
        ImGui::Spacing();
        show_tex(m_curr_meta->drc_tex, m_curr_meta->meta.drcTex(),
                 "DRC Texture", { 300, 300 });
        ImGui::EndChild();
    }
    ImGui::SameLine();
    {
        ImGui::BeginChild("## Small Textures");
        show_tex(m_curr_meta->logo_tex, m_curr_meta->meta.logoTex(),
                 "Logo Texture", { 150, 150 });
        ImGui::Spacing();
        show_tex(m_curr_meta->icon_tex, m_curr_meta->meta.iconTex(),
                 "Icon Texture", { 150, 150 });
        ImGui::EndChild();
    }
}

static bool isValidIp(const char* ip_cstr)
{
    std::string ip = ip_cstr;

    size_t start_pos = 0;
    size_t end_pos = 0;

    auto is_valid_digit = [&ip, &start_pos, &end_pos]()
    {
        if (end_pos == std::string::npos)
            return false;

        if (end_pos - start_pos == 0)
            return false;

        size_t used = 0;
        int x = std::stoi(ip.substr(start_pos, end_pos - start_pos), &used);
        if (used != end_pos - start_pos)
            return false;
        if (x > 255)
            return false;
        return true;
    };

    end_pos = ip.find('.');
    if (!is_valid_digit())
        return false;

    start_pos = end_pos + 1;
    end_pos = ip.find('.', start_pos);
    if (!is_valid_digit())
        return false;

    start_pos = end_pos + 1;
    end_pos = ip.find('.', start_pos);
    if (!is_valid_digit())
        return false;

    start_pos = end_pos + 1;
    end_pos = ip.size();
    if (!is_valid_digit())
        return false;
    return true;
}

void MainWindow::renderTitleList()
{
    const char* labels[]{
        "USB",
        "MLC",
    };
    ImGui::BeginChild("Title Selector", ImVec2(200, 0), true);

    if (ImGui::Combo("Storage", &m_title_type, labels, ARRAY_COUNT(labels)))
    {
        m_selected_idx = -1;
        m_curr_meta.reset();
    }
    ImGui::Spacing();
    ImGui::Separator();

    if (m_title_mgr.connected())
    {
#ifdef __cpp_lib_ranges_enumerate
        for (auto [i, title_id] :
             m_title_mgr.getTitles() | std::views::enumerate)
        {
#else
        for (long i = 0; auto& title_id : m_title_mgr.getTitles())
        {
            ScopeExit i_inc([&i] { i++; });
#endif
            if (title_id.title_type != m_title_type)
                continue;

            if (ImGui::Selectable(fmt::format("{}", title_id.title_id).c_str(),
                                  i == m_selected_idx))
            {
                if (i != m_selected_idx)
                {
                    auto meta = m_title_mgr.getTitle(title_id);
                    if (!meta)
                    {
                        std::visit(
                            overloaded{
                                [this](const WiiuConnexionError& err)
                                {
                                    showError(err.error);
                                    m_curr_meta.reset();
                                    m_title_mgr.cleanup();
                                },
                                [this](const MetaDirMissingFileError& err) {
                                    showError(fmt::format("Could not find {}",
                                                          err.filename));
                                },
                                [this](const SoundError&)
                                { showError("Invalid BTSND File"); },
                                [this](const ImageError&) {
                                    showError(
                                        "Invalid or unsupported TGA File");
                                } },
                            meta.error());

                        if (std::holds_alternative<WiiuConnexionError>(
                                meta.error()))
                        {
                            break;
                        }
                        continue;
                    }

                    m_curr_meta = std::make_unique<Selection>(**meta);
                    m_player.setSound(&meta.value()->sound());
                }
                m_selected_idx = i;
            }
        }
    }

    ImGui::EndChild();
}

void MainWindow::renderHeader()
{
    ImGui::BeginChild("Wii U Connexion", { 0, 20 });

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 200 / 2);
    ImGui::SetNextItemWidth(150);

    bool old_valid = m_is_ip_valid;
    if (!old_valid)
        ImGui::PushStyleColor(ImGuiCol_Text, { 255, 0, 0, 255 });

    ImGui::BeginDisabled(m_title_mgr.connected());
    if (ImGui::InputText("##IP", m_ip, sizeof(m_ip)))
    {
        m_is_ip_valid = isValidIp(m_ip);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (m_title_mgr.connected())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0, 255, 0, 255 });
        ImGui::Text("Connected");
        ImGui::PopStyleColor();
    }
    else if (m_title_mgr.connecting())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 255, 255, 0, 255 });
        ImGui::Text("Connecting...");
        ImGui::PopStyleColor();
    }
    else if (old_valid)
    {
        if (ImGui::Button("Connect"))
        {
            auto res = m_title_mgr.connect(m_ip);
            if (!res)
            {
                showError(res.error().error);
                m_title_mgr.cleanup();
            }
        }
    }
    else
    {
        ImGui::Text("Invalid IP");
    }

    if (!old_valid)
        ImGui::PopStyleColor();

    ImGui::EndChild();
}

void MainWindow::showPopup(std::function<void()> func)
{
    m_popup_func = func;
    m_open_popup_req = true;
}

void MainWindow::showError(const std::string& msg)
{
    showPopup(
        [msg]()
        {
            ImGui::Text("%s", msg.c_str());

            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
            if (ImGui::Button("OK", { 120, 0 }))
                ImGui::CloseCurrentPopup();
        });
}

void MainWindow::render()
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoDecoration;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowSize({ 800, 700 });
    ImGui::SetNextWindowPos({ 0, 0 });

    m_file_dialog.setWinFlags(window_flags);
    m_file_dialog.render();

    ImGui::Begin("Main", nullptr, window_flags);

    // ImGuiIO& io = ImGui::GetIO();
    // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
    //             1000.0f / io.Framerate, io.Framerate);

    if (m_open_popup_req)
    {
        ImGui::OpenPopup("ModalPopup");
        m_open_popup_req = false;
    }

    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing,
                                ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("ModalPopup", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_popup_func)
                m_popup_func();

            ImGui::EndPopup();
        }
    }

    renderHeader();

    ImGui::BeginDisabled(!m_title_mgr.connected());
    renderTitleList();
    ImGui::EndDisabled();

    ImGui::SameLine();
    {
        ImGui::BeginChild("Title Pane", { 0, 0 }, true);

        if (m_curr_meta)
        {
            if (ImGui::BeginTabBar("## tabs"))
            {
                if (ImGui::BeginTabItem("Textures"))
                {
                    renderTex();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Sound"))
                {
                    renderSound();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

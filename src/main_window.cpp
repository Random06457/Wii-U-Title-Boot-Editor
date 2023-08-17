#include "main_window.hpp"
#include <cmath>
#include <fmt/format.h>
#include <imgui.h>
#include <string>
#include "macro.hpp"
#include "title_meta.hpp"

MainWindow::MainWindow()
{
    snprintf(m_ip, sizeof(m_ip), "192.168.0.10");
    loadMetaCache();
}

void MainWindow::loadMetaCache()
{
    auto cache_dir = std::filesystem::directory_iterator("cache");
    m_meta_dirs.clear();
    for (auto dir : cache_dir)
        m_meta_dirs.push_back(dir.path());
}

MainWindow::Selection::Selection(TitleMeta&& title_meta) :
    meta(std::move(title_meta)),
    drc_tex(meta.drcTex()),
    tv_tex(meta.tvTex()),
    logo_tex(meta.logoTex()),
    icon_tex(meta.iconTex()),
    player(meta.sound()),
    target_idx(static_cast<int>(meta.sound().target())),
    loop_sample(static_cast<int>(meta.sound().loopSample()))
{
}

void MainWindow::renderSound()
{
    auto& player = m_curr_meta->player;

    ImGui::Text("Boot Sound");
    ImGui::Spacing();
    const char* modes[] = { "TV", "DRC", "Both" };
    ImGui::Combo("Target", &m_curr_meta->target_idx, modes, ARRAY_COUNT(modes));

    ImGui::InputInt("Loop Sample", &m_curr_meta->loop_sample);

    ImGui::Spacing();

    if (player.isPlaying())
    {
        if (ImGui::Button("Pause"))
        {
            player.pause();
        }
    }
    else
    {
        if (ImGui::Button("Play"))
        {
            if (player.getCurrSample() == player.sound().sampleCount())
            {
                player.setCurrSample(0);
            }
            player.play();
        }
    }
    ImGui::SameLine();
    float int_part;
    ImGui::Text("%02d:%02d:%02d / %02d:%02d:%02d",
                (int)player.getCurrTime() / 60, (int)player.getCurrTime() % 60,
                (int)(std::modf(player.getCurrTime(), &int_part) * 100),
                (int)player.sound().getDuration() / 60,
                (int)player.sound().getDuration() % 60,
                (int)(std::modf(player.sound().getDuration(), &int_part)) *
                    100);

    float player_pos = player.getCurrTime();
    if (ImGui::SliderFloat("## a", &player_pos, 0.0f,
                           player.sound().getDuration(), "",
                           ImGuiSliderFlags_AlwaysClamp))
    {
        // player.pause();
        player.setCurrTime(player_pos);
        // player.setCurrTime(player.sound().getDuration() / 2);
        // player.play();
    }

    for (size_t i = 0; i < player.sound().channels(); i++)
    {
        size_t curr_sample = player.getCurrSample();
        struct
        {
            SoundPlayer* x;
            size_t channel;
            size_t idx_off;
        } ctx0 = { &player, i, curr_sample };

        auto getter = [](void* data, int idx) -> float
        {
            auto ctx = reinterpret_cast<decltype(ctx0)*>(data);

            idx += static_cast<int>(ctx->idx_off);
            return ctx->x->sound().sampleNormalized(idx, ctx->channel);
        };

        size_t max_sample_count = player.sound().sampleCount() - curr_sample;
        size_t sample_count = std::min(player.sound().sampleRate() *
                                           player.sound().bytesPerSample() / 10,
                                       max_sample_count);

        ImGui::PlotLines(fmt::format("Channel {}", i).c_str(), getter, &ctx0,
                         (int)sample_count, 0, nullptr, -1, 1,
                         { 0.0F, 100.0f });
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
    ImGui::Image(m_curr_meta->tv_tex.imTexId(),
                 fitImage(m_curr_meta->tv_tex.imSize(), { 300, 300 }));
    ImGui::SameLine();
    ImGui::Image(m_curr_meta->drc_tex.imTexId(),
                 fitImage(m_curr_meta->drc_tex.imSize(), { 250, 250 }));

    ImGui::Image(m_curr_meta->logo_tex.imTexId(),
                 fitImage(m_curr_meta->logo_tex.imSize(), { 300, 300 }));
    ImGui::SameLine();
    ImGui::Image(m_curr_meta->icon_tex.imTexId(),
                 fitImage(m_curr_meta->icon_tex.imSize(), { 150, 150 }));
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
    static int combo_idx = 0;
    const char* labels[]{
        "MLC",
        "USB",
    };
    ImGui::BeginChild("Title Selector", ImVec2(200, 0), true);

    ImGui::Combo("Storage", &combo_idx, labels, ARRAY_COUNT(labels));
    ImGui::Spacing();
    ImGui::Separator();

    for (size_t i = 0; auto& dir : m_meta_dirs)
    {
        if (ImGui::Selectable(
                fmt::format("{}", dir.filename().string()).c_str(),
                i == m_selected_idx))
        {
            if (i != m_selected_idx)
            {
                auto meta = TitleMeta::fromDir(dir);
                if (!meta)
                {
                    std::abort();
                }

                m_curr_meta = std::make_unique<Selection>(std::move(*meta));
            }
            m_selected_idx = i;
        }

        i++;
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

    ImGui::BeginDisabled(m_connected);
    if (ImGui::InputText("##IP", m_ip, sizeof(m_ip)))
    {
        m_is_ip_valid = isValidIp(m_ip);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (m_connected)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0, 255, 0, 255 });
        ImGui::Text("Connected");
        ImGui::PopStyleColor();
    }
    else if (old_valid)
    {
        if (ImGui::Button("Connect"))
        {
            m_connected = true;
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

void MainWindow::render()
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoDecoration;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowSize({ 800, 700 });
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::Begin("Main", nullptr, window_flags);

    // ImGuiIO& io = ImGui::GetIO();
    // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
    //             1000.0f / io.Framerate, io.Framerate);

    renderHeader();

    renderTitleList();

    ImGui::SameLine();
    {
        ImGui::BeginChild("Title Pane", { 0, 0 }, true);

        if (m_curr_meta)
        {
            renderSound();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            renderTex();
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

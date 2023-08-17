#pragma once

#include <expected>
#include <filesystem>
#include <variant>
#include <vector>
#include "image.hpp"
#include "sound.hpp"

struct MetaDirMissingFileError
{
    std::string filename;
};

class TitleMeta
{
private:
    TitleMeta(const std::filesystem::path& path, Image&& drc_tex,
              Image&& tv_tex, Image&& logo_tex, Image&& icon_tex,
              Sound&& boot_sound);

public:
    ~TitleMeta();
    TitleMeta(const TitleMeta&) = delete;
    TitleMeta(TitleMeta&& other) { *this = std::move(other); }
    TitleMeta& operator=(const TitleMeta&) = delete;
    TitleMeta& operator=(TitleMeta&& other);

    static auto fromDir(const std::filesystem::path& path)
        -> std::expected<TitleMeta, std::variant<ImageError, SoundError,
                                                 MetaDirMissingFileError>>;

    const Image& drcTex() const { return m_drc_tex; }
    const Image& tvTex() const { return m_tv_tex; }
    const Image& logoTex() const { return m_logo_tex; }
    const Image& iconTex() const { return m_icon_tex; }
    const Sound& sound() const { return m_boot_sound; }

private:
    std::filesystem::path m_dir;
    Image m_drc_tex;
    Image m_tv_tex;
    Image m_logo_tex;
    Image m_icon_tex;
    Sound m_boot_sound;
};

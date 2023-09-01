#pragma once

#include <filesystem>
#include <variant>
#include <vector>
#include "image.hpp"
#include "sound.hpp"
#include "utils.hpp"

struct MetaDirMissingFileError
{
    std::string filename;
};

class TitleMeta
{
public:
    TitleMeta(Image&& drc_tex, Image&& tv_tex, Image&& logo_tex,
              Image&& icon_tex, Sound&& boot_sound);
    ~TitleMeta();
    TitleMeta(const TitleMeta&) = delete;
    TitleMeta(TitleMeta&& other) { *this = std::move(other); }
    TitleMeta& operator=(const TitleMeta&) = delete;
    TitleMeta& operator=(TitleMeta&& other);

    static auto fromDir(const std::filesystem::path& path)
        -> Expected<TitleMeta, std::variant<ImageError, SoundError,
                                            MetaDirMissingFileError>>;

    const Image& drcTex() const { return m_drc_tex; }
    const Image& tvTex() const { return m_tv_tex; }
    const Image& logoTex() const { return m_logo_tex; }
    const Image& iconTex() const { return m_icon_tex; }
    const Sound& sound() const { return m_boot_sound; }
    bool isDirty() const { return m_is_dirty; }
    void setDirty(bool dirty) { m_is_dirty = dirty; }
    // deducing this when :/
    Image& drcTex() { return m_drc_tex; }
    Image& tvTex() { return m_tv_tex; }
    Image& logoTex() { return m_logo_tex; }
    Image& iconTex() { return m_icon_tex; }
    Sound& sound() { return m_boot_sound; }

private:
    Image m_drc_tex;
    Image m_tv_tex;
    Image m_logo_tex;
    Image m_icon_tex;
    Sound m_boot_sound;
    bool m_is_dirty = false;
};

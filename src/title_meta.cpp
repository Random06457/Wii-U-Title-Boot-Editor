#include "title_meta.hpp"
#include "fs.hpp"

TitleMeta::TitleMeta(Image&& drc_tex, Image&& tv_tex, Image&& logo_tex,
                     Image&& icon_tex, Sound&& boot_sound) :
    m_drc_tex(std::move(drc_tex)),
    m_tv_tex(std::move(tv_tex)),
    m_logo_tex(std::move(logo_tex)),
    m_icon_tex(std::move(icon_tex)),
    m_boot_sound(std::move(boot_sound))
{
}

TitleMeta::~TitleMeta()
{
}

TitleMeta& TitleMeta::operator=(TitleMeta&& other)
{
    m_drc_tex = std::move(other.m_drc_tex);
    m_tv_tex = std::move(other.m_tv_tex);
    m_logo_tex = std::move(other.m_logo_tex);
    m_icon_tex = std::move(other.m_icon_tex);
    m_boot_sound = std::move(other.m_boot_sound);
    return *this;
}

auto TitleMeta::fromDir(const std::filesystem::path& path)
    -> Expected<TitleMeta,
                std::variant<ImageError, SoundError, MetaDirMissingFileError>>
{
    auto drc_tex_path = path / "bootDrcTex.tga";
    auto tv_tex_path = path / "bootTvTex.tga";
    auto logo_tex_path = path / "bootLogoTex.tga";
    auto icon_tex_path = path / "iconTex.tga";
    auto boot_sound_path = path / "bootSound.btsnd";

    if (!File::exists(drc_tex_path))
        return Unexpected(MetaDirMissingFileError{ "bootDrcTex.tga" });
    if (!File::exists(tv_tex_path))
        return Unexpected(MetaDirMissingFileError{ "bootTvTex.tga" });
    if (!File::exists(logo_tex_path))
        return Unexpected(MetaDirMissingFileError{ "bootLogoTex.tga" });
    if (!File::exists(icon_tex_path))
        return Unexpected(MetaDirMissingFileError{ "iconTex.tga" });
    if (!File::exists(boot_sound_path))
        return Unexpected(MetaDirMissingFileError{ "bootSound.btsnd" });

    auto drc_tex = Image::fromWiiU(File::readAllBytes(drc_tex_path));
    if (!drc_tex)
        return Unexpected(drc_tex.error());

    auto tv_tex = Image::fromWiiU(File::readAllBytes(tv_tex_path));
    if (!tv_tex)
        return Unexpected(tv_tex.error());

    auto logo_tex = Image::fromWiiU(File::readAllBytes(logo_tex_path));
    if (!logo_tex)
        return Unexpected(logo_tex.error());

    auto icon_tex = Image::fromWiiU(File::readAllBytes(icon_tex_path));
    if (!icon_tex)
        return Unexpected(icon_tex.error());

    auto boot_sound = Sound::fromBtsnd(File::readAllBytes(boot_sound_path));
    if (!boot_sound)
        return Unexpected(boot_sound.error());

    return TitleMeta(std::move(*drc_tex), std::move(*tv_tex),
                     std::move(*logo_tex), std::move(*icon_tex),
                     std::move(*boot_sound));
}

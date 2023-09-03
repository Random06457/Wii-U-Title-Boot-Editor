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
    -> Result<TitleMeta, ImageError, SoundError, MetaDirMissingFileError>
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

    auto drc_tex = PROPAGATE(Image::fromWiiU(File::readAllBytes(drc_tex_path)));
    auto tv_tex = PROPAGATE(Image::fromWiiU(File::readAllBytes(tv_tex_path)));
    auto logo_tex =
        PROPAGATE(Image::fromWiiU(File::readAllBytes(logo_tex_path)));
    auto icon_tex =
        PROPAGATE(Image::fromWiiU(File::readAllBytes(icon_tex_path)));
    auto boot_sound =
        PROPAGATE(Sound::fromBtsnd(File::readAllBytes(boot_sound_path)));

    return TitleMeta(std::move(drc_tex), std::move(tv_tex), std::move(logo_tex),
                     std::move(icon_tex), std::move(boot_sound));
}

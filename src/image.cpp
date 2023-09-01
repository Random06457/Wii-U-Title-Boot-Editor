#include "image.hpp"

#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

Image::Image(u32* data, size_t width, size_t height,
             std::function<void(u32*)> dtor) :
    m_data(data, dtor),
    m_width(width),
    m_height(height)
{
}

Image& Image::operator=(const Image& img)
{
    m_data = { new u32[img.m_width * img.m_height], img.m_data.get_deleter() };
    m_width = img.m_width;
    m_height = img.m_height;
    std::memcpy(m_data.get(), img.m_data.get(),
                m_width * m_height * sizeof(u32));
    return *this;
}
Image& Image::operator=(Image&& img)
{
    m_data = std::move(img.m_data);
    m_width = img.m_width;
    m_height = img.m_height;
    img.m_data.reset();
    return *this;
}

std::expected<Image, ImageError> Image::fromStb(const void* data,
                                                size_t data_size)
{
    int w;
    int h;
    auto stbi_data =
        stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data),
                              static_cast<int>(data_size), &w, &h, nullptr, 4);
    if (!stbi_data)
        return std::unexpected(ImageError_LoadingStbiImageFailed);

    return Image(reinterpret_cast<u32*>(stbi_data), static_cast<size_t>(w),
                 static_cast<size_t>(h),
                 [](u32* img)
                 { stbi_image_free(reinterpret_cast<void*>(img)); });
}

std::expected<Image, ImageError> Image::fromWiiU(const void* tga_data,
                                                 size_t data_size)
{
    struct [[gnu::packed]] TgaHeader
    {
        u8 id_length;
        u8 color_map_type;
        u8 image_type;
        u8 color_map_spec[5];
        u16 x;
        u16 y;
        u16 width;
        u16 height;
        u8 bpp;
        u8 image_descriptor;
        u8 data[];
    };
    static_assert(sizeof(TgaHeader) == 18);

    if (data_size < sizeof(TgaHeader))
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);

    auto hdr = reinterpret_cast<const TgaHeader*>(tga_data);

    if (data_size < sizeof(TgaHeader) + hdr->width * hdr->height * hdr->bpp / 8)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);

    if (hdr->id_length != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_type != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->image_type != 2)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_spec[0] != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_spec[1] != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_spec[2] != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_spec[3] != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->color_map_spec[4] != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);

    if (hdr->x != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->y != 0)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if (hdr->bpp != 32 && hdr->bpp != 24)
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);
    if ((hdr->bpp == 32 && hdr->image_descriptor != 8) ||
        (hdr->bpp == 24 && hdr->image_descriptor != 0))
        return std::unexpected(ImageError_InvalidOrUnsupportedTGA);

    u32* data = new u32[hdr->width * hdr->height];

    for (size_t y = 0; y < hdr->height; y++)
    {
        size_t dst_off = (hdr->height - y - 1) * hdr->width;
        size_t src_off = y * hdr->width * hdr->bpp / 8;
        for (size_t x = 0; x < hdr->width; x++)
        {
            u8 r = hdr->data[src_off + x * hdr->bpp / 8 + 0];
            u8 g = hdr->data[src_off + x * hdr->bpp / 8 + 1];
            u8 b = hdr->data[src_off + x * hdr->bpp / 8 + 2];
            u8 a = hdr->bpp == 32 ? hdr->data[src_off + x * hdr->bpp / 8 + 3]
                                  : 0xFF;

            data[dst_off + x] = b | g << 8 | r << 16 | a << 24;
        }
    }

    return Image(data, hdr->width, hdr->height);
}

bool Image::saveAsPng(const std::filesystem::path& path) const
{
    return stbi_write_png(path.string().c_str(), static_cast<int>(m_width),
                          static_cast<int>(m_height), 4, data<void>(), 0) != 0;
}

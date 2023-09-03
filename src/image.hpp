#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include "types.hpp"
#include "utils.hpp"

enum ImageError
{
    ImageError_LoadingStbiImageFailed,
    ImageError_InvalidOrUnsupportedTGA,
};

class Image
{
public:
    Image(
        u32* data, size_t width, size_t height,
        std::function<void(u32*)> dtor = [](u32* x) { delete[] x; });
    ~Image() {}

    Image() = default;
    Image(const Image& img) { *this = img; }
    Image(Image&& img) { *this = std::move(img); }
    Image& operator=(const Image& img);
    Image& operator=(Image&& img);

    static auto fromStb(const std::vector<u8>& data)
        -> Result<Image, ImageError>
    {
        return fromStb(reinterpret_cast<const void*>(data.data()), data.size());
    }
    static auto fromStb(const void* data, size_t data_size)
        -> Result<Image, ImageError>;

    static auto fromWiiU(const std::vector<u8>& data)
        -> Result<Image, ImageError>
    {
        return fromWiiU(reinterpret_cast<const void*>(data.data()),
                        data.size());
    }
    static auto fromWiiU(const void* data, size_t data_size)
        -> Result<Image, ImageError>;

    bool saveAsPng(const std::filesystem::path& path) const;
    std::vector<u8> toWiiU(bool is_bg) const;

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    template<typename T>
    const T* data() const
    {
        return reinterpret_cast<const T*>(m_data.get());
    }

private:
    std::unique_ptr<u32[], std::function<void(u32*)>> m_data;
    size_t m_width;
    size_t m_height;
};

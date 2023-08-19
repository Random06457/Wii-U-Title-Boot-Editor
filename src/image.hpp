#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include "types.hpp"

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

    static std::expected<Image, ImageError> fromStb(const std::vector<u8>& data)
    {
        return fromStb(reinterpret_cast<const void*>(data.data()), data.size());
    }
    static std::expected<Image, ImageError> fromStb(const void* data,
                                                    size_t data_size);
    static std::expected<Image, ImageError>
    fromWiiU(const std::vector<u8>& data)
    {
        return fromWiiU(reinterpret_cast<const void*>(data.data()),
                        data.size());
    }
    static std::expected<Image, ImageError> fromWiiU(const void* data,
                                                     size_t data_size);

    bool saveAsPng(const std::filesystem::path& path) const;

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

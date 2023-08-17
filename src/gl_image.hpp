#pragma once

#include <SDL_opengl.h>
#include <imgui.h>
#include "types.hpp"
#include "image.hpp"

struct GlImage
{
    GlImage() = default;

    GlImage(GlImage&& img) { *this = std::move(img); }
    GlImage& operator=(GlImage&& img)
    {
        m_id = img.m_id;
        m_width = img.m_width;
        m_height = img.m_height;
        img.m_id = 0;
        return *this;
    }

    GlImage(const Image& img) :
        GlImage(img.data<void>(), img.width(), img.height())
    {
    }
    GlImage(const void* data, size_t width, size_t height) :
        m_id(0),
        m_width(width),
        m_height(height)
    {
        // Create a OpenGL texture identifier
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     data);
    }
    ~GlImage()
    {
        if (m_id != 0)
            glDeleteTextures(1, &m_id);
    }

    ImTextureID imTexId()
    {
        return reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_id));
    }

    size_t width() const
    {
        return m_width;
    }
    size_t height() const
    {
        return m_height;
    }

    ImVec2 imSize() const
    {
        return { static_cast<float>(m_width), static_cast<float>(m_height) };
    }

private:
    GLuint m_id = 0;
    size_t m_width;
    size_t m_height;
};

//
// Created by maciej on 20.09.25.
//

// src/gfx/Texture.cpp
#define STB_IMAGE_IMPLEMENTATION
#include "Texture.hpp"
#include "stb/stb_image.h"
#include <glad.h>
#include <string>
#include <stdexcept>

static GLuint makeTex2D(int w, int h, GLenum internalFmt, GLenum fmt, const void* data) {
    GLuint tex=0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureStorage2D(tex, 1, internalFmt, w, h);
    glTextureSubImage2D(tex, 0, 0,0, w,h, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(tex);
    return tex;
}

GLuint LoadTexture2D(const std::string& path, bool srgb)
{
    stbi_set_flip_vertically_on_load(true);
    int w,h,n;
    unsigned char* img = stbi_load(path.c_str(), &w, &h, &n, 0);
    if(!img) throw std::runtime_error("stbi_load failed: " + path);

    GLenum fmt = (n==1)? GL_RED : (n==3)? GL_RGB : GL_RGBA;
    GLenum internal = fmt;
    if (srgb) {
        internal = (n==4)? GL_SRGB8_ALPHA8 : GL_SRGB8;
    } else {
        internal = (n==1)? GL_R8 : (n==3)? GL_RGB8 : GL_RGBA8;
    }
    GLuint tex = makeTex2D(w,h, internal, fmt, img);
    stbi_image_free(img);
    return tex;
}

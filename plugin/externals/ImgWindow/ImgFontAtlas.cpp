/*
 * ImgFontAtlas.cpp
 *
 * Integration for dear imgui into X-Plane: ImGui Font Atlas
 *
 * Copyright (C) 2020, Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "ImgFontAtlas.h"
#include <XPLMGraphics.h>

ImgFontAtlas::ImgFontAtlas() :
    mOurAtlas(nullptr),
    mTextureBound(false),
    mGLTextureNum(0)
{
    mOurAtlas = new ImFontAtlas;
}

ImgFontAtlas::~ImgFontAtlas()
{
    if (mTextureBound) {
        GLuint glTexNum = mGLTextureNum;
        glDeleteTextures(1, &glTexNum);
        mTextureBound = false;
    }
    delete mOurAtlas;
    mOurAtlas = nullptr;
}

ImFont*
ImgFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    return mOurAtlas->AddFont(font_cfg);
}

ImFont*
ImgFontAtlas::AddFontDefault(const ImFontConfig* font_cfg)
{
    return mOurAtlas->AddFontDefault(font_cfg);
}

ImFont*
ImgFontAtlas::AddFontFromFileTTF(const char* filename,
    float size_pixels,
    const ImFontConfig* font_cfg,
    const unsigned short* glyph_ranges)
{
    return mOurAtlas->AddFontFromFileTTF(filename, size_pixels, font_cfg, glyph_ranges);
}

ImFont*
ImgFontAtlas::AddFontFromMemoryTTF(void* font_data,
    int font_size,
    float size_pixels,
    const ImFontConfig* font_cfg,
    const unsigned short* glyph_ranges)
{
    return mOurAtlas->AddFontFromMemoryTTF(font_data, font_size, size_pixels, font_cfg, glyph_ranges);
}

ImFont*
ImgFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_font_data,
    int compressed_font_size,
    float size_pixels,
    const ImFontConfig* font_cfg,
    const unsigned short* glyph_ranges)
{
    return mOurAtlas->AddFontFromMemoryCompressedTTF(compressed_font_data, compressed_font_size, size_pixels, font_cfg, glyph_ranges);
}

ImFont*
ImgFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_font_data_base85,
    float size_pixels,
    const ImFontConfig* font_cfg,
    const unsigned short* glyph_ranges)
{
    return mOurAtlas->AddFontFromMemoryCompressedBase85TTF(compressed_font_data_base85, size_pixels, font_cfg, glyph_ranges);
}

ImFontAtlas*
ImgFontAtlas::getAtlas()
{
    return mOurAtlas;
}

void
ImgFontAtlas::bindTexture()
{
    if (mTextureBound)
        return;

    XPLMGenerateTextureNumbers(&mGLTextureNum, 1);

    unsigned char* pixData = nullptr;
    int width, height;
    mOurAtlas->GetTexDataAsRGBA32(&pixData, &width, &height);

    XPLMBindTexture2d(mGLTextureNum, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixData);

    mOurAtlas->SetTexID((void*)((intptr_t)mGLTextureNum));
    mTextureBound = true;
}
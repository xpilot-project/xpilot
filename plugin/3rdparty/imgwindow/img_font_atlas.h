/*
 * ImgFontAtlas.h
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

#ifndef IMGFONTATLAS_H
#define IMGFONTATLAS_H

#include "SystemGL.h"
#include "imgui.h"

/** Construct an empty font atlas we can use later
 *
 * This also assigns the texture name which is necessary as, again, must be done
 * in an x-plane compatible manner.
 *
 * @return an empty font atlas ready to be initialised.
 */
class ImgFontAtlas {
public:
    ImgFontAtlas();

    virtual ~ImgFontAtlas();

    ImFont* AddFont(const ImFontConfig* font_cfg);

    ImFont* AddFontDefault(const ImFontConfig* font_cfg = NULL);

    ImFont* AddFontFromFileTTF(const char* filename,
        float size_pixels,
        const ImFontConfig* font_cfg = NULL,
        const unsigned short* glyph_ranges = NULL);

    ImFont* AddFontFromMemoryTTF(void* font_data,
        int font_size,
        float size_pixels,
        const ImFontConfig* font_cfg = NULL,
        const unsigned short* glyph_ranges = NULL); // Note: Transfer ownership of 'ttf_data' to ImFontAtlas! Will be deleted after destruction of the atlas. Set font_cfg->FontDataOwnedByAtlas=false to keep ownership of your data and it won't be freed.
    ImFont* AddFontFromMemoryCompressedTTF(const void* compressed_font_data,
        int compressed_font_size,
        float size_pixels,
        const ImFontConfig* font_cfg = NULL,
        const unsigned short* glyph_ranges = NULL); // 'compressed_font_data' still owned by caller. Compress with binary_to_compressed_c.cpp.
    ImFont* AddFontFromMemoryCompressedBase85TTF(const char* compressed_font_data_base85,
        float size_pixels,
        const ImFontConfig* font_cfg = NULL,
        const unsigned short* glyph_ranges = NULL);              // 'compressed_font_data_base85' still owned by caller. Compress with binary_to_compressed_c.cpp with -base85 parameter.

/** bindTexture creates and binds the font texture to OpenGL, ready for use.
 *
 * This should be called after all fonts are loaded, before any rendering occurs!
 */
    virtual void bindTexture();

    ImFontAtlas* getAtlas();
protected:
    ImFontAtlas* mOurAtlas;
    bool        mTextureBound;
    int         mGLTextureNum;
};

#endif //IMGFONTATLAS_H
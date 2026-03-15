/// @file       FMOD_Logo.h
/// @brief      Compressed version of the FMOD logo with code to create
///             a texture ID in X-Plane for display with Dear ImGui
/// @see        [FMOD attribution requirements](https://www.fmod.com/attribution)
/// @see        [FMOD Logo download](https://www.fmod.com/assets/FMOD_LOGOS.zip)
/// @details    The FMOD logo is essential a single-color image,
///             either black (RGB 0x06 0x08 0x0b) or white (0xff 0xff 0xff),
///             with only the alpha value varying across the image.
///             The below array `ALPHA_RLE` is a run-length encoded dump
///             of these alpha values.
///             The provided functions recreate the full RGBA data,
///             so that it can be used with OpenGL functionality to be turned
///             into a texture.
/// @details    **Usage**\n
///
///             #include "FMOD_Logo.h"
///             ...
///             int logoId = 0;
///             if (FMODLogo::GetTexture(logoId)) {
///                 ImGui::Image((void*)(intptr_t)logoId, ImVec2(FMODLogo::IMG_WIDTH, FMODLogo::IMG_HEIGHT));
///             }
///
/// @copyright  of the logo itself: Firelight Technologies Pty Ltd.
///
/// @author     of the code: Birger Hoppe
/// @copyright  (c) 2022 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#pragma once

#include <cstdint>

namespace FMODLogo {

// Dimensions of the FMOD logo
constexpr int IMG_WIDTH  = 728;                         ///< Logo image width
constexpr int IMG_HEIGHT = 192;                         ///< Logo image height
constexpr int IMG_SIZE   = IMG_WIDTH * IMG_HEIGHT;      ///< Logo image size in number of pixels

/// @brief The one "public" function to return a texture id for the logo
/// @details On first call will load the logo and create a texture from it,
///          on subsequent calls will return the already loaded id.
/// @param[out] texId receives the texture id for the (black or white) FMOD logo
/// @param bBlack [opt] Black or White logo? (Will return different texture ids)
/// @returns `true` if successful
bool GetTexture (int& texId, bool bBlack = true);

/// @brief Load the data of the FMOD logo, expanded into RGBA data
/// @details This function is internally used by GetTexture().
///          It is _not_ typically used publicly but provided just in case
///          what you need is not an OpenGL texture but the raw RGBA data.
///          Data is provided in 4-byte blocks, one byte per RGBA value.
///          The returned array has size `IMG_SIZE * 4`.
///          Caller has to free the data using `delete[]`.
/// @returns pointer to raw RGBA data, 4 bytes per pixel.
uint8_t* LoadRGBA (bool bBlack = true);

}; // namespace FMODLogo

/// @file       XPMPPlaneRenderer.h
/// @brief      Rendering functions.
/// @note       This file bases on and is compatible to the header
///             provided with the original libxplanemp.
/// @deprecated None of these functions are supported any longer,
///             their definitions are only provided for backward compatibility.
/// @author     Birger Hoppe
/// @copyright  Copyright (c) 2005, Ben Supnik and Chris Serio.
/// @copyright  (c) 2020 Birger Hoppe
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

#ifndef XPLMPLANERENDERER_H
#define XPLMPLANERENDERER_H

// Theoretically you can plug in your own plane-rendering
// function (although in practice this isn't real useful.
// These functions do "the drawing" once per frame.

/// @brief Was an internal libxplanemp functions
/// @deprecated No longer supported.
[[deprecated("Internal function no longer supported")]]
void            XPMPInitDefaultPlaneRenderer(void);

/// @brief Was an internal libxplanemp functions
/// @deprecated No longer supported.
[[deprecated("Internal function no longer supported")]]
void            XPMPDefaultPlaneRenderer(int is_blend);

/// @brief Was an internal libxplanemp functions
/// @deprecated No longer supported.
[[deprecated("Internal function no longer supported")]]
void            XPMPDeinitDefaultPlaneRenderer(void);

#endif

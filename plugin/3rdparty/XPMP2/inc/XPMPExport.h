/// @file       XPMPExport.h
/// @brief      Macro definitions to support building/using a Windows DLL
/// @details    When building a Windows DLL version of XPMP2,
///             This file switches between `dllexport` and `dllimport`
///             declarations.
///
///             The default remains to be building a static library,
///             for which you as a plugin author don't need to set anything special.
///
///             Before considering to use a DLL version refer to the
///             [XPMP2 Build](https://twinfan.github.io/XPMP2/Building.html)
///             documentation.
///
///             The XPMP2 build process defines `XPMP2_DLLEXPORT` when
///             `-D"XPMP2_BUILD_SHARED_LIBS:boolean=TRUE"` is passed to the `cmake`
///             call, to build the DLL.
///
///             If you want to use the `XPMP2.dll` from your plugin you need to
///             1. define `XPMP2_DLLIMPORT` before including any XPMP2 header,
///             2. link to the `XPMP2.lib` DLL import library, one of the outputs
///                of the DLL build process, and
///             3. ship the `XPMP2.dll` with your plugin.
///
/// @author     Birger Hoppe
/// @author     Jean Philippe Lebel
/// @copyright  (c) 2026 Birger Hoppe
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

#ifndef XPMP_EXPORT_H
#define XPMP_EXPORT_H

#if defined(XPMP2_DLLEXPORT)                    // Building the DLL
#define XPMP2_EXPORT __declspec(dllexport)
#elif defined(XPMP2_DLLIMPORT)                  // Importing/using DLL
#define XPMP2_EXPORT __declspec(dllimport)
#else                                           // Building/using static library
#define XPMP2_EXPORT
#endif

#endif /* XPMP_EXPORT_H */

/// @file       Contrail.h
/// @brief      Manages contrails for an aircraft
/// @details    Contrails are created using X-Planes particle system.
///             The particle emitter is embedded in one mostly empty
///             object file in `Resources/Contrail/Contrail.obj`.
///             This object is loaded once and then instanciated
///             for any plane that needs contrails.
///             Rendering particles can eat some FPS, hence it is up to
///             the plugin to tell XPMP2 how many contrails to draw.
///             Realistically, there should be one per engine, but that
///             could dramatically increase the number of particles,
///             so it might be advisable to just have one contrail rendered
///             for a good effect but way less FPS hit.
///
/// @see        https://developer.x-plane.com/article/x-plane-11-particle-system/
///
/// @author     Birger Hoppe
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

namespace XPMP2 {

//
// MARK: Global Functions
//

/// Graceful shutdown
void ContrailCleanup ();

}

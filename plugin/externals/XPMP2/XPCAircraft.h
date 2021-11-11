/// @file       XPCAircraft.h
/// @brief      XPMP2::Aircraft / XPCAircraft represent an aircraft as managed by XPMP2
/// @deprecated XPCAircraft bases on and is compile-compatible to the XPCAircraft wrapper
///             class provided with the original libxplanemp.
///             In XPMP2, however, this class is not a wrapper but derived from
///             XPMP2::Aircraft, which is the actual means of managing aircraft,
///             Hence, it includes a lot more members.\n
///             New implementations should derive directly from XPMP2::Aircraft.
/// @author     Birger Hoppe
/// @copyright  The original XPCAircraft.h file in libxplanemp had no copyright note.
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

#ifndef _XPCAircraft_h_
#define _XPCAircraft_h_

#include "XPMPAircraft.h"

/// @brief Legacy wrapper class as provided by original libxplanemp
/// @deprecated Only provided for backwards compile-compatibility.
///             New implementations should subclass XPMP2::Aircraft directly.
class [[deprecated("Subclass XPMP2::Aircraft instead")]]
XPCAircraft : public XPMP2::Aircraft {
    
public:
    /// Last position data. GetPlanePosition() passes a pointer to this member variable
    XPMPPlanePosition_t acPos;
    /// Last surface data. GetPlaneSurfaces() passes a pointer to this member variable
    XPMPPlaneSurfaces_t acSurfaces;

public:
    
    /// @brief Legacy constructor creates a plane and puts it under control of XPlaneMP
    /// @exception XPMP2::XPMP2Error Mode S id invalid or duplicate, no model found during model matching
    /// @param _icaoType ICAO aircraft type designator, like 'A320', 'B738', 'C172'
    /// @param _icaoAirline ICAO airline code, like 'BAW', 'DLH', can be an empty string
    /// @param _livery Special livery designator, can be an empty string
    /// @param _modeS_id (optional) Unique identification of the plane [0x01..0xFFFFFF], e.g. the 24bit mode S transponder code. XPMP2 assigns an arbitrary unique number of not given
    /// @param _modelId (optional) specific model id to be used (no folder/package name, just the id as defined in the `OBJ8_AIRCRAFT` line)
    XPCAircraft(const char* _icaoType,
                const char* _icaoAirline,
                const char* _livery,
                XPMPPlaneID _modeS_id = 0,              // new parameters are defaulted, so that old code should compile
                const char* _modelId = nullptr);
    
    /// Legacy: Called before rendering to query plane's current position, overwrite to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlanePosition(XPMPPlanePosition_t* outPosition) = 0;
    /// Legacy: Called before rendering to query plane's current configuration, overwrite to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlaneSurfaces(XPMPPlaneSurfaces_t* outSurfaces) = 0;
    /// Legacy: Called before rendering to query plane's current radar visibility, overwrite to provide your implementation
    virtual XPMPPlaneCallbackResult GetPlaneRadar(XPMPPlaneRadar_t* outRadar)          = 0;
    /// Legacy: Called before rendering to query plane's textual information, overwrite to provide your implementation (optional)
    virtual XPMPPlaneCallbackResult GetInfoTexts(XPMPInfoTexts_t * /*outInfoTexts*/)
    {  return xpmpData_Unavailable; }

    /// Just calls all 4 previous `Get...` functions and copies the provided values into `drawInfo` and `v`
    virtual void UpdatePosition (float _elapsedSinceLastCall, int _flCounter);

};

#endif

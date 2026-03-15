/// @file       Aircraft.h
/// @brief      Additional definitions beyond what public XPCAircraft defines
/// @author     Birger Hoppe
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

#ifndef _Aircraft_h_
#define _Aircraft_h_

namespace XPMP2 {

/// This class helps creating aircraft via the legacy global functions. It provides standard implementations of the abstract callbacks
class [[deprecated("Don't use directly, only defined to support deprecated global XPMP functions")]]
LegacyAircraft : public XPCAircraft {
protected:
    /// Points to the callback function provided by caller to XPMPCreatePlane()
    XPMPPlaneData_f dataFunc = nullptr;
    /// The refcon provided by caller to XPMPCreatePlane()
    void*           refcon   = nullptr;
public:
    /// Constructor accepts the very same parameters as XPMPCreatePlane() and XPMPCreatePlaneWithModelName()
    LegacyAircraft(const char*      inICAOCode,
                   const char*      inAirline,
                   const char*      inLivery,
                   XPMPPlaneData_f  inDataFunc,
                   void *           inRefcon,
                   XPMPPlaneID      inModeS_id = 0,
                   const char *     inModelName = nullptr);

    /// Just calls `dataFunc`
    virtual XPMPPlaneCallbackResult GetPlanePosition(XPMPPlanePosition_t* outPosition);
    /// Just calls `dataFunc`
    virtual XPMPPlaneCallbackResult GetPlaneSurfaces(XPMPPlaneSurfaces_t* outSurfaces);
    /// Just calls `dataFunc`
    virtual XPMPPlaneCallbackResult GetPlaneRadar(XPMPPlaneRadar_t* outRadar);
    /// Just calls `dataFunc`
    virtual XPMPPlaneCallbackResult GetInfoTexts(XPMPInfoTexts_t * outInfoTexts);
};

/// @brief Map of all aircraft, key is tthe plane id
/// @note   Map stores pointers and _does not own_ the objects.
///         Plugin (the one using this library) is expected to own and destroy the object
typedef std::map<XPMPPlaneID,Aircraft*> mapAcTy;

//
// MARK: Global Functions
//

/// Initialize the module
void AcInit ();

/// Grace cleanup, esp. remove all aircraft
void AcCleanup ();

}   // namespace XPMP2

#endif

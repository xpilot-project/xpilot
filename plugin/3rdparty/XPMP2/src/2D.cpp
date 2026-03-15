/// @file       2D.cpp
/// @brief      Implementation of 2-D routines, like drawing aircraft labels
/// @details    2-D drawing is a bit "unnatural" as the aircraft are put into a 3-D world.
///             These functions require to turn 3D coordinates into 2D coordinates.
/// @see        Laminar's sample code at https://developer.x-plane.com/code-sample/coachmarks/
///             is the basis, then been taken apart.
/// @author     Laminar Research
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

#include "XPMP2.h"

#define DEBUG_ENABLE_AC_LABELS  "Aircraft labels %s"

namespace XPMP2 {

//
// MARK: 2-D projection calculations
//

// Data refs we need
static XPLMDataRef drMatrixWrld     = nullptr;  ///< sim/graphics/view/world_matrix
static XPLMDataRef drMatrixProj     = nullptr;  ///< sim/graphics/view/projection_matrix_3d
static XPLMDataRef drScreenWidth    = nullptr;  ///< sim/graphics/view/window_width
static XPLMDataRef drScreenHeight   = nullptr;  ///< sim/graphics/view/window_height
static XPLMDataRef drVisibility     = nullptr;  ///< sim/graphics/view/visibility_effective_m or sim/weather/visibility_effective_m
static XPLMDataRef drFieldOfView    = nullptr;  ///< sim/graphics/view/field_of_view_deg

/// world matrix (updates once per cycle)
static float gMatrixWrld[16];
/// projection matrix (updated once per cycle)
static float gMatrixProj[16];
/// Screen size (with, height)
static float gScreenW, gScreenH;
/// Field of view
static float gFOV;

/// 4x4 matrix transform of an XYZW coordinate - this matches OpenGL matrix conventions.
static void mult_matrix_vec(float dst[4], const float m[16], const float v[4])
{
    dst[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
    dst[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
    dst[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
    dst[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
}


/// Once per cycle read necessary matrices from X-Plane
static void read_matrices ()
{
    // Read the model view and projection matrices from this frame
    XPLMGetDatavf(drMatrixWrld,gMatrixWrld,0,16);
    XPLMGetDatavf(drMatrixProj,gMatrixProj,0,16);
    
    // Read the screen size (won't change often if at all...but could!)
    gScreenW = (float)XPLMGetDatai(drScreenWidth);
    gScreenH = (float)XPLMGetDatai(drScreenHeight);
    
    // Field of view
    gFOV = XPLMGetDataf(drFieldOfView);
}

// This drawing callback will draw a label to the screen where the

/// @brief Converts 3D local coordinates to 2D screen coordinates
/// @note Requires matrices to be set up already by a call to read_matrices()
/// @return Are coordinates visible? (Otherwise they are "in the back" of the camera)
static bool ConvertTo2d(const float x, const float y, const float z,
                        int& out_x, int& out_y)
{
    // the position to convert
    const float afPos[4] = { x, y, z, 1.0f };
    float afEye[4], afNdc[4];
    
    // Simulate the OpenGL transformation to get screen coordinates.
    mult_matrix_vec(afEye, gMatrixWrld, afPos);
    mult_matrix_vec(afNdc, gMatrixProj, afEye);
    
    afNdc[3] = 1.0f / afNdc[3];
    afNdc[0] *= afNdc[3];
    afNdc[1] *= afNdc[3];
    afNdc[2] *= afNdc[3];
    
    out_x = (int)std::lround(gScreenW * (afNdc[0] * 0.5f + 0.5f));
    out_y = (int)std::lround(gScreenH * (afNdc[1] * 0.5f + 0.5f));
    
    // afNdc[2] is basically the Z value
    if (glob.UsingModernGraphicsDriver())
        // Vulkan z-axis NDC is [0,1]
        return 0.0f <= afNdc[2] && afNdc[2] <= 1.0f;
    else
        // OGL z-axis is [-1,1]
        return -1.0f <= afNdc[2] && afNdc[2] <= 1.0;
}

//
// MARK: Drawing Control
//

/// @brief Write the labels of all aircraft
/// @see This code bases on the last part of `XPMPDefaultPlaneRenderer` of the original libxplanemp
/// @author Ben Supnik, Chris Serio, Chris Collins, Birger Hoppe
void TwoDDrawLabels ()
{
    XPLMCameraPosition_t posCamera;
    
    // short-cut if label-writing is completely switched off
    if (!glob.bDrawLabels || glob.eLabelOverride == SWITCH_CFG_OFF) return;
    
    // Set up required matrices once
    read_matrices();
    
    // Determine the maximum distance for label drawing.
    // Depends on current actual visibility as well as a configurable maximum
    XPLMReadCameraPosition(&posCamera);
    const float maxLabelDist = (std::min(glob.maxLabelDist,
                                         (glob.bLabelCutOffAtVisibility && drVisibility) ? XPLMGetDataf(drVisibility) : glob.maxLabelDist)
                                * posCamera.zoom);    // Labels get easier to see when users zooms.
    
    // Loop over all aircraft and draw their labels
    for (auto& p: glob.mapAc)
    {
        Aircraft& ac = *p.second;
        try {
            // skip if a/c is not rendered or label not to be drawn
            if (!ac.IsRendered() ||
                !(ac.ShallDrawLabel() || glob.eLabelOverride == SWITCH_CFG_ON))
                continue;
        
            // Exit if aircraft is father away from camera than we would draw labels for
            if (ac.GetCameraDist() > maxLabelDist)
                continue;
            
            // Vertical label offset: Idea is to place the label _above_ the plane
            // (as opposed to across), but finding the exact height of the plane
            // would require scanning the .obj file (well...we do so in CSLObj::FetchVertOfsFromObjFile (), but don't want to scan _every_ file)
            // We just use 3 fixed offset depending on the wake-turbulence category
            float vertLabelOfs = 7.0f;
            const XPMP2::CSLModel* pCSLMdl = ac.GetModel();
            if (pCSLMdl) {              // there's no reason why there shouldn't be a CSL model...just to be safe, though
                switch (pCSLMdl->GetDoc8643().wtc[0])
                {
                    case 'L': vertLabelOfs = 3.0f; break;
                    case 'H': vertLabelOfs = 8.0f; break;
                }
            }
        
            // Map the 3D coordinates of the aircraft to 2D coordinates of the flat screen
            int x = -1, y = -1;
            if (!ConvertTo2d(ac.drawInfo.x,
                             ac.drawInfo.y + vertLabelOfs,  // make the label appear above the plane
                             ac.drawInfo.z, x, y))
                continue;                           // label not visible

            // Determine text color:
            // It stays as defined by application for half the way to maxLabelDist.
            // For the other half, it gradually fades to gray.
            // `rat` determines how much it faded already (factor from 0..1)
            const float rat =
            ac.GetCameraDist() < maxLabelDist*0.8f ? 0.0f :                 // first 80%: no fading
            (ac.GetCameraDist() - maxLabelDist*0.8f) / (maxLabelDist*0.2f); // last  20%: fade to gray (remember: acDist <= maxLabelDist!)
            constexpr float gray[4] = {0.6f, 0.6f, 0.6f, 1.0f};
            float c[4] = {
                (1.0f-rat) * ac.colLabel[0] + rat * gray[0],     // red
                (1.0f-rat) * ac.colLabel[1] + rat * gray[1],     // green
                (1.0f-rat) * ac.colLabel[2] + rat * gray[2],     // blue
                (1.0f-rat) * ac.colLabel[3] + rat * gray[3]      // alpha? (not used for text anyway)
            };
        
            // Finally: Draw the label
            XPLMDrawString(c, x, y, (char*)ac.label.c_str(), NULL, xplmFont_Basic);
        }
        CATCH_AC(ac)
    }
}


/// Drawing callback, called by X-Plane in every drawing cycle
int CPLabelDrawing (XPLMDrawingPhase     /*inPhase*/,
                    int                  /*inIsBefore*/,
                    void *               /*inRefcon*/)
{
    // Library entry point, catch all exceptions
    try {
        TwoDDrawLabels();
    }
    catch(const std::exception& e) {
        LOG_MSG(logERR, "Exception caught: %s", e.what());
    }
    catch (...) {
        LOG_MSG(logERR, "Unknown Exception caught");
    }

    return 1;
}


/// Activate actual label drawing, esp. set up drawing callback
void TwoDActivate ()
{
    // Register the actual drawing func.
    // This actually is a deprecated call, but it is at the same time the recommended way to draw labels,
    // see https://developer.x-plane.com/code-sample/coachmarks/
    XPLMRegisterDrawCallback(CPLabelDrawing,
                             xplm_Phase_Window,
                             1,                        // after
                             nullptr);
}


/// Deactivate actual label drawing, esp. stop drawing callback
void TwoDDeactivate ()
{
    // Unregister the drawing callback
    // This actually is a deprecated call, but it is at the same time the recommended way to draw labels,
    // see https://developer.x-plane.com/code-sample/coachmarks/
    XPLMUnregisterDrawCallback(CPLabelDrawing, xplm_Phase_Window, 1, nullptr);
}


// Initialize the module
void TwoDInit ()
{
    // initialize dataRef handles:
    drMatrixWrld   = XPLMFindDataRef("sim/graphics/view/world_matrix");
    drMatrixProj   = XPLMFindDataRef("sim/graphics/view/projection_matrix_3d");
    drScreenWidth  = XPLMFindDataRef("sim/graphics/view/window_width");
    drScreenHeight = XPLMFindDataRef("sim/graphics/view/window_height");
    drVisibility   = XPLMFindDataRef("sim/graphics/view/visibility_effective_m");
    if (!drVisibility)
        drVisibility    = XPLMFindDataRef("sim/weather/visibility_effective_m");
    drFieldOfView  = XPLMFindDataRef("sim/graphics/view/field_of_view_deg");
    
    // Register the drawing callback if need be
    if (glob.bDrawLabels)
        TwoDActivate();
}

// Grace cleanup
void TwoDCleanup ()
{
    // Remove drawing callbacks
    TwoDDeactivate();
}


}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Enable/Disable/Query drawing of labels
void XPMPEnableAircraftLabels (bool _enable)
{
    // Label drawing overriden in global config?
    switch (glob.eLabelOverride) {
        case XPMP2::SWITCH_CFG_ON:
            LOG_MSG(logDEBUG, "Label drawing enforced ON in an XPMP2.prf config file");
            _enable = true;
            break;
        case XPMP2::SWITCH_CFG_OFF:
            LOG_MSG(logDEBUG, "Label drawing enforced OFF in an XPMP2.prf config file");
            _enable = false;
            break;
        case XPMP2::SWITCH_CFG_AUTO:
            break;
    }
    
    // Only do anything if this actually is a change to prevent log spamming
    if (glob.bDrawLabels != _enable) {
        LOG_MSG(logDEBUG, DEBUG_ENABLE_AC_LABELS, _enable ? "enabled" : "disabled");
        glob.bDrawLabels = _enable;
        
        // Start/stop drawing as requested
        if (glob.bDrawLabels)
            TwoDActivate();
        else
            TwoDDeactivate();
    }
}

void XPMPDisableAircraftLabels()
{
    XPMPEnableAircraftLabels(false);
}

bool XPMPDrawingAircraftLabels()
{
    return glob.bDrawLabels;
}

// Configure maximum label distance and if labels shall be cut off at reported visibility
void XPMPSetAircraftLabelDist (float _dist_nm, bool _bCutOffAtVisibility)
{
    glob.bLabelCutOffAtVisibility = _bCutOffAtVisibility;
    glob.maxLabelDist = std::max(_dist_nm,1.0f) * M_per_NM; // store in meter
}


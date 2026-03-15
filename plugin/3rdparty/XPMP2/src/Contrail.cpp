/// @file       Contrail.cpp
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

#include "XPMP2.h"

namespace XPMP2 {

#define DR_NAME_THRUST      "libxplanemp/controls/thrust_ratio"
#define DR_NAME_LIFETIME    "libxplanemp/contrail/lifetime"

//
// MARK: Module level
//

// provided in Aircraft.cpp
float obj_get_float(void * /*refcon*/);
int obj_get_float_array(void *               /*refcon*/,
                        float *              ,
                        int                  /*inOffset*/,
                        int                  inCount);

/// The one global Contrail object
XPLMObjectRef ghContrailObj = nullptr;

/// The dataRefs we pass to the contrail object is a very reduced set
static std::array<const char*, 3> DR_CONTRAIL = {
    DR_NAME_THRUST,
    DR_NAME_LIFETIME,
    // always last, marks the end in the call to XPLMCreateInstace:
    nullptr
};

/// The handle for the registered Contrail LifeTime dataRef
XPLMDataRef ghDrContrailLifeTime = nullptr;

/// @brief Initialize the Contrail module and load the Contrail object
/// @details Has a quick exit if already initialized or has already failed to initialize
/// @returns Successfully initialized?
bool ContrailInit ();

//
// MARK: Aircraft class member functions
//

// Trigger contrails as per plane type
unsigned Aircraft::ContrailTrigger ()
{
    // Need CSL model info to proceed
    if (!pCSLMdl) return 0;
    // Only for jets
    if (pCSLMdl->GetClassEngType() != 'J') {
        if (contrailNum > 0) ContrailRemove();      // remove if there are contrails at the moment
        return 0;
    }
    // Request contrails with global configuration parameters
    ContrailRequest(glob.contrailMulti ? unsigned(pCSLMdl->GetNumEngines()) : 1);
    return contrailNum;
}


// Request Contrails
void Aircraft::ContrailRequest (unsigned num, unsigned dist_m, unsigned lifeTime)
{
    contrailNum = num;
    if (dist_m)     contrailDist_m      = dist_m;
    if (lifeTime)   contrailLifeTime    = lifeTime;
}


// Remove all contrail objects
void Aircraft::ContrailRemove ()
{
    // Only allowed in XP's main thread
    if (!glob.IsXPThread()) return;
    
    // Remove contrail instances
    if (!listContrail.empty()) {
        while (!listContrail.empty()) {
            XPLMDestroyInstance(listContrail.back());
            listContrail.pop_back();
        }
        LOG_MSG(logDEBUG, "Aircraft 0x%06X: Contrails removed", modeS_id);
    }
    
    // In case of automatic contrail creation we remove the number of contrails, too,
    // will automatically be re-created when necessary.
    // If it was set manually then we don't touch what the plugin has set
    if (XPMPContrailsAutoEnabled())
        contrailNum = 0;
}

// Internal: Create/move contrails
void Aircraft::ContrailMove ()
{
    // Contrail module initialization latest now if needed
    if ((contrailNum > 0) && !ContrailInit()) return;
    
    // --- Have the correct number of contrail objects ready ---
    if (size_t(contrailNum) != listContrail.size()) {
        
        // increase number of objects as necessary
        while (listContrail.size() < size_t(contrailNum)) {
            // Create a (new) instance of the contrail object,
            // registering all the dataRef names we support
            XPLMInstanceRef hInst = XPLMCreateInstance (ghContrailObj, DR_CONTRAIL.data());
            
            // Didn't work???
            if (!hInst) {
                LOG_MSG(logERR, "Aircraft 0x%06X: Create Instance FAILED for Contrail object",
                        modeS_id);
                return;
            }

            // Save the instance
            listContrail.push_back(hInst);
        }
        
        // reduce number of objects as necessary
        while (listContrail.size() > size_t(contrailNum)) {
            XPLMDestroyInstance(listContrail.back());
            listContrail.pop_back();
        }
        
        LOG_MSG(logDEBUG, "Aircraft 0x%06X now has %u contrail object(s)", modeS_id,
                unsigned(listContrail.size()));
    }
    
    // If no contrails left, then there's nothing to position
    if (listContrail.empty()) return;
    
    // --- move all contrail objects in position ---
    
    // The data array to be passed as dataRef values
    std::array<float, DR_CONTRAIL.size()> afDrVal = {
        GetThrustRatio(),               // libxplanemp/controls/thrust_ratio
        float(contrailLifeTime),        // libxplanemp/contrail/lifetime
        0.0f                            // probably not needed ;-)
    };
    
    // Iterator into the list of contrail objects
    auto iterContrObj = listContrail.begin();
    
    // In case of an odd number of contrails we need a center object,
    // placed at the plane's location directly
    if (listContrail.size() % 2) {
        XPLMInstanceSetPosition(*(iterContrObj++), &drawInfo, afDrVal.data());
    }
    
    // Exit early if done
    if (iterContrObj == listContrail.end()) return;
    
    // Calculate the vector to which the contrails need to be displaced
    // (+90 degree is pointing right along the wing)
    std::valarray<float> vDisplace = HeadPitch2Vec(GetHeading() + 90.0f, GetPitch());
    vDisplace *= float(contrailDist_m);
    // Positions along the right/left wing
    XPLMDrawInfo_t RPos = drawInfo, LPos = drawInfo;

    // All other objects now come in pairs, each `contrailDist_m` distance from before
    while (iterContrObj != listContrail.end()) {
        // Starboard
        RPos.x += vDisplace[0];
        RPos.y += vDisplace[1];
        RPos.z += vDisplace[2];
        XPLMInstanceSetPosition(*(iterContrObj++), &RPos, afDrVal.data());

        // Port
        assert(iterContrObj != listContrail.end());
        LPos.x -= vDisplace[0];
        LPos.y -= vDisplace[1];
        LPos.z -= vDisplace[2];
        XPLMInstanceSetPosition(*(iterContrObj++), &LPos, afDrVal.data());
    }
}


// Internal: (Re)Assess if contrails are to be created
void Aircraft::ContrailAutoUpdate ()
{
    // --- Auto-create/remove contrails? ---
    if (XPMPContrailsAutoEnabled() && !std::isnan(alt1s_ft)) {
        const bool bInContrailAlt = glob.contrailAltMin_ft <= alt1s_ft && alt1s_ft <= glob.contrailAltMax_ft;
        if (bInContrailAlt && !contrailNum)             // in contrail altitude but have no contrails?
            ContrailTrigger();
        else if (!bInContrailAlt && contrailNum > 0)    // not in contrail altitiude but have contrails?
            ContrailRemove();
    }
}

//
// MARK: Internal Module-level Functions
//


// Initialize the Contrail module and load the Contrail object
bool ContrailInit ()
{
    static bool bFailedAlready = false;
    
    // Don't do twice
    if (ghContrailObj) return true;
    if (bFailedAlready) return false;
    
    // Register the dataRef for the contrail LifeTime
    if (!ghDrContrailLifeTime) {
        ghDrContrailLifeTime =
        XPLMRegisterDataAccessor(DR_NAME_LIFETIME,
                                 xplmType_Float, 0,
                                 nullptr, nullptr,
                                 obj_get_float, nullptr,
                                 nullptr, nullptr,
                                 nullptr, nullptr,
                                 obj_get_float_array, nullptr,
                                 nullptr, nullptr,
                                 nullptr, nullptr);
    }
    
    // path to Contrail object
    std::string path = glob.resourceDir;
    path += "Contrail";
    path += PATH_DELIM_STD;
    path += "Contrail.obj";
    if (!ExistsFile(path)) {
        LOG_MSG(logWARN, "Contrails unavailable: Could not find %s",
                path.c_str());
        bFailedAlready = true;
        return false;
    }
    
    // Load the contrail object
    // (that's _very_ small one, and we are in init phase,
    //  so we just quickly do it synchronously right here and now)
    ghContrailObj = XPLMLoadObject(path.c_str());
    if (!ghContrailObj) {
        LOG_MSG(logWARN, "Contrails unavailable: Failed to load %s",
                path.c_str());
        bFailedAlready = true;
        return false;
    }
    
    LOG_MSG(logDEBUG, "Contrails available, have loaded %s",
            path.c_str());
    
    return true;
}

// Graceful shutdown
void ContrailCleanup ()
{
    if (ghContrailObj) {
        XPLMUnloadObject(ghContrailObj);
        ghContrailObj = nullptr;
    }
    if (ghDrContrailLifeTime) {
        XPLMUnregisterDataAccessor(ghDrContrailLifeTime);
        ghDrContrailLifeTime = nullptr;
    }
}


} // namespace XPMP2

//
// MARK: Public Global Functions
//

// Are Contrails enabled?
bool XPMPContrailsAutoEnabled ()
{
    // Implicitely, this also test for one of them not being zero
    return XPMP2::glob.contrailAltMax_ft > XPMP2::glob.contrailAltMin_ft;
}

// Are Contrails available?
bool XPMPContrailsAvailable ()
{
    return XPMP2::ghContrailObj != nullptr;
}

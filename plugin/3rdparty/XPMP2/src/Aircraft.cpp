/// @file       Aircraft.cpp
/// @brief      XPMP2::Aircraft represents an aircraft as managed by XPMP2
/// @note       This class bases on and is compatible to the XPCAircraft wrapper
///             class provided with the original libxplanemp.
///             In XPMP2, however, this class is not a wrapper but the actual
///             means of managing aircraft. Hence, it includes a lot more members.
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

#include "XPMP2.h"

using namespace XPMP2;

//
// MARK: Globals
//

#define WARN_MODEL_NOT_FOUND    "Named CSL Model '%s' not found"
#define FATAL_MODE_S_OUT_OF_RGE "_modeS_id (0x%06X) is out of range [0x%06X..0x%06X]"
#define FATAL_MODE_S_EXISTS     "_modeS_id (0x%06X) already exists"
#define FATAL_CREATE_INVALID    "Called Aircraft::Create() on already defined plane with _modeS_id (0x%06X)"
#define DEBUG_REPL_MODE_S       "Replaced duplicate _modeS_id 0x%06X with new unique value 0x%06X"
#define ERR_CREATE_INSTANCE     "Aircraft 0x%06X: Create Instance FAILED for CSL Model %s"
#define DEBUG_INSTANCE_CREATED  "Aircraft 0x%06X: Instance created of model %s for '%s'"
#define DEBUG_INSTANCE_DESTRYD  "Aircraft 0x%06X: Instance destroyed"
#define INFO_MODEL_CHANGE       "Aircraft 0x%06X: Changing model from %s to %s"
#define ERR_YPROBE              "Aircraft 0x%06X: Could not create Y-Probe for terrain testing!"
#define ERR_SET_INVALID         "Aircraft 0x%06X set INVALID"
#define WARN_PLANES_LEFT_EXIT   "Still %lu aircaft defined during shutdown! Plugin should destroy them prior to shutting down."
#define ERR_ADD_DATAREF_INIT    "Could not add dataRef %s, XPMP2 not yet initialized?"
#define ERR_ADD_DATAREF_PLANES  "Could add dataRef %s only if no aircraft are flying, but currently there are %lu aircraft."
#define ERR_WORKER_THREAD       "Can only be called from XP's main thread!"
#define DEBUG_DATAREF_ADDED     "Added dataRef %s as index %lu"

namespace XPMP2 {

/// The id of our flight loop callback
XPLMFlightLoopID gFlightLoopID = nullptr;

/// @brief The list of dataRefs we support to be read by the CSL Model (for gear, flaps, lights etc.)
/// @details Can be extended by the user
static std::vector<const char*> DR_NAMES = {
    "libxplanemp/controls/gear_ratio",
    "libxplanemp/controls/nws_ratio",
    "libxplanemp/controls/flap_ratio",
    "libxplanemp/controls/spoiler_ratio",
    "libxplanemp/controls/speed_brake_ratio",
    "libxplanemp/controls/slat_ratio",
    "libxplanemp/controls/wing_sweep_ratio",
    "libxplanemp/controls/thrust_ratio",
    "libxplanemp/controls/yoke_pitch_ratio",
    "libxplanemp/controls/yoke_heading_ratio",
    "libxplanemp/controls/yoke_roll_ratio",
    "libxplanemp/controls/thrust_revers",
    
    "libxplanemp/controls/taxi_lites_on",
    "libxplanemp/controls/landing_lites_on",
    "libxplanemp/controls/beacon_lites_on",
    "libxplanemp/controls/strobe_lites_on",
    "libxplanemp/controls/nav_lites_on",
    
    "libxplanemp/gear/nose_gear_deflection_mtr",
    "libxplanemp/gear/tire_vertical_deflection_mtr",
    "libxplanemp/gear/tire_rotation_angle_deg",
    "libxplanemp/gear/tire_rotation_speed_rpm",
    "libxplanemp/gear/tire_rotation_speed_rad_sec",
    
    "libxplanemp/engines/engine_rotation_angle_deg",
    "libxplanemp/engines/engine_rotation_speed_rpm",
    "libxplanemp/engines/engine_rotation_speed_rad_sec",       // PE defines this: https://www.pilotedge.net/pages/csl-authoring
    "libxplanemp/engines/prop_rotation_angle_deg",
    "libxplanemp/engines/prop_rotation_speed_rpm",
    "libxplanemp/engines/prop_rotation_speed_rad_sec",
    "libxplanemp/engines/thrust_reverser_deploy_ratio",
    
    "libxplanemp/engines/engine_rotation_angle_deg1",       // support for individual control of different engines
    "libxplanemp/engines/engine_rotation_angle_deg2",
    "libxplanemp/engines/engine_rotation_angle_deg3",
    "libxplanemp/engines/engine_rotation_angle_deg4",
    "libxplanemp/engines/engine_rotation_speed_rpm1",
    "libxplanemp/engines/engine_rotation_speed_rpm2",
    "libxplanemp/engines/engine_rotation_speed_rpm3",
    "libxplanemp/engines/engine_rotation_speed_rpm4",
    "libxplanemp/engines/engine_rotation_speed_rad_sec1",
    "libxplanemp/engines/engine_rotation_speed_rad_sec2",
    "libxplanemp/engines/engine_rotation_speed_rad_sec3",
    "libxplanemp/engines/engine_rotation_speed_rad_sec4",

    "libxplanemp/misc/touch_down",
    
    // always last, marks the end in the call to XPLMCreateInstace:
    nullptr
};

/// Here we store the dataRef strings as smart pointers,
/// so that the pointed-to location will not change but we can easily manage them
std::vector<std::unique_ptr<std::string> > drStrings;

/// Registered dataRefs
std::vector<XPLMDataRef> ahDataRefs;

/// Standard name for "no model"
static std::string noMdlName("<none>");

//
// MARK: Wake Support
//

typedef std::map<std::string, Aircraft::wakeTy> mapWakeTy;

/// @brief Mapping table from Wake Turbulence Category (WTC) to default values for XPMP2::Aircraft::wake
/// @detail Filled with popular example aircraft of the category.
///         Mass is filled with `(Empty Weight + 80% * (MTOW - Empty Weight))`
static mapWakeTy mapWake = {
    // C172: http://www.flugzeuginfo.net/acdata_php/acdata_cessna172_en.php
    { "L",      { 11.00f,  16.2f,   1037.6f }},
    // B350: http://www.flugzeuginfo.net/acdata_php/acdata_beech350_en.php
    { "L/M",    { 17.65f,  28.8f,   6301.0f }},
    // A320: http://www.flugzeuginfo.net/acdata_php/acdata_a320_en.php
    { "M",      { 34.09f, 122.6f,  74500.0f }},
    // B744: http://www.flugzeuginfo.net/acdata_php/acdata_7474_en.php
    { "H",      { 64.40f, 541.2f, 367129.4f }},
    // A388: http://www.flugzeuginfo.net/acdata_php/acdata_a380_en.php
    { "J",      { 79.80f, 845.0f, 510560.0f }},
};

// any value left at `NAN`, ie. requires setting from Doc8643 WTC defaults?
bool Aircraft::wakeTy::needsDefaults() const
{
    return
        std::isnan(wingSpan_m) ||
        std::isnan(wingArea_m2) ||
        std::isnan(mass_kg);
}

// based on Doc8643 WTC fill with defaults
void Aircraft::wakeTy::applyDefaults(const std::string& _wtc, bool _bOverwriteAllFields)
{
    // Force complete refresh?
    if (_bOverwriteAllFields)
        clear();
    // or in the contrary: don't need to set anything?
    else if (!needsDefaults())
        return;

    // Try to find given WTC in our mapping table
    LOG_ASSERT(!mapWake.empty());                       // there's no code to remove from mapWake, so this _should_ always be true
    mapWakeTy::const_iterator i = mapWake.find(_wtc);
    // If not found, look for "M"
    if (i == mapWake.cend())
        i = mapWake.find("M");
    // If not found (???) just take the first one (we verified mapWake is not empty, so there's something in there)
    if (i == mapWake.cend())
        i = mapWake.cbegin();
    LOG_ASSERT(i != mapWake.cend());

    // Copy over missing values
    fillUpFrom(i->second);
}

// Copies values only for non-NAN fields
void Aircraft::wakeTy::fillUpFrom(const wakeTy& o)
{
    if (std::isnan(wingSpan_m))     wingSpan_m  = o.wingSpan_m;
    if (std::isnan(wingArea_m2))    wingArea_m2 = o.wingArea_m2;
    if (std::isnan(mass_kg))        mass_kg     = o.mass_kg;
}

//
// MARK: Aircraft
//

// Constructor creates a new aircraft object, which will be managed and displayed
Aircraft::Aircraft(const std::string& _icaoType,
                   const std::string& _icaoAirline,
                   const std::string& _livery,
                   XPMPPlaneID _modeS_id,
                   const std::string& _cslId) :
drawInfo({sizeof(drawInfo), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
// create an approrpiately sized 'v' array and initialize with zeroes
v(DR_NAMES.size(), 0.0f)
{
    // Create the plane right away
    Create(_icaoType, _icaoAirline, _livery, _modeS_id, _cslId);
}

// Default constructor creates an empty, invalid(!) and invisible shell; call XPMP2::Aircraft::Create() to actually create a plane
Aircraft::Aircraft () :
drawInfo({sizeof(drawInfo), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
// create an approrpiately sized 'v' array and initialize with zeroes
v(DR_NAMES.size(), 0.0f),
bValid(false), bVisible(false)      // Invalid and invisible for the time being!
{}

// Destructor cleans up all resources acquired
Aircraft::~Aircraft ()
{
    // inform observers and the network
    XPMPSendNotification(*this, xpmp_PlaneNotification_Destroyed);
    RemoteAcRemove(*this);
    
    // Remove the instance
    DestroyInstances();
    
    // Decrease the reference counter of the CSL model
    if (pCSLMdl)
        pCSLMdl->DecRefCnt();

    // remove myself from the global map of planes
    glob.mapAc.erase(modeS_id);
    
    // remove the Y Probe
    if (hProbe) {
        XPLMDestroyProbe(hProbe);
        hProbe = nullptr;
    }
}


// Creates a plane, only a valid operation if object was created using the default constructor
void Aircraft::Create (const std::string& _icaoType,
                       const std::string& _icaoAirline,
                       const std::string& _livery,
                       XPMPPlaneID _modeS_id,
                       const std::string& _cslId,
                       CSLModel* _pCSLModel)
{
    // Must be called from XP's main thread only as we are calling XPLM SDK functions!!
    LOG_ASSERT(glob.IsXPThread());
    
    // Must not be used on already defined aircraft
    if (modeS_id > 0) {
        THROW_ERROR(FATAL_CREATE_INVALID, modeS_id);
    }
    
    // assign the next synthetic plane id
    modeS_id = _modeS_id ? _modeS_id : glob.NextPlaneId();

    // Verify uniqueness of modeS if defined by caller
    if (_modeS_id) {
        if (_modeS_id < MIN_MODE_S_ID || _modeS_id > MAX_MODE_S_ID) {
            THROW_ERROR(FATAL_MODE_S_OUT_OF_RGE,
                        _modeS_id, MIN_MODE_S_ID, MAX_MODE_S_ID);
        }
        if (glob.mapAc.count(_modeS_id) != 0)       // _modeS_id already exists
        {
            // we shall assign a new unique id?
            if (glob.bHandleDupId)
            {
                modeS_id = glob.NextPlaneId();
                LOG_MSG(logDEBUG, DEBUG_REPL_MODE_S, _modeS_id, modeS_id);
            } else {
                // throw exception
                THROW_ERROR(FATAL_MODE_S_EXISTS, _modeS_id);
            }
        }
    }
    
    // Now valid and to be displayed
    bValid = bVisible = true;
    
    // if given try to find the CSL model to use by its name, or just use the given model
    if (!_cslId.empty() || _pCSLModel) {
        if (AssignModel(_cslId, _pCSLModel)) {
            // however, remember the passed-in type details if given
            if (!_icaoType.empty())     acIcaoType = _icaoType;
            if (!_icaoAirline.empty())  acIcaoAirline = _icaoAirline;
            if (!_livery.empty())       acLivery = _livery;
        }
    }
    
    // Let Matching happen, if we still don't have a model
    if (!pCSLMdl)
        ChangeModel(_icaoType, _icaoAirline, _livery);
    LOG_ASSERT(pCSLMdl);
    
    // Setup sound for this aircraft
    SoundSetup();

    // Initialize contrail life time from global config
    contrailLifeTime = unsigned(glob.contrailLifeTime);

    // add the aircraft to our global map and inform observers
    glob.mapAc.emplace(modeS_id,this);
    XPMPSendNotification(*this, xpmp_PlaneNotification_Created);
    
    // make sure the flight loop callback gets called if this was the first a/c
    if (glob.mapAc.size() == 1) {
        // Create the flight loop callback (unscheduled) if not yet there
        if (!gFlightLoopID) {
            XPLMCreateFlightLoop_t cfl = {
                sizeof(XPLMCreateFlightLoop_t),                 // size
                xplm_FlightLoop_Phase_BeforeFlightModel,        // phase
                FlightLoopCB,                                   // callback function
                (void*)xplm_FlightLoop_Phase_BeforeFlightModel  // refcon
            };
            gFlightLoopID = XPLMCreateFlightLoop(&cfl);
            
            // Enforce TCAS/AI Control if global config says so
            if (glob.eAIOverride == SWITCH_CFG_ON && !XPMPHasControlOfAIAircraft()) {
                LOG_MSG(logDEBUG, "TCAS/AI Control enforced ON in an XPMP2.prf config file");
                const char* sz = XPMPMultiplayerEnable();
                if (sz && sz[0]) {
                    LOG_MSG(logWARN, "Forced enabling of TCAS/AI failed: %s", sz);
                }
            }
        }
        
        // Schedule the flight loop callback to be called next flight loop cycle
        XPLMScheduleFlightLoop(gFlightLoopID, -1.0f, 0);
        LOG_MSG(logDEBUG, "Flight loop callback started");
    }
}


// Is this object a ground vehicle?
bool Aircraft::IsGroundVehicle() const
{
    return IsRelatedTo(glob.carIcaoType);
}


// Is this object "related" to the given ICAO code? (named in the same line in related.txt)
bool Aircraft::IsRelatedTo(const std::string& _icaoType) const
{
    if (acIcaoType == _icaoType)                    // exactly equal types
        return true;
    if (!acRelGrp)                                  // this a/c is not in any related group
        return false;
    // compare related group to passed-in type
    return acRelGrp == RelatedGet(REL_TXT_DESIGNATOR, _icaoType);
}


// Return a value for dataRef .../tcas/target/flight_id
std::string XPMP2::Aircraft::GetFlightId() const
{
    // Flight number available?
    if (acInfoTexts.flightNum[0])
        return acInfoTexts.flightNum;
    // Registration (tail number) available?
    else if (acInfoTexts.tailNum[0])
        return acInfoTexts.tailNum;
    // Departure or destination airport available?
    else if (acInfoTexts.aptFrom[0] || acInfoTexts.aptTo[0]) {
        std::string ret(acInfoTexts.aptFrom);
        ret += '-';
        ret += acInfoTexts.aptTo;
    }

    // nothing found
    return "";
}


// (Potentially) change the plane's model after doing a new match attempt
int Aircraft::ChangeModel (const std::string& _icaoType,
                           const std::string& _icaoAirline,
                           const std::string& _livery)
{
    // Let matching happen
    CSLModel* pMdl = nullptr;
    int q = CSLModelMatching(_icaoType,
                             _icaoAirline,
                             _livery,
                             pMdl);

    // Is this a change to the currently used model?
    const bool bChangeExisting = (pCSLMdl && pMdl != pCSLMdl);
    if (bChangeExisting) {
        // remove the current instance (which is based on the previous model)
        LOG_MSG(logINFO, INFO_MODEL_CHANGE,
                modeS_id,
                pCSLMdl->GetModelName().c_str(),
                pMdl ? pMdl->GetModelName().c_str() : noMdlName.c_str());
        DestroyInstances();
    }
    // Decrease the reference counter of the current CSL model
    if (pCSLMdl)
        pCSLMdl->DecRefCnt();

    // save the newly selected model
    pCSLMdl         = pMdl;             // could theoretically be nullptr!
    matchQuality    = q;
    acIcaoType      = _icaoType;
    acIcaoAirline   = _icaoAirline;
    acLivery        = _livery;
    acRelGrp        = RelatedGet(REL_TXT_DESIGNATOR, acIcaoType);

    // Increase the reference counter of the CSL model to track that the object is being used
    if (pCSLMdl)
        pCSLMdl->IncRefCnt();

    // Determin map icon based on icao type
    MapFindIcon();

    // (Re)set wake support data
    WakeApplyDefaults(bChangeExisting);
    
    // inform observers in case this was an actual replacement change
    if (bChangeExisting) {
        SoundSetup();
        XPMPSendNotification(*this, xpmp_PlaneNotification_ModelChanged);
    }

    return q;
}


// Assigns the given model per name, returns if successful
bool Aircraft::AssignModel (const std::string& _cslId,
                            CSLModel* _pCSLModel)
{
    // set the model, or try finding the model by name
    CSLModel* pMdl = _pCSLModel ? _pCSLModel : CSLModelById(_cslId);
    if (!pMdl) {                            // nothing changes if not found
        LOG_MSG(logWARN, WARN_MODEL_NOT_FOUND, _cslId.c_str());
        return false;
    }

    // Is this a change to the currently used model?
    const bool bChangeExisting = (pCSLMdl && pMdl != pCSLMdl);
    if (bChangeExisting) {
        LOG_MSG(logINFO, INFO_MODEL_CHANGE,
                modeS_id,
                pCSLMdl->GetModelName().c_str(),
                pMdl->GetModelName().c_str());
        DestroyInstances();                 // remove the current instance (which is based on the previous model)
    }
    // Decrease the reference counter of the current CSL model
    if (pCSLMdl)
        pCSLMdl->DecRefCnt();
    
    // save the newly selected model
    pCSLMdl         = pMdl;
    matchQuality    = 0;
    acIcaoType      = pCSLMdl->GetIcaoType();
    acIcaoAirline   = pCSLMdl->GetIcaoAirline();
    acLivery        = pCSLMdl->GetLivery();
    acRelGrp        = RelatedGet(REL_TXT_DESIGNATOR, acIcaoType);

    // Increase the reference counter of the CSL model to track that the object is being used
    pCSLMdl->IncRefCnt();

    // Determin map icon based on icao type
    MapFindIcon();
    
    // (Re)set wake support data
    WakeApplyDefaults(bChangeExisting);

    // inform observers
    if (bChangeExisting)
        XPMPSendNotification(*this, xpmp_PlaneNotification_ModelChanged);

    return true;
}


// Fill in default wake turbulence support data based on Doc8643 wake turbulence category
void Aircraft::WakeApplyDefaults(bool _bOverwriteAllFields)
{
    // Determine the aircraft's WTC
    wake.applyDefaults(pCSLMdl ? pCSLMdl->GetWTC() : "", _bOverwriteAllFields);
}


// return the name of the CSL model in use
const std::string& Aircraft::GetModelName () const
{
    return pCSLMdl ? pCSLMdl->GetModelName() : noMdlName;
}


// Vertical offset, ie. the value that needs to be added to drawInfo.y to make the aircraft appear on the ground
/// @details 1. add `VERT_OFFSET`, which pushes the plane up on the tarmac onto its gear
///          2. reduce again by the tire deflection (which reduces gear's size,
///             but based on experience is not exactly aligned with planes altitude in meters.
float Aircraft::GetVertOfs () const
{
    if (pCSLMdl)
        return pCSLMdl->GetVertOfs() * vertOfsRatio - GetTireDeflection() * gearDeflectRatio;
    else
        return 0.0f;
}


// Category between 0=light and 3=Super, derived from WTC
int Aircraft::GetWakeCat() const
{
    return pCSLMdl ? pCSLMdl->GetDoc8643().GetWakeCat() : 1;
}

// Static: Flight loop callback function
float Aircraft::FlightLoopCB(float _elapsedSinceLastCall, float, int _flCounter, void*)
{
    // This is a plugin entry function, so we try to catch all exceptions
    try {
        glob.xpCycleNum=XPLMGetCycleNumber();               // Store current cycle number in glob.xpCycleNum
        const float now = UpdateCachedValuesGetNetwTime();  // As this is a main thread call, let's update some values we can get only now

        // Update configuration
        glob.UpdateCfgVals();

        // Need the camera's position to calculate the a/c's distance to it
        glob.UpdateCameraPos();

        // give remote model the chance for some prep work
        RemoteAcEnqueueStarts(now);

        // Tell Sound module that we are about to start updaing
        SoundUpdatesBegin();

        // Update positional and configurational values
        for (mapAcTy::value_type& pair : glob.mapAc) {
            Aircraft& ac = *pair.second;
            // Catch up with instance destroy
            if (ac.bDestroyInst)
                ac.DestroyInstances();
            // skip invalid aircraft
            if (!ac.IsValid())
                continue;
            try {
                // Have the aircraft provide up-to-date position and orientation values
                ac.UpdatePosition(_elapsedSinceLastCall, _flCounter);
                // A/c still valid? Then proceed:
                if (ac.IsValid()) {
                    // If requested, clamp to ground, ie. make sure it is not below ground
                    if (ac.bClampToGround || glob.bClampAll)
                        ac.ClampToGround();
                    // Do some expensive stuff every second only
                    ac.DoEverySecondUpdates(now);
                    // If required reset touch down animation
                    if (!std::isnan(ac.tsResetTouchDown) && (now >= ac.tsResetTouchDown)) {
                        ac.SetTouchDown(false);
                        ac.tsResetTouchDown = NAN;
                    }
                    // Actually move the plane, ie. the instance that represents it
                    ac.DoMove();
                    // Feed remote connections
                    RemoteAcEnqueue(ac);
                }
            }
            CATCH_AC(ac)
        }
        
        // Tell remote module that we are done updated a/c so it can send out last pending messages
        RemoteAcEnqueueDone();
        
        // Tell Sound module that we are done updating
        SoundUpdatesDone();
        
        // Publish aircraft data on the AI/multiplayer dataRefs
        AIMultiUpdate();
    }
    catch (const std::exception& e) {
        LOG_MSG(logFATAL, ERR_EXCEPTION, e.what());
        RemoteAcEnqueueDone();          // must make sure to release a lock
        SoundUpdatesDone();
    }

    // Don't call me again if there are no more aircraft,
    if (glob.mapAc.empty()) {
        RemoteAcClearAll ();            // remote module can clean up, too
        LOG_MSG(logDEBUG, "Flight loop callback ended");
        return 0.0f;
    }
    else {
        // call me next cycle if there are
        return -1.0f;
    }
}

// This puts the instance into XP's sky and makes it move
void Aircraft::DoMove ()
{
    // Only for planes that are to be rendered
    if (IsRendered()) {
        // Already have instances? 
        if (!listInst.empty() || CreateInstances()) {
            // Move the instances (this is probably the single most important line of code ;-) )
            for (XPLMInstanceRef hInst: listInst)
                 XPLMInstanceSetPosition(hInst, &drawInfo, v.data());
            // Move/create contrails
            ContrailMove();
            // Update Sound
            SoundUpdate();
        }
    }
}

// Processes once every second only stuff that doesn't require being computed every flight loop
void Aircraft::DoEverySecondUpdates (float now)
{
    // Update plane's distance/bearing every second only
    if (CheckEverySoOften(camTimLstUpd, 1.0f, now)) {
        GetLocation(lat1s, lon1s, alt1s_ft);        // convert position
        UpdateDistBearingCamera(glob.posCamera);
        ComputeMapLabel();
        ContrailAutoUpdate();
    }
}

// Clamp to ground: Make sure the plane is not below ground, corrects Aircraft::drawInfo if needed.
void Aircraft::ClampToGround ()
{
    // Make sure we have a probe object
    if (!hProbe)
        hProbe = XPLMCreateProbe(xplm_ProbeY);
    if (!hProbe) {
        LOG_MSG(logERR, ERR_YPROBE, modeS_id);
        bClampToGround = false;
        return;
    }
    
    // Where's the ground?
    XPLMProbeInfo_t infoProbe = {
        sizeof(XPLMProbeInfo_t),            // structSIze
        0.0f, 0.0f, 0.0f,                   // location
        0.0f, 0.0f, 0.0f,                   // normal vector
        0.0f, 0.0f, 0.0f,                   // velocity vector
        0                                   // is_wet
    };
    if (XPLMProbeTerrainXYZ(hProbe,
                            drawInfo.x, drawInfo.y, drawInfo.z,
                            &infoProbe) == xplm_ProbeHitTerrain)
    {
        // if currently the aircraft would be below ground,
        // then lift it on the ground
        infoProbe.locationY += GetVertOfs();
        if (drawInfo.y < infoProbe.locationY)
            drawInfo.y = infoProbe.locationY;
    }
}

// Internal: Update the plane's distance/bearing from the camera location
void Aircraft::UpdateDistBearingCamera (const XPLMCameraPosition_t& posCam)
{
    // distance just by Pythagoras
    camDist = dist(posCam.x,   posCam.y,   posCam.z,
                      drawInfo.x, drawInfo.y, drawInfo.z);
    // Bearing (note: x points east, z points south
    camBearing = angleLocCoord(posCam.x, posCam.z, drawInfo.x, drawInfo.z);
}


// Create the instances, return if successful
bool Aircraft::CreateInstances ()
{
    // If we have instances already we just return
    if (!listInst.empty()) return true;

    // Let's see if we can get ALL object handles:
    LOG_ASSERT(pCSLMdl);
    std::list<XPLMObjectRef> listObj = pCSLMdl->GetAllObjRefs();
    if (listObj.empty())                    // we couldn't...
        return false;
    
    // OK, we got a complete list of objects, so let's instanciate them:
    for (XPLMObjectRef hObj: listObj) {
        // Create a (new) instance of this CSL Model object,
        // registering all the dataRef names we support
        XPLMInstanceRef hInst = XPLMCreateInstance (hObj, DR_NAMES.data());
        
        // Didn't work???
        if (!hInst) {
            LOG_MSG(logERR, ERR_CREATE_INSTANCE,
                    modeS_id,
                    GetModelName().c_str());
            DestroyInstances();             // remove other instances we might have created already
            return false;
        }

        // Save the instance
        listInst.push_back(hInst);
    }
    
    // Success!
    LOG_MSG(logDEBUG, DEBUG_INSTANCE_CREATED, modeS_id, GetModelName().c_str(),
            label.c_str());
    return true;
}

// Destroy all instances
void Aircraft::DestroyInstances ()
{
    // Only allowed in XP's main thread
    if (!glob.IsXPThread()) {
        // if not in main thread then only set a flag to myself
        bDestroyInst = true;
        return;
    }
    
    // Remove all sound
    SoundRemoveAll();
    
    // Remove Contrails
    ContrailRemove();

    // Remove aircraft instances
    if (!listInst.empty()) {
        while (!listInst.empty()) {
            XPLMInstanceRef hRef = listInst.back();
            listInst.pop_back();
            XPLMDestroyInstance(hRef);
        }
        LOG_MSG(logDEBUG, DEBUG_INSTANCE_DESTRYD, modeS_id);
    }
    bDestroyInst = false;
}


// Set if the aircraft is on the ground
void Aircraft::SetOnGrnd (bool _grnd, float _fSetTouchDownTime)
{
    // Set the ground flag as stated
    bOnGrnd = _grnd;
    // If asked for set the touch down animation dataRef and remember when to reset
    if (!std::isnan(_fSetTouchDownTime)) {
        SetTouchDown(true);
        tsResetTouchDown = GetMiscNetwTime() + _fSetTouchDownTime;
    }
}


// Converts world coordinates to local coordinates, writes to `drawInfo`
void Aircraft::SetLocation(double lat, double lon, double alt_f)
{
    // Must be called from XP's main thread only as we are calling XPLM SDK functions!!
    LOG_ASSERT(glob.IsXPThread());

    // Weirdly, XPLMWorldToLocal expects points to double, while XPLMDrawInfo_t later on provides floats,
    // so we need intermediate variables
    double x, y, z;
    XPLMWorldToLocal(lat1s = lat,               // once we know the lat/lon location also store it locally
                     lon1s = lon,
                     (alt1s_ft = alt_f) * M_per_FT,
                     &x, &y, &z);
    
    // Copy to drawInfo
    drawInfo.x = float(x);
    drawInfo.y = float(y) + GetVertOfs();
    drawInfo.z = float(z);
}



// Converts aircraft's local coordinates to lat/lon values
void Aircraft::GetLocation (double& lat, double& lon, double& alt_ft) const
{
    // Must be called from XP's main thread only as we are calling XPLM SDK functions!!
    LOG_ASSERT(glob.IsXPThread());

    XPLMLocalToWorld(drawInfo.x, drawInfo.y, drawInfo.z,
                     &lat, &lon, &alt_ft);
    alt_ft /= M_per_FT;
}

// Engine rotation angle [degree]
void  Aircraft::SetEngineRotAngle (float _deg)
{
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG] =
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1] =
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG2] =
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG3] =
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG4] = _deg;
}

// Engine rotation speed [rpm], also sets [rad/s]
void  Aircraft::SetEngineRotRpm (float _rpm)
{
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM2] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM3] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM4] = _rpm;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4] = _rpm * RPM_to_RADs;
}

// Engine rotation speed [rad/s], also sets [rpm]
void  Aircraft::SetEngineRotRad (float _rad)
{
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4] = _rad;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM2] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM3] =
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM4] = _rad / RPM_to_RADs;
}

// Engine rotation angle [degree] for engine `idx` (1..4)
void  Aircraft::SetEngineRotAngle (size_t idx, float _deg)
{
    if (idx < 1 || idx > 4) return;
    v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1+idx-1] = _deg;
    // for backwards compatibility we also set the generic dataRef
    if (idx == 1)
        v[V_ENGINES_ENGINE_ROTATION_ANGLE_DEG] = _deg;
}

// Engine rotation speed [rpm] for engine `idx` (1..4), also sets [rad/s]
void  Aircraft::SetEngineRotRpm (size_t idx, float _rpm)
{
    if (idx < 1 || idx > 4) return;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1+idx-1]       = _rpm;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1+idx-1]   = _rpm * RPM_to_RADs;
    // for backwards compatibility we also set the generic dataRef
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM] = std::max({
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM2],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM3],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM4],
    });
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] = v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM] * RPM_to_RADs;
}

// Engine rotation speed [rad/s] for engine `idx` (1..4), also sets [rpm]
void  Aircraft::SetEngineRotRad (size_t idx, float _rad)
{
    if (idx < 1 || idx > 4) return;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1+idx-1]   = _rad;
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM1+idx-1]       = _rad / RPM_to_RADs;
    // for backwards compatibility we also set the generic dataRef
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] = std::max({
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3],
        v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4],
    });
    v[V_ENGINES_ENGINE_ROTATION_SPEED_RPM] = v[V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC] / RPM_to_RADs;
}

// Mark the plane invalid, e.g. after exceptions occured on the data
void Aircraft::SetInvalid()
{
    // no change?
    if (!bValid)
        return;

    // Set the flag
    bValid = false;
    LOG_MSG(logERR, ERR_SET_INVALID, modeS_id);

    // Cleanup the object as good as possible
    try {
        ResetTcasTargetIdx();
        DestroyInstances();
    }
    // We are not reporting anything here...the object is invalid, we just try our best
    catch (...) {}
}

// Make the plane (in)visible
void Aircraft::SetVisible (bool _bVisible)
{
    // no change?
    if (bVisible == _bVisible)
        return;
    
    // Set the flag
    bVisible = _bVisible;
    
    // In case of _now_ being invisible remove the instances and any AI slot
    if (!bVisible) {
        ResetTcasTargetIdx();
        DestroyInstances();
    }
}

// Switch rendering of the CSL model on or off
void Aircraft::SetRender (bool _bRender)
{
    // no change?
    if (bRender == _bRender)
        return;
    
    // Set the flag
    bRender = _bRender;
    
    // In case rendering is _now_ switched off: Remove the instance (but leave AI as is!)
    if (!bRender)
        DestroyInstances();
}

//
// MARK: LegacyAircraft
//

// Constructor accepts the very same parameters as XPMPCreatePlane() and XPMPCreatePlaneWithModelName()
LegacyAircraft::LegacyAircraft(const char*      _ICAOCode,
                               const char*      _Airline,
                               const char*      _Livery,
                               XPMPPlaneData_f  _DataFunc,
                               void *           _Refcon,
                               XPMPPlaneID      _modeS_id,
                               const char *     _ModelName) :
XPCAircraft (_ICAOCode, _Airline, _Livery, _modeS_id, _ModelName),
dataFunc (_DataFunc),
refcon (_Refcon)
{}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlanePosition(XPMPPlanePosition_t* outPosition)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(modeS_id, xpmpDataType_Position, outPosition, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlaneSurfaces(XPMPPlaneSurfaces_t* outSurfaces)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(modeS_id, xpmpDataType_Surfaces, outSurfaces, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetPlaneRadar(XPMPPlaneRadar_t* outRadar)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(modeS_id, xpmpDataType_Radar, outRadar, refcon);
}

// Just calls `dataFunc`
XPMPPlaneCallbackResult LegacyAircraft::GetInfoTexts(XPMPInfoTexts_t* outInfoTexts)
{
    if (!dataFunc) return xpmpData_Unavailable;
    return dataFunc(modeS_id, xpmpDataType_InfoTexts, outInfoTexts, refcon);
}

} // Namespace XPMP2

//
// MARK: XPCAircraft legacy
//

// Legacy constructor creates a plane and puts it under control of XPlaneMP
XPCAircraft::XPCAircraft(const char* _icaoType,
                         const char* _icaoAirline,
                         const char* _livery,
                         XPMPPlaneID _modeS_id,
                         const char* _modelId) :
Aircraft(_icaoType, _icaoAirline, _livery, _modeS_id,
         _modelId ? _modelId : "")
{}

// Just calls all 4 previous `Get...` functions and copies the provided values into `drawInfo` and `v`
void XPCAircraft::UpdatePosition(float, int)
{
    // Call the "callback" virtual functions and then update the core variables
    acPos.multiIdx = GetTcasTargetIdx();             // provide the multiplayer index back to the plugin
    if (GetPlanePosition(&acPos) == xpmpData_NewData) {
        // Set the position and orientation
        SetLocation(acPos.lat, acPos.lon, acPos.elevation);
        drawInfo.pitch      = acPos.pitch;
        drawInfo.roll       = acPos.roll;
        drawInfo.heading    = acPos.heading;
        // Update the other values from acPos
        label               = acPos.label;
        vertOfsRatio        = acPos.offsetScale;
        bClampToGround      = acPos.clampToGround;
        aiPrio              = acPos.aiPrio;
        memmove(colLabel, acPos.label_color, sizeof(colLabel));
    }
    
    if (GetPlaneSurfaces(&acSurfaces) == xpmpData_NewData) {
        SetGearRatio            (acSurfaces.gearPosition);
        SetFlapRatio            (acSurfaces.flapRatio);
        SetSpoilerRatio         (acSurfaces.spoilerRatio);
        SetSpeedbrakeRatio      (acSurfaces.speedBrakeRatio);
        SetSlatRatio            (acSurfaces.slatRatio);
        SetWingSweepRatio       (acSurfaces.wingSweep);
        SetThrustRatio          (acSurfaces.thrust);
        SetYokePitchRatio       (acSurfaces.yokePitch);
        SetYokeHeadingRatio     (acSurfaces.yokeHeading);
        SetYokeRollRatio        (acSurfaces.yokeRoll);
        SetThrustReversRatio    (acSurfaces.thrust < 0.0f ? 1.0f : 0.0f);
        
        SetLightsTaxi           (acSurfaces.lights.taxiLights);
        SetLightsLanding        (acSurfaces.lights.landLights);
        SetLightsBeacon         (acSurfaces.lights.bcnLights);
        SetLightsStrobe         (acSurfaces.lights.strbLights);
        SetLightsNav            (acSurfaces.lights.navLights);
        
        SetTireDeflection       (acSurfaces.tireDeflect);
        SetNoseGearDeflection   (acSurfaces.tireDeflect);
        SetTireRotAngle         (acSurfaces.tireRotDegree);
        SetTireRotRpm           (acSurfaces.tireRotRpm);
        
        SetEngineRotAngle       (acSurfaces.engRotDegree);
        SetEngineRotRpm         (acSurfaces.engRotRpm);
        SetPropRotAngle         (acSurfaces.propRotDegree);
        SetPropRotRpm           (acSurfaces.propRotRpm);

        SetReversDeployRatio    (acSurfaces.reversRatio);
        
        SetTouchDown            (acSurfaces.touchDown);
    }
    
    // The following 2 calls provide directly the member variable structures:
    GetPlaneRadar(&acRadar);
    GetInfoTexts(&acInfoTexts);
}

//
// MARK: Global Functions
//

namespace XPMP2 {

/// We need to provide these functions for purely formal reasons.
/// They are not actually _ever_ called as we provide the current dataRef values via XPLMInstanceSetPosition.
/// So we don't bother providing any implementation
float obj_get_float(void * /*refcon*/)
{
    return 0.0f;
}

/// See obj_get_float()
int obj_get_float_array(
        void *               /*refcon*/,
        float *              ,
        int                  /*inOffset*/,
        int                  inCount)
{
    return inCount;
}



// Initialize the module
void AcInit ()
{
    // as long as no additional dataRefs are defined we can validate
    // the the internal dataRef definitions match up with the DR_VALS enums:
    if (drStrings.empty()) {
        LOG_ASSERT(DR_NAMES.size()-1 == V_COUNT);
    }
    
    // Register all our dataRefs
    if (ahDataRefs.empty()) {
        ahDataRefs.reserve(DR_NAMES.size()-1);
        for (const char* drName: DR_NAMES) {
            if (!drName) break;
            ahDataRefs.push_back(XPLMRegisterDataAccessor(drName,
                                                          xplmType_Float|xplmType_FloatArray, 0,
                                                          NULL, NULL,
                                                          obj_get_float, NULL,
                                                          NULL, NULL,
                                                          NULL, NULL,
                                                          obj_get_float_array, NULL,
                                                          NULL, NULL, (void*)drName, NULL));
        }
        
        // We expect to have one less dataRef handle than strings
        // because the last string must be nullptr
        LOG_ASSERT(ahDataRefs.size() == DR_NAMES.size()-1);
    }
}

// Grace cleanup
void AcCleanup ()
{
    // We don't own the aircraft! So whatever is left now was not properly
    // destroyed prior to shutdown
    if (!glob.mapAc.empty()) {
        LOG_MSG(logWARN, WARN_PLANES_LEFT_EXIT, (unsigned long)glob.mapAc.size());
        glob.mapAc.clear();
    }
    
    // Destroy flight loop
    if (gFlightLoopID) {
        XPLMDestroyFlightLoop(gFlightLoopID);
        gFlightLoopID = nullptr;
    }
    
    // Unregister dataRefs
    for (XPLMDataRef dr: ahDataRefs)
        if (dr)
            XPLMUnregisterDataAccessor(dr);
    ahDataRefs.clear();
}

// Find aircraft by its plane ID, can return nullptr
Aircraft* AcFindByID (XPMPPlaneID _id)
{
    try {
        // try finding the plane by its id
        return glob.mapAc.at(_id);
    }
    catch (const std::out_of_range&) {
        // not found
        return nullptr;
    }
}

// (Re)Define default wake turbulence values per WTC
bool AcSetDefaultWakeData(const std::string& _wtc, const Aircraft::wakeTy& _wake)
{
    // _wake values must be properly filled, only `lift_N` can be left out
    if (_wake.needsDefaults()) {
        LOG_MSG(logERR, "Parameter '_wake' does not define all required data for wtc '%s'", _wtc.c_str());
        return false;
    }

    // Add or reapply it to our map
    mapWake[_wtc] = _wake;
    return true;
}


}   // namespace XPMP2

//
// MARK: Global functions outside XPMP2 namespace
//

// Add a new dataRef
size_t XPMPAddModelDataRef (const std::string& dataRef)
{
    // Sanity check
    if (dataRef.empty())
        return 0;
    
    // Cannot do if not yet initialized
    if (ahDataRefs.size() != DR_NAMES.size()-1) {
        LOG_MSG(logERR, ERR_ADD_DATAREF_INIT, dataRef.c_str())
        return 0;
    }
    
    // Do we know that dataRef already? -> return its index
    const auto iter = std::find_if(DR_NAMES.cbegin(),
                                   DR_NAMES.cend(),
                                   [&](const char*s){return s && dataRef==s;});
    if (iter != DR_NAMES.cend())
        return (size_t)std::distance(DR_NAMES.cbegin(), iter);
    
    // Cannot add a new one while planes are active
    if (!glob.mapAc.empty()) {
        LOG_MSG(logERR, ERR_ADD_DATAREF_PLANES, dataRef.c_str(), (unsigned long)glob.mapAc.size());
        return 0;
    }
    
    // --- Add the new dataRaf ---
    
    // Must be called from XP's main thread only as we are calling XPLM SDK functions!!
    if (!glob.IsXPThread()) {
        LOG_MSG(logERR, ERR_WORKER_THREAD);
        return 0;
    }

    // Copy the provided text: This creates a copy of std::string, pointed to by a smart pointer
    drStrings.emplace_back(std::make_unique<std::string>(dataRef));
    const char* drName = drStrings.back()->c_str();
        
    // The last element of DR_NAMES is always a 'nullptr' as this marks
    // the end of the list when passed on to XPLMCreateInstance.
    // We now overwrite with the new dataRef and add instead a new nullptr:
    assert(DR_NAMES.back() == nullptr);
    DR_NAMES.back() = drName;
    DR_NAMES.push_back(nullptr);
    
    // Register this new data accessor with XP
    ahDataRefs.push_back(XPLMRegisterDataAccessor(drName,
                                                  xplmType_Float|xplmType_FloatArray, 0,
                                                  NULL, NULL,
                                                  obj_get_float, NULL,
                                                  NULL, NULL,
                                                  NULL, NULL,
                                                  obj_get_float_array, NULL,
                                                  NULL, NULL, (void*)drName, NULL));
    // We expect to have one less dataRef handle than strings
    // because the last string must be nullptr
    assert(ahDataRefs.size() == DR_NAMES.size()-1);

    // the index of the new dataRef
    const size_t idx = DR_NAMES.size() - 2;
    LOG_MSG(logDEBUG, DEBUG_DATAREF_ADDED, drName, (unsigned long)idx);
    return idx;
}

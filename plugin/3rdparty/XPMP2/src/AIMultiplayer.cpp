/// @file       AIMultiplayer.cpp
/// @brief      Implementation of serving TCAS target dataRef updates
/// @details    Updates the up to 63 slots of X-Plane's defined TCAS targets,
///             which are in turn synced with the 19 "legacy" multiplayer slots,
///             which in turn many other plugins (like TCAS implementations or map tools)
///             read to learn about any multiplayer aircraft.\n
///             Also serves shared dataRefs for publishing text information,
///             which is not X-Plane standard but originally suggested by FSTramp.
///             However, the post and file, in which he suggested this addition,
///             is no longer available on forums.x-plane.org. Still, XPMP2
///             fullfills the earlier definition.
/// @see        X-Plane 11.50 Beta 8 introduced a new way of informing X-Plane
///             of TCAS targets, see here a blog entry for details:
///             https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/
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

#define INFO_AI_CONTROL         "Have control now over AI/Multiplayer planes"
#define INFO_AI_CONTROL_ENDS    "Released control of AI/Multiplayer planes"
#define INFO_REMOTE_CLIENT_AI   "XPMP2 Remote Client controls TCAS. We don't need control as our planes will show up there."
#define WARN_NO_AI_CONTROL      "%s controls TCAS / AI. %s could NOT acquire control, our planes might NOT appear on TCAS or 3rd party apps."
#define DEBUG_AI_SLOT_ASSIGN    "Aircraft %llu: ASSIGNING AI Slot %d (%s, %s, %s)"
#define DEBUG_AI_SLOT_RELEASE   "Aircraft %llu: RELEASING AI Slot %d (%s, %s, %s)"

namespace XPMP2 {

//
// MARK: Handling of AI/multiplayer Planes for TCAS, maps, camera plugins...
//

/// Don't dare using NAN...but with this coordinate for x/y/z a plane should be far out and virtually invisible
constexpr float FAR_AWAY_VAL_GL = 9999999.9f;
/// How often do we reassign AI slots? [seconds]
constexpr float AISLOT_CHANGE_PERIOD = 15.0f;
/// How much distance does each AIPrio add?
constexpr int AI_PRIO_MULTIPLIER = 10 * M_per_NM;
/// A constant array of zero values supporting quick array initialization
float F_NULL[10] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

/// The drawing phase "xplm_Phase_Airplanes" is deprecated in XP11.50 upwards, but we need it in earlier versions to fake TCAS
constexpr int XPLM_PHASE_AIRPLANES = 25;

// DataRef editors, which we inform about our dataRefs
#define MSG_ADD_DATAREF 0x01000000          ///< Message to DRE/DRT to inform about dataRefs
const char* DATA_REF_EDITORS[] = {          ///< the dataRef editors that we search for and inform
    "xplanesdk.examples.DataRefEditor",
    "com.leecbaker.datareftool"
};
std::vector<std::string> vecDREdataRefStr;  ///< list of dataRef strings to be send to the editors

//
// MARK: Legacy multiplayer dataRefs
//

/// Keeps the dataRef handles for one of the up to 19 legacy AI/Multiplayer slots ("sim/multiplayer/position")
struct multiDataRefsTy {
    XPLMDataRef X = nullptr;            ///< position
    XPLMDataRef Y = nullptr;
    XPLMDataRef Z = nullptr;

    XPLMDataRef v_x = nullptr;          ///< cartesian velocities
    XPLMDataRef v_y = nullptr;
    XPLMDataRef v_z = nullptr;

    XPLMDataRef pitch   = nullptr;      ///< theta
    XPLMDataRef roll    = nullptr;      ///< phi
    XPLMDataRef heading = nullptr;      ///< psi

    XPLMDataRef gear        = nullptr;  ///< gear_deploy[10]
    XPLMDataRef flap        = nullptr;  ///< flap_ratio
    XPLMDataRef flap2       = nullptr;  ///< flap_ratio2
    XPLMDataRef spoiler     = nullptr;  ///< spoiler_ratio
    XPLMDataRef speedbrake  = nullptr;  ///< speedbrake
    XPLMDataRef slat        = nullptr;  ///< slat_ratio
    XPLMDataRef wingSweep   = nullptr;  ///< wing_sweep
    XPLMDataRef throttle    = nullptr;  ///< throttle[8]
    XPLMDataRef yoke_pitch  = nullptr;  ///< yolk_pitch
    XPLMDataRef yoke_roll   = nullptr;  ///< yolk_roll
    XPLMDataRef yoke_yaw    = nullptr;  ///< yolk_yaw

    XPLMDataRef bcnLights   = nullptr;  ///< beacon_lights_on
    XPLMDataRef landLights  = nullptr;  ///< landing_lights_on
    XPLMDataRef navLights   = nullptr;  ///< nav_lights_on
    XPLMDataRef strbLights  = nullptr;  ///< strobe_lights_on
    XPLMDataRef taxiLights  = nullptr;  ///< taxi_light_on

    /// Looks OK, the dataRefs are available?
    inline operator bool () const { return X && Y && Z && pitch && roll && heading && taxiLights; }
    /// Clear the tested dataRefs
    void clear () { X = Y = Z = pitch = roll = heading = yoke_pitch = taxiLights = nullptr; }
};

/// Keeps the dataRef handles for one of the up to 63 shared data slots ("sim/multiplayer/position/plane#...")
struct infoDataRefsTy {
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    XPLMDataRef infoTailNum         = nullptr;  // tailnum
    XPLMDataRef infoIcaoAcType      = nullptr;  // ICAO
    XPLMDataRef infoManufacturer    = nullptr;  // manufacturer
    XPLMDataRef infoModel           = nullptr;  // model
    XPLMDataRef infoIcaoAirline     = nullptr;  // ICAOairline
    XPLMDataRef infoAirline         = nullptr;  // airline
    XPLMDataRef infoFlightNum       = nullptr;  // flightnum
    XPLMDataRef infoAptFrom         = nullptr;  // apt_from
    XPLMDataRef infoAptTo           = nullptr;  // apt_to
    // Additional fields beyond content of XPMPInfoTexts_t:
    XPLMDataRef cslModel            = nullptr;  ///< CSL model as XPMP2::Aircraft::GetModelName() returns it

    /// Looks OK, the dataRefs are available?
    inline operator bool () const { return infoTailNum && infoIcaoAirline && infoAptTo; }
};

/// Number of characters to be allowed for CSL model text
constexpr size_t SDR_CSLMODEL_TXT_SIZE = 40;

//
// MARK: TCAS Target dataRefs
//

static XPLMDataRef drTcasOverride   = nullptr;      ///< sim/operation/override/override_TCAS               int
static XPLMDataRef drMapOverride    = nullptr;      ///< sim/operation/override/override_multiplayer_map_layer int
static XPLMDataRef drTcasModeS      = nullptr;      ///< sim/cockpit2/tcas/targets/modeS_id                 int[64]
static XPLMDataRef drTcasModeC      = nullptr;      ///< sim/cockpit2/tcas/targets/modeC_code               int[64]
static XPLMDataRef drTcasSsrMode    = nullptr;      ///< sim/cockpit2/tcas/targets/ssr_mode                 int[64]    y    enum    Transponder mode: off=0, stdby=1, on (mode A)=2, alt (mode C)=3, test=4, GND (mode S)=5, ta_only (mode S)=6, ta/ra=7
static XPLMDataRef drTcasFlightId   = nullptr;      ///< sim/cockpit2/tcas/targets/flight_id                byte[512]
static XPLMDataRef drTcasIcaoType   = nullptr;      ///< sim/cockpit2/tcas/targets/icao_type                byte[512]
static XPLMDataRef drTcasX          = nullptr;      ///< sim/cockpit2/tcas/targets/position/x               float[64]
static XPLMDataRef drTcasY          = nullptr;      ///< sim/cockpit2/tcas/targets/position/y               float[64]
static XPLMDataRef drTcasZ          = nullptr;      ///< sim/cockpit2/tcas/targets/position/z               float[64]
static XPLMDataRef drTcasVX         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vx              float[64]
static XPLMDataRef drTcasVY         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vy              float[64]
static XPLMDataRef drTcasVZ         = nullptr;      ///< sim/cockpit2/tcas/targets/position/vz              float[64]
static XPLMDataRef drTcasVertSpeed  = nullptr;      ///< sim/cockpit2/tcas/targets/position/vertical_speed  float[64]
static XPLMDataRef drTcasHeading    = nullptr;      ///< sim/cockpit2/tcas/targets/position/psi             float[64]
static XPLMDataRef drTcasPitch      = nullptr;      ///< sim/cockpit2/tcas/targets/position/the             float[64]
static XPLMDataRef drTcasRoll       = nullptr;      ///< sim/cockpit2/tcas/targets/position/phi             float[64]
static XPLMDataRef drTcasGrnd       = nullptr;      ///< sim/cockpit2/tcas/targets/position/weight_on_wheels  int[64]
static XPLMDataRef drTcasGear       = nullptr;      ///< sim/cockpit2/tcas/targets/position/gear_deploy     float[64]
static XPLMDataRef drTcasFlap       = nullptr;      ///< sim/cockpit2/tcas/targets/position/flap_ratio      float[64]
static XPLMDataRef drTcasFlap2      = nullptr;      ///< sim/cockpit2/tcas/targets/position/flap_ratio2     float[64]
static XPLMDataRef drTcasSpeedbrake = nullptr;      ///< sim/cockpit2/tcas/targets/position/speedbrake_ratio float[64]
static XPLMDataRef drTcasSlat       = nullptr;      ///< sim/cockpit2/tcas/targets/position/slat_ratio      float[64]
static XPLMDataRef drTcasWingSweep  = nullptr;      ///< sim/cockpit2/tcas/targets/position/wing_sweep      float[64]
static XPLMDataRef drTcasThrottle   = nullptr;      ///< sim/cockpit2/tcas/targets/position/throttle        float[64]
static XPLMDataRef drTcasYokePitch  = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_pitch [sic!]   float[64]
static XPLMDataRef drTcasYokeRoll   = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_roll [sic!]    float[64]
static XPLMDataRef drTcasYokeYaw    = nullptr;      ///< sim/cockpit2/tcas/targets/position/yolk_yaw [sic!]     float[64]
static XPLMDataRef drTcasLights     = nullptr;      ///< sim/cockpit2/tcas/targets/position/lights          int[64] (bitfield: beacon=1, land=2, nav=4, strobe=8, taxi=16)
// Wake support as of X-Plane 12, see https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/
static XPLMDataRef drTcasWakeWingSpan   = nullptr;  ///< sim/cockpit2/tcas/targets/wake/wing_span_m	float[64]	y	meter	wing span of the aircraft creating wake turbulence
static XPLMDataRef drTcasWakeWingArea   = nullptr;  ///< sim/cockpit2/tcas/targets/wake/wing_area_m2	float[64]	y	meter	wing area(total area of both wings combined) of the aircraft creating wake turbulence
static XPLMDataRef drTcasWakeCat        = nullptr;  ///< sim/cockpit2/tcas/targets/wake/wake_cat	int[64]	y	enum	wake category of the aircraft.This is for information purposes onlyand is not used to calculate the actual strength of the turbulence. 0 = Light, 1 = Medium, 2 = Heavy, 3 = Super
static XPLMDataRef drTcasWakeMass       = nullptr;  ///< sim/cockpit2/tcas/targets/wake/mass_kg	float[64]	y	kg	actual mass of the aircraft creating the wake
static XPLMDataRef drTcasWakeAoA        = nullptr;  ///< sim/cockpit2/tcas/targets/wake/aoa	float[64]	y	degrees	angle of attack of the wing creating the wake
static XPLMDataRef drTcasWakeLift       = nullptr;  ///< sim/cockpit2/tcas/targets/wake/lift_N	float[64]	y	Newton	instantaneous lift force of the whole wing generated right now, in Newtons

//
// MARK: Global variables for TCAS handling
//

/// Number of TCAS slots (either as TCAS targets or as multiplayer slots), _not_ counting user's plane (so expected to be 63 or 19)
static size_t numSlots = 0;

/// A structure simplifying communicaton with sim/cockpit2/tcas/targets/position/lights
union TcasLightsTy {
    int i;                                          ///< the full integer value
    struct BitsTy {
        bool beacon : 1;                            ///< Beacon lights
        bool land   : 1;                            ///< Landing lights
        bool nav    : 1;                            ///< Navigation lights
        bool strobe : 1;                            ///< Strobe lights
        bool taxi   : 1;                            ///< Taxi lights
    } b;
};

/// Keeps the dataRef handles for one of the up to 19 legacy AI/Multiplayer slots ("sim/multiplayer/position")
static std::vector<multiDataRefsTy> gMultiRef;
/// Keeps the dataRef handles for one of the up to 63 shared data slots ("sim/multiplayer/position/plane#...")
static std::vector<infoDataRefsTy>  gInfoRef;

/// Map of Aircrafts, sorted by (priority-biased) distance
typedef std::map<float,XPMPPlaneID> mapAcByDistTy;
/// Map of Aircrafts, sorted by (priority-biased) distance
static mapAcByDistTy gMapAcByDist;
/// Vector of actual (verified) aircraft, ordered by distance
static std::vector<Aircraft*> vAcByDist;

/// Vector organized by slots (either multiplayer or TCAS target slots)
static std::vector<Aircraft*> gSlots;

/// When did we re-calculate slots last time?
static float tLastSlotSwitching = 0.0f;

// How many planes did we produce last cycle?
static size_t numTargetsLastTime = 0;

//
// MARK: Aircraft functions related to TCAS
//

// Is this plane currently also being tracked by X-Plane's classic AI/multiplayer?
bool Aircraft::IsCurrentlyShownAsAI () const
{
    return 1 <= tcasTargetIdx && tcasTargetIdx < (int)gMultiRef.size();
}

//
// MARK: TCAS/Multiplayer Update
//

// Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
void AIMultiClearAIDataRefs (multiDataRefsTy& mdr, bool bDeactivateToZero = false);
void AIMultiClearInfoDataRefs (infoDataRefsTy& drI);

/// @brief Are we going to new (TCAS override) or the old way?
inline bool GoTCASOverride ()
{
    return drTcasModeS != nullptr;
}

/// @brief Can we use the XP12 wake support system?
inline bool GoTCASWake()
{
    return drTcasWakeWingSpan != nullptr && drTcasWakeLift != nullptr;
}

/// @brief The old way: Update Multiplayer dataRefs directly
/// @return Number of TCAS targets produced
size_t AIUpdateMultiplayerDataRefs()
{
    // Loop over all filled slots
    size_t slot = 1;
    for (; slot < gSlots.size() && gSlots[slot] != nullptr; ++slot)
    {
        Aircraft& ac = *gSlots[slot];
        
        try {
            // Has the slot for this plane changed?
            const bool bSlotChanged = ac.GetTcasTargetIdx() != (int)slot;
            if (bSlotChanged)
                ac.SetTcasTargetIdx((int)slot);

            // the dataRefs to use
            const multiDataRefsTy& mdr = gMultiRef.at(slot);

            // This plane's position
            XPLMSetDataf(mdr.X, ac.drawInfo.x);
            XPLMSetDataf(mdr.Y, ac.drawInfo.y - ac.GetVertOfs());  // align with original altitude
            XPLMSetDataf(mdr.Z, ac.drawInfo.z);
            // attitude
            XPLMSetDataf(mdr.pitch,   ac.drawInfo.pitch);
            XPLMSetDataf(mdr.roll,    ac.drawInfo.roll);
            XPLMSetDataf(mdr.heading, ac.drawInfo.heading);
            // configuration
            std::array<float,10> arrGear;               // gear ratio for any possible gear...10 are defined by X-Plane!
            arrGear.fill(ac.v[V_CONTROLS_GEAR_RATIO]);
            XPLMSetDatavf(mdr.gear, arrGear.data(), 0, 10);
            XPLMSetDataf(mdr.flap,  ac.v[V_CONTROLS_FLAP_RATIO]);
            XPLMSetDataf(mdr.flap2, ac.v[V_CONTROLS_FLAP_RATIO]);
            // [...]
            if (mdr.yoke_pitch) {
                XPLMSetDataf(mdr.yoke_pitch, ac.v[V_CONTROLS_YOKE_PITCH_RATIO]);
                XPLMSetDataf(mdr.yoke_roll,  ac.v[V_CONTROLS_YOKE_ROLL_RATIO]);
                XPLMSetDataf(mdr.yoke_yaw,   ac.v[V_CONTROLS_YOKE_HEADING_RATIO]);
            }

            // For performance reasons and because differences (cartesian velocity)
            // are smoother if calculated over "longer" time frames,
            // the following updates are done about every second only
            const float now = GetMiscNetwTime();
            if (bSlotChanged || now >= ac.prev_ts + 1.0f)
            {
                // do we have any prev x/y/z values at all?
                if (ac.prev_ts > 0.0001f) {
                    // yes, so we can calculate velocity
                    const float d_s = now - ac.prev_ts;                 // time that had passed in seconds
                    XPLMSetDataf(mdr.v_x, ac.v_x = (ac.drawInfo.x - ac.prev_x) / d_s);
                    XPLMSetDataf(mdr.v_y, ac.v_y = (ac.drawInfo.y - ac.prev_y) / d_s);
                    XPLMSetDataf(mdr.v_z, ac.v_z = (ac.drawInfo.z - ac.prev_z) / d_s);
                    // based on horizontal coordinates calculate a (rough) ground speed
                    ac.gs_kn = std::sqrt(ac.v_x*ac.v_x + ac.v_z*ac.v_z) * float(KT_per_M_per_S);
                }
                ac.prev_x = ac.drawInfo.x;
                ac.prev_y = ac.drawInfo.y;
                ac.prev_z = ac.drawInfo.z;
                ac.prev_ts = now;

                // configuration (cont.)
                XPLMSetDataf(mdr.spoiler,       ac.v[V_CONTROLS_SPOILER_RATIO]);
                XPLMSetDataf(mdr.speedbrake,    ac.v[V_CONTROLS_SPEED_BRAKE_RATIO]);
                XPLMSetDataf(mdr.slat,          ac.v[V_CONTROLS_SLAT_RATIO]);
                XPLMSetDataf(mdr.wingSweep,     ac.v[V_CONTROLS_WING_SWEEP_RATIO]);
                std::array<float,8> arrThrottle;
                arrThrottle.fill(ac.v[V_CONTROLS_THRUST_RATIO]);
                XPLMSetDatavf(mdr.throttle,     arrThrottle.data(), 0, 8);
                // lights
                XPLMSetDatai(mdr.bcnLights,     ac.v[V_CONTROLS_BEACON_LITES_ON] > 0.5f);
                XPLMSetDatai(mdr.landLights,    ac.v[V_CONTROLS_LANDING_LITES_ON] > 0.5f);
                XPLMSetDatai(mdr.navLights,     ac.v[V_CONTROLS_NAV_LITES_ON] > 0.5f);
                XPLMSetDatai(mdr.strbLights,    ac.v[V_CONTROLS_STROBE_LITES_ON] > 0.5f);
                XPLMSetDatai(mdr.taxiLights,    ac.v[V_CONTROLS_TAXI_LITES_ON] > 0.5f);

                // Shared data for providing textual info (see XPMPInfoTexts_t)
                const infoDataRefsTy drI = gInfoRef.at(slot);
                XPLMSetDatab(drI.infoTailNum,       ac.acInfoTexts.tailNum,       0, sizeof(XPMPInfoTexts_t::tailNum));
                XPLMSetDatab(drI.infoIcaoAcType,    ac.acInfoTexts.icaoAcType,    0, sizeof(XPMPInfoTexts_t::icaoAcType));
                XPLMSetDatab(drI.infoManufacturer,  ac.acInfoTexts.manufacturer,  0, sizeof(XPMPInfoTexts_t::manufacturer));
                XPLMSetDatab(drI.infoModel,         ac.acInfoTexts.model,         0, sizeof(XPMPInfoTexts_t::model));
                XPLMSetDatab(drI.infoIcaoAirline,   ac.acInfoTexts.icaoAirline,   0, sizeof(XPMPInfoTexts_t::icaoAirline));
                XPLMSetDatab(drI.infoAirline,       ac.acInfoTexts.airline,       0, sizeof(XPMPInfoTexts_t::airline));
                XPLMSetDatab(drI.infoFlightNum,     ac.acInfoTexts.flightNum,     0, sizeof(XPMPInfoTexts_t::flightNum));
                XPLMSetDatab(drI.infoAptFrom,       ac.acInfoTexts.aptFrom,       0, sizeof(XPMPInfoTexts_t::aptFrom));
                XPLMSetDatab(drI.infoAptTo,         ac.acInfoTexts.aptTo,         0, sizeof(XPMPInfoTexts_t::aptTo));
                
                char buf[SDR_CSLMODEL_TXT_SIZE];
                memset(buf, 0, sizeof(buf));
                STRCPY_S(buf, ac.GetModelName().c_str());
                XPLMSetDatab(drI.cslModel,          buf,                          0, sizeof(buf));
            }
        }
        CATCH_AC(ac)
    }
    
    // return number of multiplayer planes
    return slot;
}

/// @brief The modern way: Use TCAS override and update TCAS targets
/// @return Number of TCAS targets produced (incl. user's plane)
size_t AIUpdateTCASTargets ()
{
    // Arrays we need every frame over and over again, so we keep them static for performance
    // data arrays for providing TCAS target values
    static std::vector<int>   vModeS;      
    static std::vector<int>   vModeC;      
    static std::vector<int>   vSsrMode;
    static std::vector<float> vX;
    static std::vector<float> vY;          
    static std::vector<float> vZ;          
    static std::vector<float> vVertSpeed;  
    static std::vector<float> vHeading;    
    static std::vector<float> vPitch;      
    static std::vector<float> vRoll;       
    static std::vector<int>   vGrnd;
    static std::vector<float> vGear;
    static std::vector<float> vFlap;       
    static std::vector<float> vSpeedbrake; 
    static std::vector<float> vSlat;       
    static std::vector<float> vWingSweep;  
    static std::vector<float> vThrottle;   
    static std::vector<float> vYokePitch;  
    static std::vector<float> vYokeRoll;   
    static std::vector<float> vYokeYaw;    
    static std::vector<int>   vLights;     
    static std::vector<float> vWakeWingSpan;
    static std::vector<float> vWakeWingArea;
    static std::vector<int>   vWakeCat;
    static std::vector<float> vWakeMass;
    static std::vector<float> vWakeAoA;
    static std::vector<float> vWakeLift;

    // Start filling up TCAS targets, ordered by distance,
    // so that the closest planes are in the lower slots,
    // mirrored to the legacy multiplayer slots
    vModeS.assign(numSlots, 0);
    vModeC.assign(numSlots, 0);
    vSsrMode.assign(numSlots, 0);
    vX.assign(numSlots, 0);
    vY.assign(numSlots, 0);
    vZ.assign(numSlots, 0);
    vVertSpeed.assign(numSlots, 0);
    vHeading.assign(numSlots, 0);
    vPitch.assign(numSlots, 0);
    vRoll.assign(numSlots, 0);
    vGrnd.assign(numSlots, 0);
    vGear.assign(numSlots, 0);
    vFlap.assign(numSlots, 0);
    vSpeedbrake.assign(numSlots, 0);
    vSlat.assign(numSlots, 0);
    vWingSweep.assign(numSlots, 0);
    vThrottle.assign(numSlots, 0);
    vYokePitch.assign(numSlots, 0);
    vYokeRoll.assign(numSlots, 0);
    vYokeYaw.assign(numSlots, 0);
    vLights.assign(numSlots, 0);
    vWakeWingSpan.assign(numSlots, 0);
    vWakeWingArea.assign(numSlots, 0);
    vWakeCat.assign(numSlots, 0);
    vWakeMass.assign(numSlots, 0);
    vWakeAoA.assign(numSlots, 0);
    vWakeLift.assign(numSlots, 0);

    const float now = GetMiscNetwTime();

    // Loop over all filled slots
    size_t slot = 1;
    for (; slot < gSlots.size() && gSlots[slot] != nullptr; ++slot)
    {
        Aircraft& ac = *gSlots[slot];
        
        try {
            // Has the slot for this plane changed?
            const bool bSlotChanged = ac.GetTcasTargetIdx() != (int)slot;
            if (bSlotChanged)
                ac.SetTcasTargetIdx((int)slot);

            // Add the plane to the list of TCAS targets
            const size_t idx = slot-1;                      // index in all the v... arrays
            vModeS.at(idx)          = int(ac.GetModeS_ID());
            vModeC.at(idx)          = int(ac.acRadar.code);
            vSsrMode.at(idx)        = int(ac.acRadar.mode);
            
            // This plane's position
            vX.at(idx)              = ac.drawInfo.x;
            vY.at(idx)              = ac.drawInfo.y - ac.GetVertOfs();  // align with original altitude
            vZ.at(idx)              = ac.drawInfo.z;
            vGrnd.at(idx)           = ac.IsOnGrnd();
            
            // attitude
            vPitch.at(idx)          = ac.drawInfo.pitch;
            vRoll.at(idx)           = ac.drawInfo.roll;
            vHeading.at(idx)        = ac.drawInfo.heading;
            
            // configuration
            vGear.at(idx)           = ac.v[V_CONTROLS_GEAR_RATIO];
            vFlap.at(idx)           = ac.v[V_CONTROLS_FLAP_RATIO];
            vSpeedbrake.at(idx)     = ac.v[V_CONTROLS_SPEED_BRAKE_RATIO];
            vSlat.at(idx)           = ac.v[V_CONTROLS_SLAT_RATIO];
            vWingSweep.at(idx)      = ac.v[V_CONTROLS_WING_SWEEP_RATIO];
            vThrottle.at(idx)       = ac.v[V_CONTROLS_THRUST_RATIO];
            
            // Yoke
            vYokePitch.at(idx)      = ac.v[V_CONTROLS_YOKE_PITCH_RATIO];
            vYokeRoll.at(idx)       = ac.v[V_CONTROLS_YOKE_ROLL_RATIO];
            vYokeYaw.at(idx)        = ac.v[V_CONTROLS_YOKE_HEADING_RATIO];
            
            // lights
            TcasLightsTy l = {0};
            l.b.beacon              = ac.v[V_CONTROLS_BEACON_LITES_ON] > 0.5f;
            l.b.land                = ac.v[V_CONTROLS_LANDING_LITES_ON] > 0.5f;
            l.b.nav                 = ac.v[V_CONTROLS_NAV_LITES_ON] > 0.5f;
            l.b.strobe              = ac.v[V_CONTROLS_STROBE_LITES_ON] > 0.5f;
            l.b.taxi                = ac.v[V_CONTROLS_TAXI_LITES_ON] > 0.5f;
            vLights.at(idx)         = l.i;

            // Wake data
            vWakeWingSpan.at(idx)   = ac.GetWingSpan();
            vWakeWingArea.at(idx)   = ac.GetWingArea();
            vWakeCat.at(idx)        = ac.GetWakeCat();
            vWakeMass.at(idx)       = ac.GetMass();
            vWakeAoA.at(idx)        = ac.GetAoA();
            vWakeLift.at(idx)       = ac.GetLift();

            // For performance reasons and because differences (cartesian velocity)
            // are smoother if calculated over "longer" time frames,
            // the following updates are done about every second only,
            // or if the a/c changed slot (to make sure all dataRef values are in synch)
            if (bSlotChanged || (now >= ac.prev_ts + 1.0f))
            {
                // do we have any prev x/y/z values at all?
                if (ac.prev_ts > 0.0001f) {
                    // yes, so we can calculate velocity
                    const float d_t = now - ac.prev_ts;                 // time that had passed in seconds
                    const float d_x = ac.drawInfo.x - ac.prev_x;
                    const float d_y = ac.drawInfo.y - ac.prev_y;
                    const float d_z = ac.drawInfo.z - ac.prev_z;
                    float f = d_x / d_t;
                    XPLMSetDatavf(drTcasVX, &f, int(slot), 1);
                    f = d_y / d_t;
                    XPLMSetDatavf(drTcasVY, &f, int(slot), 1);
                    f = d_z / d_t;
                    XPLMSetDatavf(drTcasVZ, &f, int(slot), 1);
                    
                    // vertical speed (roughly...y is not exact, but let's keep things simple here),
                    // convert from m/s to ft/min
                    f = (d_y / d_t) * (60.0f / float(M_per_FT));
                    XPLMSetDatavf(drTcasVertSpeed, &f, int(slot), 1);
                }
                ac.prev_x = ac.drawInfo.x;
                ac.prev_y = ac.drawInfo.y;
                ac.prev_z = ac.drawInfo.z;
                ac.prev_ts = now;
                
                // Flight or tail number as FlightID
                char s[8];
                memset(s, 0, sizeof(s));
                STRCPY_S(s, ac.GetFlightId().c_str());
                XPLMSetDatab(drTcasFlightId, s, int(slot * sizeof(s)), sizeof(s));
                
                // Icao Type code
                memset(s, 0, sizeof(s));
                STRCPY_S(s, ac.acIcaoType.substr(0,sizeof(s)-1).c_str());
                XPLMSetDatab(drTcasIcaoType, s, int(slot * sizeof(s)), sizeof(s));
                
                // Shared data for providing textual info (see XPMPInfoTexts_t)
                const infoDataRefsTy drI = gInfoRef.at(slot);
                XPLMSetDatab(drI.infoTailNum,       ac.acInfoTexts.tailNum,       0, sizeof(XPMPInfoTexts_t::tailNum));
                XPLMSetDatab(drI.infoIcaoAcType,    ac.acInfoTexts.icaoAcType,    0, sizeof(XPMPInfoTexts_t::icaoAcType));
                XPLMSetDatab(drI.infoManufacturer,  ac.acInfoTexts.manufacturer,  0, sizeof(XPMPInfoTexts_t::manufacturer));
                XPLMSetDatab(drI.infoModel,         ac.acInfoTexts.model,         0, sizeof(XPMPInfoTexts_t::model));
                XPLMSetDatab(drI.infoIcaoAirline,   ac.acInfoTexts.icaoAirline,   0, sizeof(XPMPInfoTexts_t::icaoAirline));
                XPLMSetDatab(drI.infoAirline,       ac.acInfoTexts.airline,       0, sizeof(XPMPInfoTexts_t::airline));
                XPLMSetDatab(drI.infoFlightNum,     ac.acInfoTexts.flightNum,     0, sizeof(XPMPInfoTexts_t::flightNum));
                XPLMSetDatab(drI.infoAptFrom,       ac.acInfoTexts.aptFrom,       0, sizeof(XPMPInfoTexts_t::aptFrom));
                XPLMSetDatab(drI.infoAptTo,         ac.acInfoTexts.aptTo,         0, sizeof(XPMPInfoTexts_t::aptTo));
                
                char buf[SDR_CSLMODEL_TXT_SIZE];
                memset(buf, 0, sizeof(buf));
                STRCPY_S(buf, ac.GetModelName().c_str());
                XPLMSetDatab(drI.cslModel,          buf,                          0, sizeof(buf));
            }
        }
        CATCH_AC(ac)
    }
    
    // Feed the dataRefs to X-Plane for TCAS target tracking
#define SET_DR(ty, dr) XPLMSetData##ty(drTcas##dr, v##dr.data(), 1, (int)v##dr.size())
#define SET_DR_ONLY_USED(ty, dr) XPLMSetData##ty(drTcas##dr, v##dr.data(), 1, (int)(slot-1))
    SET_DR(vi, ModeS);
    SET_DR(vi, ModeC);
    SET_DR(vi, SsrMode);
    SET_DR_ONLY_USED(vf, X);            // must not set/clean unused slots here, otherwise XP12.4+ throws "Traffic plugin error...gave us target with no ID"
    SET_DR_ONLY_USED(vf, Y);
    SET_DR_ONLY_USED(vf, Z);
    SET_DR(vf, VertSpeed);
    SET_DR(vf, Heading);
    SET_DR(vf, Pitch);
    SET_DR(vf, Roll);
    SET_DR(vi, Grnd);
    SET_DR(vf, Gear);
    SET_DR(vf, Flap);
    XPLMSetDatavf(drTcasFlap2, vFlap.data(), 1, (int)vFlap.size());
    SET_DR(vf, Speedbrake);
    SET_DR(vf, Slat);
    SET_DR(vf, WingSweep);
    SET_DR(vf, Throttle);
    SET_DR(vf, YokePitch);
    SET_DR(vf, YokeRoll);
    SET_DR(vf, YokeYaw);
    SET_DR(vi, Lights);
    SET_DR(vf, WakeWingSpan);
    SET_DR(vf, WakeWingArea);
    SET_DR(vi, WakeCat);
    SET_DR(vf, WakeMass);
    SET_DR(vf, WakeAoA);
    SET_DR(vf, WakeLift);

    // return the number of targets
    return slot;
}

/// @brief Assigns slots, ie. planes to places in gSlots
/// @details Called in several passes as to make sure that
///          the lowest n slots are occupied by the closest n planes.
void AIAssignSlots (size_t fromSlot, size_t toSlot)
{
    // 1. Keep planes stable, which are in the given range of slots
    size_t nPlane;
    for (nPlane = fromSlot-1; nPlane < toSlot; ++nPlane) {
        const size_t acSlot = (size_t)vAcByDist.at(nPlane)->GetTcasTargetIdx();
        if (fromSlot <= acSlot && acSlot <= toSlot) {
            gSlots[acSlot] = vAcByDist[nPlane];     // assign plane to same slot again
            vAcByDist[nPlane] = nullptr;            // this plane is handled now
        }
    }
    
    // 2. fill up empty slots with remaining planes
    nPlane = fromSlot-1;                            // points into list of planes
    for (size_t slot=fromSlot;
         slot <= toSlot; ++slot)                    // loop all slots in range
    {
        if (!gSlots[slot])                          // yet empty slot?
        {
            for (;nPlane < toSlot; ++nPlane)        // search for a yet unassigned plane
            {
                if (vAcByDist[nPlane]) {            // found a plane -> assign to slot
                    gSlots[slot] = vAcByDist[nPlane];
                    vAcByDist[nPlane++] = nullptr;
                    break;
                }
            }
        }
    }
    
#ifdef DEBUG
    // it is expected that the range of slots is exactly filled
    // and all planes are used!
    if (!(std::all_of(gSlots.begin()+(long)fromSlot,
                           gSlots.begin()+(long)toSlot,
                           [](const Aircraft* pAc){return pAc!=nullptr;})))
    {
        LOG_MSG(logDEBUG, "Not all gSlots continuously assigned!");
    }
    if (!(std::all_of(vAcByDist.begin()+(long)fromSlot-1,
                           vAcByDist.begin()+(long)toSlot-1,
                           [](const Aircraft* pAc){return pAc==nullptr;})))
    {
        LOG_MSG(logDEBUG, "Not all vAcByDist used!");
    }
#endif
}


// Updates all TCAS target dataRefs, both standard X-Plane,
// as well as additional shared dataRefs for text publishing
void AIMultiUpdate ()
{
    // Some one time stuff
    if (!vecDREdataRefStr.empty())
        AIMultiInformDREs();
    
    // If we don't control AI aircraft we bail out
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // only every few seconds rearrange slots, ie. add/remove planes or
    // move planes between lower and upper section of AI slots:
    if (CheckEverySoOften(tLastSlotSwitching, AISLOT_CHANGE_PERIOD) ||
        glob.mapAc.size() != gMapAcByDist.size())
    {
        // Sort all planes by prioritized distance
        gMapAcByDist.clear();
        for (auto& pair: glob.mapAc) {
            Aircraft& ac = *pair.second;
            // only consider planes that require being shown as AI aircraft
            // (these excludes invisible planes and those with transponder off)
            if (ac.ShowAsAIPlane())
                // Priority distance means that we add artificial distance for higher-numbered AI priorities
                gMapAcByDist.emplace(ac.GetCameraDist() + ac.aiPrio * AI_PRIO_MULTIPLIER,
                                     ac.GetModeS_ID());
            else
                // for non-shown aircraft make sure no slot is remembered
                ac.ResetTcasTargetIdx();
        }
    }
    
    // Aircraft come and go, so the entries in gMapAcByDist can be outdated
    // Here we verify existence of aircraft and compile the definitive
    // list of aircraft to show
    vAcByDist.clear();
    vAcByDist.reserve(numSlots);
    for (const auto& p: gMapAcByDist)
    {
        mapAcTy::const_iterator iterAc = glob.mapAc.find(p.second);
        if (iterAc == glob.mapAc.end() ||       // not found any longer?
            !iterAc->second->ShowAsAIPlane())   // or no longer to be shown on TCAS?
        {
            tLastSlotSwitching = 0.0f;          // ensure we reinit the map next time
            if (iterAc != glob.mapAc.end())     // Clear the TCAS index of no longer to be displayed ac
                iterAc->second->ResetTcasTargetIdx();
        } else {
            // Plane exists!
            // If there's still room: add it to the list
            if (vAcByDist.size() < numSlots)
                vAcByDist.push_back(iterAc->second);
            else
                // else reset the TCAS target index
                iterAc->second->ResetTcasTargetIdx();
        }
    }
    const size_t numAcToShow = vAcByDist.size();
    LOG_ASSERT(numAcToShow <= numSlots);
    
    // Slotting: Organize the planes into the TCAS target/multiplayer slots
    //           in a way that existing planes don't change slot (too often)
    // Prepare the array in which we organize the planes per slot
    // Here, for ease of access, slot 0 (user's plane) is reserved, too
    gSlots.assign(numSlots+1, nullptr);
    
    // There are up to 2 passes:
    std::vector<size_t> vLimits;
    if (GoTCASOverride()) {
        // TCAS override: number of multiplayer dataRefs (19); number of TCAS targets (63)
        if (gMultiRef.size()-1 < numAcToShow)
            vLimits.push_back(gMultiRef.size()-1);
    } else {
        // Classic way: number of configured AI planes (user configured); number of dataRefs (19)
        int nTotal = 0, nActive = 0;
        XPLMPluginID plgId = 0;
        XPLMCountAircraft(&nTotal, &nActive, &plgId);
        --nTotal;                           // don't count user's plane
        if (0 < nTotal && nTotal < (int)numAcToShow)
            vLimits.push_back((size_t)nTotal);
    }
    if (numAcToShow > 0)
        vLimits.push_back(numAcToShow);

    // Optimize the aircraft into their slots
    size_t fromSlot = 1;
    for (size_t toSlot: vLimits) {
        AIAssignSlots(fromSlot, toSlot);
        fromSlot = toSlot+1;
    }
    
    // The new or the old way? -> actually update the dataRefs
    const size_t numTargets =
    GoTCASOverride() ? AIUpdateTCASTargets() : AIUpdateMultiplayerDataRefs();
    
    // now we know how many planes we fed
    XPLMSetActiveAircraftCount(int(numTargets));

    // Cleanup unused datarefs left over now
    if (numTargets < numTargetsLastTime) {
        for (size_t i = (size_t)numTargets; i < std::min(numTargetsLastTime,gMultiRef.size()); i++)
            AIMultiClearAIDataRefs(gMultiRef[i]);
        for (size_t i = (size_t)numTargets; i < std::min(numTargetsLastTime,gInfoRef.size()); i++)
            AIMultiClearInfoDataRefs(gInfoRef[i]);
    }
    
    // remember for next time how many targets we had now
    numTargetsLastTime = numTargets;
}

/// @brief Callback to toggle aircraft count ("TCAS hack")
/// @details We use AI Aircraft to simulate TCAS blibs,
///          but we don't want these AI Aircraft to actually draw.
///          So during aircraft drawing phase we tell X-Plane there are no
///          AI planes, afterwards we put the correct number back
/// @note    Only used if TCAS fallback via classic multiplayer dataRefs is active
int AIMultiControlPlaneCount(
        XPLMDrawingPhase     /*inPhase*/,
        int                  inIsBefore,
        void *               /*inRefcon*/)
{
    // Aren't controlling TCAS???
    if (!glob.bHasControlOfAIAircraft)
        return 1;

    if (inIsBefore)
        XPLMSetActiveAircraftCount(1);
    else
        XPLMSetActiveAircraftCount((int)numTargetsLastTime);

    // Can draw
    return 1;
}

//
// MARK: Control Functions
//

/// @brief Resets all actual values of the AI/multiplayer dataRefs of one plane to something initial
/// @note There as pending bug, filed as XPD-13332, which affects this function with XP12.
///       X-Plane 12 can freeze since b5 when clearing Multiplayer positions to zero.
/// @see https://forums.x-plane.org/index.php?/forums/topic/276319-xp12-on-mac-since-b5-freeze-upon-tcas-activation/
/// @see also https://forums.x-plane.org/index.php?/forums/topic/296584-livetraffic-freezes-xp-1208beta1-on-taking-over-tcas/
///      reporting the same issue for Linux, so we switch initializing off completely.
void AIMultiClearAIDataRefs (multiDataRefsTy& drM,
                             bool bDeactivateToZero)
{
    // not ok dataRefs?
    if (!drM) return;
    
    // XPD-13332 Workaround for XP12: Don't init position, otherwise may freeze
    if (glob.verXPlane < 12000) {
        // either a "far away" location or standard 0, which is, however, a valid location somewhere!
        XPLMSetDataf(drM.X, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
        XPLMSetDataf(drM.Y, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
        XPLMSetDataf(drM.Z, bDeactivateToZero ? 0.0f : FAR_AWAY_VAL_GL);
    }

    XPLMSetDataf(drM.v_x, 0.0f);                // zero speed
    XPLMSetDataf(drM.v_y, 0.0f);
    XPLMSetDataf(drM.v_z, 0.0f);

    XPLMSetDataf(drM.pitch, 0.0f);              // level attitude
    XPLMSetDataf(drM.roll, 0.0f);
    XPLMSetDataf(drM.heading, 0.0f);

    XPLMSetDatavf(drM.gear, F_NULL, 0, 10);     // all configuration reset to 0
    XPLMSetDataf(drM.flap, 0.0f);
    XPLMSetDataf(drM.flap2, 0.0f);
    XPLMSetDataf(drM.spoiler, 0.0f);
    XPLMSetDataf(drM.speedbrake, 0.0f);
    XPLMSetDataf(drM.slat, 0.0f);
    XPLMSetDataf(drM.wingSweep, 0.0f);
    XPLMSetDatavf(drM.throttle, F_NULL, 0, 8);
    if (drM.yoke_pitch) {
        XPLMSetDataf(drM.yoke_pitch, 0.0f);
        XPLMSetDataf(drM.yoke_roll, 0.0f);
        XPLMSetDataf(drM.yoke_yaw, 0.0f);
    }

    XPLMSetDatai(drM.bcnLights, 0);             // all lights off
    XPLMSetDatai(drM.landLights, 0);
    XPLMSetDatai(drM.navLights, 0);
    XPLMSetDatai(drM.strbLights, 0);
    XPLMSetDatai(drM.taxiLights, 0);
}

/// Clears the shared info dataRefs
void AIMultiClearInfoDataRefs (infoDataRefsTy& drI)
{
    // not ok dataRefs?
    if (!drI) return;
    
    // Shared data for providing textual info (see XPMPInfoTexts_t)
    char allNulls[100];
    memset (allNulls, 0, sizeof(allNulls));
    XPLMSetDatab(drI.infoTailNum,       allNulls, 0, sizeof(XPMPInfoTexts_t::tailNum));
    XPLMSetDatab(drI.infoIcaoAcType,    allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAcType));
    XPLMSetDatab(drI.infoManufacturer,  allNulls, 0, sizeof(XPMPInfoTexts_t::manufacturer));
    XPLMSetDatab(drI.infoModel,         allNulls, 0, sizeof(XPMPInfoTexts_t::model));
    XPLMSetDatab(drI.infoIcaoAirline,   allNulls, 0, sizeof(XPMPInfoTexts_t::icaoAirline));
    XPLMSetDatab(drI.infoAirline,       allNulls, 0, sizeof(XPMPInfoTexts_t::airline));
    XPLMSetDatab(drI.infoFlightNum,     allNulls, 0, sizeof(XPMPInfoTexts_t::flightNum));
    XPLMSetDatab(drI.infoAptFrom,       allNulls, 0, sizeof(XPMPInfoTexts_t::aptFrom));
    XPLMSetDatab(drI.infoAptTo,         allNulls, 0, sizeof(XPMPInfoTexts_t::aptTo));
    XPLMSetDatab(drI.cslModel,          allNulls, 0, SDR_CSLMODEL_TXT_SIZE);
}

/// Clears the key (mode_s) of the TCAS target dataRefs
void AIMultiClearTcasDataRefs ()
{
    std::vector<int> nullArr (numSlots, 0);
    if (drTcasModeS) {
        XPLMSetDatavi(drTcasModeS, nullArr.data(), 1, (int)numSlots);
    }
    if (drTcasModeC) {
        XPLMSetDatavi(drTcasModeC, nullArr.data(), 1, (int)numSlots);
    }
    if (drTcasSsrMode) {
        XPLMSetDatavi(drTcasSsrMode, nullArr.data(), 1, (int)numSlots);
    }
}

/// Reset all (controlled) multiplayer dataRef values of all planes
void AIMultiInitAllDataRefs(bool bDeactivateToZero)
{
    if (XPMPHasControlOfAIAircraft()) {
        // TCAS target keys
        AIMultiClearTcasDataRefs();
        
        // Legacy multiplayer dataRefs
        for (multiDataRefsTy& drM: gMultiRef)
            AIMultiClearAIDataRefs(drM, bDeactivateToZero);
        
        // Shared info dataRefs
        for (infoDataRefsTy& drI: gInfoRef)
            AIMultiClearInfoDataRefs(drI);
    }
}

// Inform DRE and DRT of our shared dataRefs
void AIMultiInformDREs ()
{
    if (!vecDREdataRefStr.empty()) {
        // loop over all available data ref editor signatures
        for (const char* szDREditor: DATA_REF_EDITORS) {
            // find the plugin by signature
            XPLMPluginID PluginID = XPLMFindPluginBySignature(szDREditor);
            if (PluginID != XPLM_NO_PLUGIN_ID) {
                // loop over all dataRef strings that are to be infomred and send a msg for them
                for (const std::string& sDataRef: vecDREdataRefStr)
                XPLMSendMessageToPlugin(PluginID,
                                        MSG_ADD_DATAREF,
                                        (void*)sDataRef.c_str());
            }
        }
        // Don't register these again
        vecDREdataRefStr.clear();
    }
}


// Initialize the module
/// @details Fetches all dataRef handles for all dataRefs of all up to 19 AI/multiplayer slots
void AIMultiInit ()
{
    // *** TCAS Target dataRefs ***
    
    // Many of these will only be available with X-Plane 11.50b8 or later
    drTcasOverride      = XPLMFindDataRef("sim/operation/override/override_TCAS");
    drMapOverride       = XPLMFindDataRef("sim/operation/override/override_multiplayer_map_layer");

    // This is probably the most important one, serving as a key,
    // but for us also as an indicator if we are in the right XP version
    // to use TCAS Override
    numSlots = 0;
    drTcasModeS         = XPLMFindDataRef("sim/cockpit2/tcas/targets/modeS_id");
    if (GoTCASOverride())                   // we can use TCAS Override
    {
        // Let's establish array size (we shall not assume it is 64, though initially it is)
        numSlots = (size_t)XPLMGetDatavi(drTcasModeS, nullptr, 0, 0);
        if (numSlots >= 2) {                // expected is 64...just a safety check
            // Reduce by one: The user plane is off limits
            numSlots--;
            
            // Now fetch all the other dataRefs...they should work
            drTcasModeC         = XPLMFindDataRef("sim/cockpit2/tcas/targets/modeC_code");
            drTcasSsrMode       = XPLMFindDataRef("sim/cockpit2/tcas/targets/ssr_mode");
            drTcasFlightId      = XPLMFindDataRef("sim/cockpit2/tcas/targets/flight_id");
            drTcasIcaoType      = XPLMFindDataRef("sim/cockpit2/tcas/targets/icao_type");
            drTcasX             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/x");
            drTcasY             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/y");
            drTcasZ             = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/z");
            drTcasVX            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vx");
            drTcasVY            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vy");
            drTcasVZ            = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vz");
            drTcasVertSpeed     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/vertical_speed");
            drTcasHeading       = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/psi");
            drTcasPitch         = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/the");
            drTcasRoll          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/phi");
            drTcasGrnd          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/weight_on_wheels");
            drTcasGear          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/gear_deploy");
            drTcasFlap          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/flap_ratio");
            drTcasFlap2         = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/flap_ratio2");
            drTcasSpeedbrake    = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/speedbrake_ratio");
            drTcasSlat          = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/slat_ratio");
            drTcasWingSweep     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/wing_sweep");
            drTcasThrottle      = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/throttle");
            drTcasYokePitch     = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yoke_pitch");     // XP12 silently corrected the spelling
            if (!drTcasYokePitch)
                drTcasYokePitch = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_pitch");     // XP11 had copied the original spelling mistake
            drTcasYokeRoll      = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yoke_roll");
            if (!drTcasYokeRoll)
                drTcasYokeRoll  = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_roll");
            drTcasYokeYaw       = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yoke_yaw");
            if (!drTcasYokeYaw)
                drTcasYokeYaw   = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/yolk_yaw");
            drTcasLights        = XPLMFindDataRef("sim/cockpit2/tcas/targets/position/lights");
            // Wake support as of XP12, so these can fail in earlier versions:
            drTcasWakeWingSpan  = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/wing_span_m");
            drTcasWakeWingArea  = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/wing_area_m2");
            drTcasWakeCat       = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/wake_cat");
            drTcasWakeMass      = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/mass_kg");
            drTcasWakeAoA       = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/aoa");
            drTcasWakeLift      = XPLMFindDataRef("sim/cockpit2/tcas/targets/wake/lift_N");
        } else {
            // not expected to happen, but safety measure: fallback to classic TCAS
            drTcasModeS = nullptr;
            numSlots = 0;
        }
    }
    
    // *** Legacy Multiplayer DataRefs ***
    
    // We need them only for proper initialization at the beginning
    // and when not filling all (19) slots
    char        buf[100];
    gMultiRef.clear();                      // just a safety measure against multi init
    multiDataRefsTy drM;                    // one set of dataRefs for one plane
    gMultiRef.push_back(drM);               // add an empty record at position 0 (user's plane)

    // Code for finding a standard X-Plane dataRef
#define FIND_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);    \
    drM.membVar = XPLMFindDataRef(buf);

    // We don't know how many multiplayer planes there are - fetch as many as we can.
    // Loop over all possible AI/multiplayer slots
    for (unsigned nDrM = 1; true; nDrM++)
    {
        drM.clear();
        // position
        FIND_PLANE_DR(X,                    "x",                    nDrM);
        if (!drM.X) break;
        FIND_PLANE_DR(Y,                    "y",                    nDrM);
        FIND_PLANE_DR(Z,                    "z",                    nDrM);
        // cartesian velocities
        FIND_PLANE_DR(v_x,                  "v_x",                  nDrM);
        FIND_PLANE_DR(v_y,                  "v_y",                  nDrM);
        FIND_PLANE_DR(v_z,                  "v_z",                  nDrM);
        // attitude
        FIND_PLANE_DR(pitch,                "the",                  nDrM);
        FIND_PLANE_DR(roll,                 "phi",                  nDrM);
        FIND_PLANE_DR(heading,              "psi",                  nDrM);
        // configuration
        FIND_PLANE_DR(gear,                 "gear_deploy",          nDrM);
        FIND_PLANE_DR(flap,                 "flap_ratio",           nDrM);
        FIND_PLANE_DR(flap2,                "flap_ratio2",          nDrM);
        FIND_PLANE_DR(spoiler,              "spoiler_ratio",        nDrM);
        FIND_PLANE_DR(speedbrake,           "speedbrake_ratio",     nDrM);
        FIND_PLANE_DR(slat,                 "slat_ratio",           nDrM); // _should_ expect this name
        if (!drM.slat) {
            FIND_PLANE_DR(slat,             "sla1_ratio",           nDrM); // but in reality it is this
        }
        FIND_PLANE_DR(wingSweep,            "wing_sweep",           nDrM);
        FIND_PLANE_DR(throttle,             "throttle",             nDrM);
        
        // In XP12 the following classic dataRefs are considered legacy and throw user-visible warnings if used
        // I'm too lazy to use the (proper) array sim/multiplayer/controls/yoke_pitch_ratio[20] instead
        // due to that being an array dataRefs as opposed to these here using plane%u semantics.
        // But in XP12 we don't need the legacy dataRefs anyway, so we can live without these 3 if we have TCAS override:
        if (!numSlots) {
            FIND_PLANE_DR(yoke_pitch,       "yolk_pitch",           nDrM);
            FIND_PLANE_DR(yoke_roll,        "yolk_roll",            nDrM);
            FIND_PLANE_DR(yoke_yaw,         "yolk_yaw",             nDrM);
        }
        
        // lights
        FIND_PLANE_DR(bcnLights,            "beacon_lights_on",     nDrM);
        FIND_PLANE_DR(landLights,           "landing_lights_on",    nDrM);
        FIND_PLANE_DR(navLights,            "nav_lights_on",        nDrM);
        FIND_PLANE_DR(strbLights,           "strobe_lights_on",     nDrM);
        FIND_PLANE_DR(taxiLights,           "taxi_light_on",        nDrM);
        if (!drM) break;                    // break out of loop once the slot doesn't exist
        gMultiRef.push_back(drM);
    }
    
    // If using TCAS fallback with multiplayer dataRefs
    // then we set the number of slots based on available multiplayer slots
    if (!numSlots)
        numSlots = gMultiRef.size()-1;
    
    // *** Shared dataRefs for providing additional text info ***
    
    // While these had been defined to be 19 only,
    // we extend it to as many as there are TCAS targets
    gInfoRef.clear();
    infoDataRefsTy  drI;            // one set of dataRefs for one plane
    gInfoRef.push_back(drI);        // add an empty record at position 0 (user's plane)

    // Code for finding a non-standard shared dataRef for text information sharing
#define SHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                                    \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);   \
    if (XPLMShareData(buf, xplmType_Data, NULL, NULL)) {                                \
        drI.membVar = XPLMFindDataRef(buf);                                             \
        vecDREdataRefStr.push_back(buf);                                                \
    }                                                                                   \
    else drI.membVar = NULL;
    
    // We add as many shared info dataRefs as there are TCAS targets (probably 63)
    // or alternatively standard multiplayer dataRefs
    for (unsigned n = 1; n <= numSlots; n++)
    {
        // Shared data for providing textual info (see XPMPInfoTexts_t)
        SHARE_PLANE_DR(infoTailNum,         "tailnum",              n);
        SHARE_PLANE_DR(infoIcaoAcType,      "ICAO",                 n);
        SHARE_PLANE_DR(infoManufacturer,    "manufacturer",         n);
        SHARE_PLANE_DR(infoModel,           "model",                n);
        SHARE_PLANE_DR(infoIcaoAirline,     "ICAOairline",          n);
        SHARE_PLANE_DR(infoAirline,         "airline",              n);
        SHARE_PLANE_DR(infoFlightNum,       "flightnum",            n);
        SHARE_PLANE_DR(infoAptFrom,         "apt_from",             n);
        SHARE_PLANE_DR(infoAptTo,           "apt_to",               n);
        SHARE_PLANE_DR(cslModel,            "cslModel",             n);
        gInfoRef.push_back(drI);
    }
}

// Grace cleanup
/// @details Make sure we aren't in control, then Unshare the shared dataRefs
void AIMultiCleanup ()
{
    // Make sure we are no longer in control
    XPMPMultiplayerDisable();
    
    // Unshare shared data
#define UNSHARE_PLANE_DR(membVar, dataRefTxt, PlaneNr)                         \
    snprintf(buf,sizeof(buf),"sim/multiplayer/position/plane%u_" dataRefTxt,PlaneNr);       \
    XPLMUnshareData(buf, xplmType_Data, NULL, NULL);                           \
    drI.membVar = NULL;
    
    char        buf[100];
    for (unsigned n = 1; n <= gInfoRef.size(); n++)
    {
        infoDataRefsTy& drI = gInfoRef[(size_t)n-1];
        UNSHARE_PLANE_DR(infoTailNum,         "tailnum",        n);
        UNSHARE_PLANE_DR(infoIcaoAcType,      "ICAO",           n);
        UNSHARE_PLANE_DR(infoManufacturer,    "manufacturer",   n);
        UNSHARE_PLANE_DR(infoModel,           "model",          n);
        UNSHARE_PLANE_DR(infoIcaoAirline,     "ICAOairline",    n);
        UNSHARE_PLANE_DR(infoAirline,         "airline",        n);
        UNSHARE_PLANE_DR(infoFlightNum,       "flightnum",      n);
        UNSHARE_PLANE_DR(infoAptFrom,         "apt_from",       n);
        UNSHARE_PLANE_DR(infoAptTo,           "apt_to",         n);
        UNSHARE_PLANE_DR(cslModel,            "cslModel",       n);
    }
    gInfoRef.clear();
    
    // Cleanup other arrays properly before shutdown
    gMultiRef.clear();
}

}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Acquire control of multiplayer aircraft
const char *    XPMPMultiplayerEnable(void (*_callback)(void*),
                                      void*  _refCon )
{
    static char szWarn[400];

    // short-cut if we are already in control
    if (XPMPHasControlOfAIAircraft())
        return "";
    
    // Override in global config?
    if (glob.eAIOverride == SWITCH_CFG_OFF) {
        LOG_MSG(logDEBUG, "TCAS/AI Control enforced OFF in an XPMP2.prf config file");
        return "";
    }
    
    // We try up to 2 times...if a controlling plugin release control
    // immediately (based on processing the XPLM_MSG_RELEASE_PLANES message)
    // then we have a chance when trying again right away
    for ([[maybe_unused]] int i: {1,2})
    {
        // Attempt to grab multiplayer planes, then analyze.
        glob.bHasControlOfAIAircraft = XPLMAcquirePlanes(NULL, _callback, _refCon) != 0;
        if (glob.bHasControlOfAIAircraft)
        {
            if (GoTCASOverride()) {
                // We definitely want to override TCAS and map!
                XPLMSetDatai(drTcasOverride, 1);
                XPLMSetDatai(drMapOverride, 1);
            } else {
                // We disable AI on all planes
                for (int nPlane = 1; nPlane < (int)gMultiRef.size(); ++nPlane)
                    XPLMDisableAIForPlane(nPlane);
                
                // Register the 'number of planes' control calls.
                XPLMRegisterDrawCallback(AIMultiControlPlaneCount,
                    XPLM_PHASE_AIRPLANES, 1 /* before*/, nullptr);

                XPLMRegisterDrawCallback(AIMultiControlPlaneCount,
                    XPLM_PHASE_AIRPLANES, 0 /* after */, nullptr);

            }

            // No Planes yet started, initialize all dataRef values for a clean start
            XPLMSetActiveAircraftCount(1);
            AIMultiInitAllDataRefs(false);

            // Success
            LOG_MSG(logINFO, INFO_AI_CONTROL);
            return "";
        }
        else
        {
            // Failed! Because of whom?
            int total=0, active=0;
            XPLMPluginID who=0;
            char whoName[256];
            char signature[256] = "";
            XPLMCountAircraft(&total, &active, &who);
            
            // Maybe the controlling plugin released control immediately,
            // in that case "who" is now -1
            if (who >= 0)
            {
                XPLMGetPluginInfo(who, whoName, nullptr, signature, nullptr);
            
                // If it is the Remote Client we don't need to warn...the Remote Client will show our planes
                if (strncmp(signature, REMOTE_SIGNATURE, sizeof(signature)) == 0) {
                    LOG_MSG(logINFO, INFO_REMOTE_CLIENT_AI);
                    return INFO_REMOTE_CLIENT_AI;
                }

                // Write a proper message and return it also to caller
                snprintf(szWarn, sizeof(szWarn), WARN_NO_AI_CONTROL,
                         whoName, glob.pluginName.c_str());
                LOG_MSG(logWARN, "%s", szWarn);
                return szWarn;
            }
        }
    }
    
    // We don't know who...but somebody keeps blocking us
    snprintf(szWarn, sizeof(szWarn), WARN_NO_AI_CONTROL,
             "An unknown plugin", glob.pluginName.c_str());
    LOG_MSG(logWARN, "%s", szWarn);
    return szWarn;
}

// Release control of multiplayer aircraft
void XPMPMultiplayerDisable()
{
    // short-cut if we are not in control
    if (!XPMPHasControlOfAIAircraft())
        return;
    
    // Override in global config?
    if (glob.eAIOverride == SWITCH_CFG_ON) {
        LOG_MSG(logDEBUG, "TCAS/AI Control enforced ON in an XPMP2.prf config file");
        return;
    }
    
    // Cleanup our values
    XPLMSetActiveAircraftCount(1);      // no active AI plane
    AIMultiInitAllDataRefs(true);       // reset all dataRef values to zero
    
    // Reset the last index used of all planes
    for (auto& pair: glob.mapAc)
        pair.second->ResetTcasTargetIdx();

    // Then fully release AI/multiplayer planes
    if (GoTCASOverride()) {
        XPLMSetDatai(drMapOverride, 0);
        XPLMSetDatai(drTcasOverride, 0);
    } else {
        // Remove control callbacks
        XPLMUnregisterDrawCallback(AIMultiControlPlaneCount, XPLM_PHASE_AIRPLANES, 1, nullptr);
        XPLMUnregisterDrawCallback(AIMultiControlPlaneCount, XPLM_PHASE_AIRPLANES, 0, nullptr);
    }
    XPLMReleasePlanes();
    glob.bHasControlOfAIAircraft = false;
    LOG_MSG(logINFO, INFO_AI_CONTROL_ENDS);
}

// Do we control AI planes?
bool XPMPHasControlOfAIAircraft()
{
    return glob.bHasControlOfAIAircraft;
}


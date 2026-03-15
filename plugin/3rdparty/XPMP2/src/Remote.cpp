/// @file       Remote.cpp
/// @brief      Master/Client communication for aircraft synchronization
///             on remote networked instances
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

#if APL == 1 || LIN == 1
#include <unistd.h>                     // for pipe
#include <sys/fcntl.h>                  // for F_SETFL, O_NONBLOCK
#endif

namespace XPMP2 {

//
// MARK: dataRefs packing
//

/// Defines per dataRef the value range we support transferring to the remote client
const std::array<RemoteDataRefPackTy,V_COUNT> REMOTE_DR_DEF = { {
    {   0.0f,     1.0f},        // V_CONTROLS_GEAR_RATIO = 0,                `libxplanemp/controls/gear_ratio` and \n`sim/cockpit2/tcas/targets/position/gear_deploy`
    {-127.0f,   128.0f},        // V_CONTROLS_NWS_RATIO,                     `libxplanemp/controls/nws_ratio`, the nose wheel angle, actually in degrees
    {   0.0f,     1.0f},        // V_CONTROLS_FLAP_RATIO,                    `libxplanemp/controls/flap_ratio` and \n`sim/cockpit2/tcas/targets/position/flap_ratio` and `...flap_ratio2`
    {   0.0f,     1.0f},        // V_CONTROLS_SPOILER_RATIO,                 `libxplanemp/controls/spoiler_ratio`
    {   0.0f,     1.0f},        // V_CONTROLS_SPEED_BRAKE_RATIO,             `libxplanemp/controls/speed_brake_ratio` and \n`sim/cockpit2/tcas/targets/position/speedbrake_ratio`
    {   0.0f,     1.0f},        // V_CONTROLS_SLAT_RATIO,                    `libxplanemp/controls/slat_ratio` and \n`sim/cockpit2/tcas/targets/position/slat_ratio`
    {   0.0f,     1.0f},        // V_CONTROLS_WING_SWEEP_RATIO,              `libxplanemp/controls/wing_sweep_ratio` and \n`sim/cockpit2/tcas/targets/position/wing_sweep`
    {   0.0f,     1.0f},        // V_CONTROLS_THRUST_RATIO,                  `libxplanemp/controls/thrust_ratio` and \n`sim/cockpit2/tcas/targets/position/throttle`
    {   0.0f,     1.0f},        // V_CONTROLS_YOKE_PITCH_RATIO,              `libxplanemp/controls/yoke_pitch_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_pitch`
    {   0.0f,     1.0f},        // V_CONTROLS_YOKE_HEADING_RATIO,            `libxplanemp/controls/yoke_heading_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_yaw`
    {   0.0f,     1.0f},        // V_CONTROLS_YOKE_ROLL_RATIO,               `libxplanemp/controls/yoke_roll_ratio` and \n`sim/cockpit2/tcas/targets/position/yolk_roll`
    {   0.0f,     1.0f},        // V_CONTROLS_THRUST_REVERS,                 `libxplanemp/controls/thrust_revers`
    {   0.0f,     1.0f},        // V_CONTROLS_TAXI_LITES_ON,                 `libxplanemp/controls/taxi_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    {   0.0f,     1.0f},        // V_CONTROLS_LANDING_LITES_ON,              `libxplanemp/controls/landing_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    {   0.0f,     1.0f},        // V_CONTROLS_BEACON_LITES_ON,               `libxplanemp/controls/beacon_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    {   0.0f,     1.0f},        // V_CONTROLS_STROBE_LITES_ON,               `libxplanemp/controls/strobe_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    {   0.0f,     1.0f},        // V_CONTROLS_NAV_LITES_ON,                  `libxplanemp/controls/nav_lites_on` and \n`sim/cockpit2/tcas/targets/position/lights`
    {   0.0f,    10.0f},        // V_GEAR_NOSE_GEAR_DEFLECTION_MTR,          `libxplanemp/gear/nose_gear_deflection_mtr`
    {   0.0f,    10.0f},        // V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR,      `libxplanemp/gear/tire_vertical_deflection_mtr`
    {   0.0f,   360.0f},        // V_GEAR_TIRE_ROTATION_ANGLE_DEG,           `libxplanemp/gear/tire_rotation_angle_deg`
    {   0.0f,  1000.0f},        // V_GEAR_TIRE_ROTATION_SPEED_RPM,           `libxplanemp/gear/tire_rotation_speed_rpm`
    {   0.0f,   100.0f},        // V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC,       `libxplanemp/gear/tire_rotation_speed_rad_sec`
    {   0.0f,   360.0f},        // V_ENGINES_ENGINE_ROTATION_ANGLE_DEG,      `libxplanemp/engines/engine_rotation_angle_deg`
    {   0.0f, 15000.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RPM,      `libxplanemp/engines/engine_rotation_speed_rpm`
    {   0.0f,  1500.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC,  `libxplanemp/engines/engine_rotation_speed_rad_sec`
    {   0.0f,   260.0f},        // V_ENGINES_PROP_ROTATION_ANGLE_DEG,        `libxplanemp/engines/prop_rotation_angle_deg`
    {   0.0f,  3000.0f},        // V_ENGINES_PROP_ROTATION_SPEED_RPM,        `libxplanemp/engines/prop_rotation_speed_rpm`
    {   0.0f,   300.0f},        // V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC,    `libxplanemp/engines/prop_rotation_speed_rad_sec`
    {   0.0f,     1.0f},        // V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO,   `libxplanemp/engines/thrust_reverser_deploy_ratio`
    {   0.0f,   360.0f},        // V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1,     `libxplanemp/engines/engine_rotation_angle_deg1`
    {   0.0f,   360.0f},        // V_ENGINES_ENGINE_ROTATION_ANGLE_DEG2,     `libxplanemp/engines/engine_rotation_angle_deg2`
    {   0.0f,   360.0f},        // V_ENGINES_ENGINE_ROTATION_ANGLE_DEG3,     `libxplanemp/engines/engine_rotation_angle_deg3`
    {   0.0f,   360.0f},        // V_ENGINES_ENGINE_ROTATION_ANGLE_DEG4,     `libxplanemp/engines/engine_rotation_angle_deg4`
    {   0.0f, 15000.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RPM1,     `libxplanemp/engines/engine_rotation_speed_rpm1`
    {   0.0f, 15000.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RPM2,     `libxplanemp/engines/engine_rotation_speed_rpm2`
    {   0.0f, 15000.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RPM3,     `libxplanemp/engines/engine_rotation_speed_rpm3`
    {   0.0f, 15000.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RPM4,     `libxplanemp/engines/engine_rotation_speed_rpm4`
    {   0.0f,  1500.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1, `libxplanemp/engines/engine_rotation_speed_rad_sec1`
    {   0.0f,  1500.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2, `libxplanemp/engines/engine_rotation_speed_rad_sec2`
    {   0.0f,  1500.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3, `libxplanemp/engines/engine_rotation_speed_rad_sec3`
    {   0.0f,  1500.0f},        // V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4, `libxplanemp/engines/engine_rotation_speed_rad_sec4`
    {   0.0f,     1.0f},        // V_MISC_TOUCH_DOWN,                        `libxplanemp/misc/touch_down`
} };


//
// MARK: ENQUEUE: Data Cache (XP Main Thread)
//

// Constant definitions
constexpr int   REMOTE_RECV_BEACON_INTVL    = 15;       ///< How often to send an Interest Beacon? [s]
constexpr float REMOTE_SEND_AC_DETAILS_INTVL= 10.f;     ///< How often to send full a/c details? [s]

/// Array holding all dataRef names, defined in Aircraft.cpp
extern std::vector<const char*> DR_NAMES;

// Global variables
mapRmtAcCacheTy gmapRmtAcCache;     ///< Cache for last data sent out to the network
unsigned gFullUpdDue = 0;           ///< What's the full update group that has its turn now?
unsigned gFullUpdLastDone = 0;      ///< the last actually processed full update group
unsigned gNxtFullUpdGrpToAssign = 0;///< What's the next group number to assign to the next a/c? (Assigned will be the value incremented by 1)
float gNow = 0.0f;                  ///< Current network timestamp
float gNxtTxfTime = 0.0f;           ///< When to actually process position updates next?

queueRmtDataTy gqueueRmtData;       ///< the queue for passing data from main to network thread
std::condition_variable gcvRmtData; ///< notifies the network thread of available data to be processed
std::mutex gmutexRmtData;           ///< protects modifying access to the queue and the condition variable
/// The lock that we keep during handling of the flight loop
std::unique_lock<std::mutex> glockRmtData(gmutexRmtData);


// Constructor copies relevant values from the passed-in aircraft
RmtAcCacheTy::RmtAcCacheTy (const Aircraft& ac,
                            double _lat, double _lon, double _alt_ft) :
fullUpdGrp(++gNxtFullUpdGrpToAssign), ts(gNow),
lat(_lat), lon(_lon), alt_ft(_alt_ft),
drawInfo(ac.drawInfo), v(ac.v),
bValid(ac.IsValid()), bVisible(ac.IsVisible()), bRender(ac.IsRendered()),
pCSLMdl(ac.GetModel())
{
    // roll over on the next-to-assign group
    if (gNxtFullUpdGrpToAssign >= unsigned(REMOTE_SEND_AC_DETAILS_INTVL))
        gNxtFullUpdGrpToAssign = 0;
}

// Updates current values from given aircraft
void RmtAcCacheTy::UpdateFrom (const Aircraft& ac,
                               double _lat, double _lon, double _alt_ft)
{
    ts = gNow;                      // is valid right now
    lat = _lat;
    lon = _lon;
    alt_ft = _alt_ft;
    drawInfo = ac.drawInfo;
    v = ac.v;
    bValid      = ac.IsValid();
    bVisible    = ac.IsVisible();
    bRender     = ac.IsRendered();
    pCSLMdl     = ac.GetModel();
}


//
// MARK: Message Types
//

// --- RemoteMsgBaseTy ---

RemoteMsgBaseTy::RemoteMsgBaseTy (RemoteMsgTy _ty, std::uint8_t _ver) :
msgTy(_ty), msgVer(_ver),
bLocalSender(false), filler1(0),
pluginId(std::uint8_t(glob.pluginId & 0xFFFF))
{}

// --- RemoteMsgBeaconTy ---

RemoteMsgBeaconTy::RemoteMsgBeaconTy() :
RemoteMsgBaseTy(RMT_MSG_INTEREST_BEACON, RMT_VER_BEACON)
{}

// --- RemoteMsgSettingsTy ---

RemoteMsgSettingsTy::RemoteMsgSettingsTy () :
RemoteMsgBaseTy(RMT_MSG_SETTINGS, RMT_VER_SETTINGS)
{
    // set everything after the header to zero
    memset(reinterpret_cast<char*>(this) + sizeof(RemoteMsgBaseTy),0,sizeof(*this) - sizeof(RemoteMsgBaseTy));
    maxLabelDist = 0.0f;
}

// --- RemoteAcDetailTy ---

// Default Constructor sets all to zero
RemoteAcDetailTy::RemoteAcDetailTy ()
{
    // set everything to zero
    memset(this,0,sizeof(*this));
    lat = lon = 0.0;
    alt_ft = 0.0f;
}

// A/c copy constructor fills from passed-in XPMP2::Aircraft object
RemoteAcDetailTy::RemoteAcDetailTy (const Aircraft& _ac,
                                    double _lat, double _lon, float _alt_ft,
                                    std::uint16_t _dTime)
{
    // set everything to zero
    memset(this, 0, sizeof(*this));
    lat = lon = 0.0;
    alt_ft = 0.0f;
    // then copy from a/c
    CopyFrom(_ac, _lat, _lon, _alt_ft, _dTime);
}

// Copies values from passed-in XPMP2::Aircraft object
void RemoteAcDetailTy::CopyFrom (const Aircraft& _ac,
                                 double _lat, double _lon, float _alt_ft,
                                 std::uint16_t _dTime)
{
    modeS_id = _ac.GetModeS_ID();
    strncpy(icaoType,   _ac.acIcaoType.c_str(),                 sizeof(icaoType));
    strncpy(icaoOp,     _ac.acIcaoAirline.c_str(),              sizeof(icaoOp));
    strncpy(sShortId,   _ac.GetModel()->GetShortId().c_str(),   sizeof(sShortId));
    pkgHash = _ac.GetModel()->pkgHash;
    strncpy(label,      _ac.label.c_str(),                      sizeof(label));
    SetLabelCol(_ac.colLabel);
    
    // World Position
    lat = _lat;
    lon = _lon;
    alt_ft = _alt_ft;

    // Attitude: Copy from drawInfo, but converted to smaller 16 bit types
    SetPitch    (_ac.drawInfo.pitch);
    SetHeading  (_ac.drawInfo.heading);
    SetRoll     (_ac.drawInfo.roll);

    aiPrio      = std::int16_t(_ac.aiPrio);
    dTime       = _dTime;
    bValid      = _ac.IsValid();
    bVisible    = _ac.IsVisible();
    bRender     = _ac.IsRendered();
    // Labels are only to be drawn if both individually (per a/c) and globally they shall:
    bDrawLabel  = _ac.ShallDrawLabel() && XPMPDrawingAircraftLabels();
    bOnGrnd     = _ac.IsOnGrnd();
    contrailNum = std::uint8_t(std::min<unsigned>(8,_ac.contrailNum));
    
    // Info texts
#define memcpy_min(to,from) std::memcpy(to,from,std::min(sizeof(from),sizeof(to)))
    memcpy_min(tailNum,         _ac.acInfoTexts.tailNum);
    memcpy_min(manufacturer,    _ac.acInfoTexts.manufacturer);
    memcpy_min(model,           _ac.acInfoTexts.model);
    memcpy_min(airline,         _ac.acInfoTexts.airline);
    memcpy_min(flightNum,       _ac.acInfoTexts.flightNum);
    memcpy_min(aptFrom,         _ac.acInfoTexts.aptFrom);
    memcpy_min(aptTo,           _ac.acInfoTexts.aptTo);

    contrailDist_m = (std::uint8_t)std::min<unsigned>(255,_ac.contrailDist_m);
    contrailLifeTime = (std::uint8_t)std::min<unsigned>(255,_ac.contrailLifeTime);
    
    // dataRef Animation values converted to uint8
    for (size_t i = 0; i < XPMP2::V_COUNT; ++i)
        v[i] = REMOTE_DR_DEF[i].pack(_ac.v[i]);
}

// set the label color from a float array (4th number, alpha, is always considered 1.0)
void RemoteAcDetailTy::SetLabelCol (const float _col[4])
{
    labelCol[0] = std::uint8_t(_col[0] * 255.0f);
    labelCol[1] = std::uint8_t(_col[1] * 255.0f);
    labelCol[2] = std::uint8_t(_col[2] * 255.0f);
}

// writes color out into a float array
void RemoteAcDetailTy::GetLabelCol (float _col[4]) const
{
    _col[0] = float(labelCol[0]) / 255.0f;
    _col[1] = float(labelCol[1]) / 255.0f;
    _col[2] = float(labelCol[2]) / 255.0f;
    _col[3] = 1.0f;
}

// Sets heading from float
void RemoteAcDetailTy::SetHeading (float _h)
{
    heading = std::uint16_t(headNormalize(_h) * 100.0f);
}

// --- V0 of RemoteAcDetailTy ---

#ifdef _MSC_VER                                 // Visual C++
#pragma pack(push,1)                            // set packing once (ie. not per struct)
#endif

/// Version 0 A/C details, packed into an array message
struct RemoteAcDetailTy_v0 {
    std::uint32_t   modeS_id;           ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
    char            icaoType[4];        ///< icao a/c type
    char            icaoOp[4];          ///< icao airline code
    char            sShortId[20];       ///< CSL model's short id
    std::uint16_t   pkgHash;            ///< hash value of package name
    char            label[23];          ///< label
    std::uint8_t    labelCol[3];        ///< label color (RGB)
    float           alt_ft;             ///< [ft] current altitude
    // ^ the above has 64 bytes, so that these doubles start on an 8-byte bounday:
    double          lat;                ///< latitude
    double          lon;                ///< longitude
    std::int16_t    pitch;              ///< [0.01°] pitch/100
    std::uint16_t   heading;            ///< [0.01°] heading/100
    std::int16_t    roll;               ///< [0.01°] roll/100
    
    std::int16_t    aiPrio;             ///< priority for display in limited TCAS target slots, `-1` indicates "no TCAS display"
    std::uint16_t   dTime;              ///< [0.0001s] time difference to previous position in 1/10000s
    bool            bValid : 1;         ///< is this object valid? (Will be reset in case of exceptions)
    bool            bVisible : 1;       ///< Shall this plane be drawn at the moment?
    bool            bRender : 1;        ///< Shall the CSL model be drawn in 3D world?

    std::uint8_t    filler[3];          ///< yet unused
    
    ///< Array of _packed_ dataRef values for CSL model animation
    std::uint8_t    v[XPMP2::V_COUNT];    // 42

    static constexpr size_t msgSize () { return sizeof(RemoteAcDetailTy_v0); }   ///< message size
} PACKED;

/// Version 0 A/C detail message, has an inherited header plus an array of XPMP2::RemoteAcDetailTy elements
struct RemoteMsgAcDetailTy_v0 : public RemoteMsgBaseTy {
    RemoteAcDetailTy_v0 arr[1];         ///< basis for the array of actual details
    
    /// Convert msg len to number of arr elements
    static constexpr size_t NumElem (size_t _msgLen) { return (_msgLen - sizeof(RemoteMsgBaseTy)) / sizeof(RemoteAcDetailTy_v0); }
    
    /// @brief Convert v0 to v1 message, must be freed after use!
    /// @param _msgLenV0 Message length of this message, ie. the v0 version
    /// @param[out] _msgLen receives the message length of the converted message
    /// @returns pointer to the converted message
    RemoteMsgAcDetailTy* convert (size_t _msgLenV0, size_t& _msgLen) const;
} PACKED;

#ifdef _MSC_VER                                 // Visual C++
#pragma pack(pop)                               // reseting packing
#endif

static_assert(sizeof(RemoteAcDetailTy_v0)   ==  94+42,  "RemoteAcDetailTy_v0 doesn't have expected size");
static_assert(sizeof(RemoteMsgAcDetailTy_v0)== 102+42,  "RemoteMsgAcDetailTy_v0 doesn't have expected size");

#if defined(__GNUC__) && (__GNUC__ >= 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

// Convert v0 to v1 message, must be freed after use!
/// @details Mallocates a memory chunk large enough for the v1 message
///          (which could be larger than a real-life v1 message!)
///          and converts each detail array element into it.
RemoteMsgAcDetailTy* RemoteMsgAcDetailTy_v0::convert(size_t _msgLenV0,
                                                     size_t& _msgLen) const
{
    // Allocate a memory chunk for the converted message
    const size_t numElems = NumElem(_msgLenV0);
    _msgLen = sizeof(RemoteMsgBaseTy) + numElems * sizeof(RemoteAcDetailTy);
    RemoteMsgAcDetailTy* pMsg = (RemoteMsgAcDetailTy*)std::malloc(_msgLen);
    LOG_ASSERT(pMsg);
    assert(pMsg->NumElem(_msgLen) == numElems);
    std::memset(pMsg, 0, _msgLen);
    
    // Copy the header, which is identical, except for the version number
    std::memcpy(pMsg, this, sizeof(RemoteMsgBaseTy));
    pMsg->msgVer = RMT_VER_AC_DETAIL;
    
    // Convert each array element
    const RemoteAcDetailTy_v0* pElemV0 = arr;
    RemoteAcDetailTy* pElem = pMsg->arr;
    for (size_t i = 0;
         i < numElems;
         ++pElemV0, ++pElem, ++i)
    {
        // the first 91 bytes are identical
        std::memcpy(pElem, pElemV0, 91);
        // then copy the dataRef array
        std::memcpy(pElem->v, pElemV0->v, sizeof(pElem->v));
    }
    
    return pMsg;
}

#if defined(__GNUC__) && (__GNUC__ >= 9)
#pragma GCC diagnostic pop
#endif

// --- RemoteAcPosUpdateTy ---

RemoteAcPosUpdateTy::RemoteAcPosUpdateTy (XPMPPlaneID _modeS_id,
                                          std::int16_t _dLat,
                                          std::int16_t _dLon,
                                          std::int16_t _dAlt_ft,
                                          std::uint16_t _dTime,
                                          float _pitch, float _heading, float _roll) :
modeS_id(_modeS_id),
dLat(_dLat), dLon(_dLon), dAlt_ft(_dAlt_ft),
dTime(_dTime),
filler1(0)
{
    SetPitch(_pitch);
    SetHeading(_heading);
    SetRoll(_roll);
}

// Sets heading from float
void RemoteAcPosUpdateTy::SetHeading (float _h)
{
    heading = std::uint16_t(headNormalize(_h) * 100.0f);
}

//
// MARK: SENDING Remote Data (Worker Thread)
//

#define INFO_MC_SEND_WAIT   "Listening to %s, waiting for someone being interested in our data..."
#define INFO_MC_SEND_RCVD   "Received word from %s, will start sending aircraft data"

#define ERR_MC_THREAD       "Exception in multicast handling: %s"
#define ERR_MC_MAX          "Too many errors, I give up on remote functionality!"
constexpr int MC_MAX_ERR=5;             ///< after this many errors we no longer try listening

std::thread gThrMC;                     ///< remote listening/sending thread
UDPMulticast* gpMc = nullptr;           ///< multicast socket for listening/sending (destructor uses locks, which don't work during module shutdown, so can't create a global object due to its exit-time destructor)
volatile bool gbStopMCThread = false;   ///< Shall the network thread stop?
bool gbRemoteGaveUp = false;            ///< No longer try remote functionality, have seen to many errors

#if APL == 1 || LIN == 1
/// the self-pipe to shut down the APRS thread gracefully
SOCKET gSelfPipe[2] = { INVALID_SOCKET, INVALID_SOCKET };
#endif

/// Timestamp when we sent our settings the last time
float gSendSettingsLast = 0.0f;

// Messages waiting to be filled and send, all having a size of glob.remoteBufSize
RmtMsgBufTy<RemoteAcDetailTy,RMT_MSG_AC_DETAILED,RMT_VER_AC_DETAIL> gMsgAcDetail;               ///< A/C Detail message
RmtMsgBufTy<RemoteAcPosUpdateTy,RMT_MSG_AC_POS_UPDATE,RMT_VER_AC_POS_UPDATE> gMsgAcPosUpdate;   ///< A/C Position Update message
RmtMsgBufTy<RemoteAcAnimTy, RMT_MSG_AC_ANIM, RMT_VER_AC_ANIM> gMsgAcAnim;                       ///< A/C Animation dataRefs message
RmtMsgBufTy<RemoteAcRemoveTy,RMT_MSG_AC_REMOVE,RMT_VER_AC_REMOVE> gMsgAcRemove;                 ///< A/C Removal message


// Free up the buffer, basically a reset
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
void RmtMsgBufTy<ElemTy,MsgTy,msgVer>::free ()
{
    if (pMsg) std::free (pMsg);
    pMsg = nullptr;
    elemCount = 0;
    size = 0;
}

// If necessary allocate the required buffer, then initialize it to an empty message
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
void RmtMsgBufTy<ElemTy,MsgTy,msgVer>::init ()
{
    // if buffer does not exist: create it
    if (!pMsg) {
        pMsg = malloc(glob.remoteBufSize);
        LOG_ASSERT(pMsg);
    }
    // initialize the buffer to an empty msg
    elemCount = 0;
    std::memset(pMsg, 0, glob.remoteBufSize);
    // overwrite with a standard initialized message, will set msg type, for example
    *reinterpret_cast<RemoteMsgBaseTy*>(pMsg) = RemoteMsgBaseTy(MsgTy,msgVer);
    size = sizeof(RemoteMsgBaseTy);     // msg hdr is defined
}

// Add another element to the buffer, returns if now full
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
bool RmtMsgBufTy<ElemTy,MsgTy,msgVer>::add (const ElemTy& _elem)
{
    // no buffer defined yet? -> do so!
    if (!pMsg) init();
        
    // no space left?
    const size_t elemSize = _elem.msgSize();
    if (glob.remoteBufSize - size < elemSize)
        return false;
    
    // Copy the element into the msg buffer
    memcpy(reinterpret_cast<char*>(pMsg) + size, &_elem, elemSize);
    ++elemCount;
    size += elemSize;
    return true;
}

// send the message (if there is any), then reset the buffer
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
void RmtMsgBufTy<ElemTy,MsgTy,msgVer>::send (UDPMulticast& _mc)
{
    if (!empty()) {
        _mc.SendMC(pMsg, size);
        init();
    }
}

// Perform add(), then if necessary send()
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
bool RmtMsgBufTy<ElemTy,MsgTy,msgVer>::add_send (const ElemTy& _elem,
                                                 UDPMulticast& _mc)
{
    if (!add(_elem)) {
        send(_mc);
        if (!add(_elem)) {
            throw std::runtime_error("Could not add new alement after sending/initializing a new message!");
        }
        return true;
    }
    return false;
}


// Add a pair of animation type and value to the structure
void RmtDataAcAnimTy::add (DR_VALS idx, float f)
{
    LOG_ASSERT(data.numVals < V_COUNT);
    data.v[data.numVals].idx = idx;
    data.v[data.numVals].v   = REMOTE_DR_DEF[idx].pack(f);
    ++data.numVals;
}


// Returns a pointer to the first/next animation data element in the message
const RemoteAcAnimTy* RemoteMsgAcAnimTy::next (size_t _msgLen,
                                               const RemoteAcAnimTy* pCurr) const
{
    // first element asked? That's simple...
    if (!pCurr)
        return &animData;
    
    // for all others we need to base on the current one and add its variable size
    const char* p = reinterpret_cast<const char*>(pCurr);
    p += pCurr->msgSize();
    
    // Is this now beyond the message size?
    if (size_t(p - reinterpret_cast<const char*>(this)) >= _msgLen)
        return nullptr;
    
    // Should be valid, cast as required
    return reinterpret_cast<const RemoteAcAnimTy*>(p);
}


/// Conditions for continued send operation
inline bool RmtSendContinue ()
{ return !gbStopMCThread && glob.RemoteIsSender() && gpMc && gpMc->isOpen(); }


/// Process the data passed down to us in the queue
void RmtSendProcessQueue ()
{
    LOG_ASSERT(gpMc != nullptr);
    
    // Loop till forced to shut down or queue with data empty
    while (RmtSendContinue() && !gqueueRmtData.empty()) {
        // For taking data out of the queue we need the lock as briefly as possible
        // (flight loop has priority!)
        ptrRmtDataBaseTy ptrData(nullptr);
        {
            std::unique_lock<std::mutex> lk(gmutexRmtData);
            // ptrData takes ownership of the queue data!
            ptrData = std::move(gqueueRmtData.front());
            gqueueRmtData.pop();
        }
        
        // Further handling depends on the type of message
        switch (ptrData->msgTy) {
            // Aircraft detail: Add to pending message, send if full
            case RMT_MSG_AC_DETAILED: {
                RmtDataAcDetailTy* pAcDetail = dynamic_cast<RmtDataAcDetailTy*>(ptrData.get());
                LOG_ASSERT(pAcDetail);
                gMsgAcDetail.add_send(pAcDetail->data, *gpMc);
                break;
            }
                
            // Aircraft position update: Add to pending message, send if full
            case RMT_MSG_AC_POS_UPDATE: {
                RmtDataAcPosUpdateTy* pAcPosUpd = dynamic_cast<RmtDataAcPosUpdateTy*>(ptrData.get());
                LOG_ASSERT(pAcPosUpd);
                gMsgAcPosUpdate.add_send(pAcPosUpd->data, *gpMc);
                break;
            }
                
            // Aircraft animation dataRef values
            case RMT_MSG_AC_ANIM: {
                RmtDataAcAnimTy* pAcAnim = dynamic_cast<RmtDataAcAnimTy*>(ptrData.get());
                LOG_ASSERT(pAcAnim);
                gMsgAcAnim.add_send(pAcAnim->data, *gpMc);
                break;
            }
                
            // Aircraft removal (the XPMP2::Aircraft object will already be gone by this time!)
            case RMT_MSG_AC_REMOVE: {
                RmtDataAcRemoveTy* pAcRemoval = dynamic_cast<RmtDataAcRemoveTy*>(ptrData.get());
                LOG_ASSERT(pAcRemoval);
                gMsgAcRemove.add_send(pAcRemoval->data, *gpMc);
                break;
            }
                
            // Send out pending message
            case RMT_MSG_SEND:
                gMsgAcDetail.send(*gpMc);
                gMsgAcPosUpdate.send(*gpMc);
                gMsgAcAnim.send(*gpMc);
                gMsgAcRemove.send(*gpMc);
                break;
                
            // This type is not expected to happen (because it is send by the receiver or directly)
            case RMT_MSG_SETTINGS:
            case RMT_MSG_INTEREST_BEACON:
                LOG_MSG(logWARN, "Received unexpected send queue entry of type RMT_MSG_SETTINGS or RMT_MSG_INTEREST_BEACON");
                break;
        }
        
        // delete the queue data (we do the explicitely...because we are a nice guy)
        ptrData.reset();
    }
}

/// Send our settings
void RmtSendSettings ()
{
    RemoteMsgSettingsTy s;
    // copy all strings
    strncpy(s.name, glob.pluginName.c_str(), sizeof(s.name));
    strncpy(s.defaultIcao, glob.defaultICAO.c_str(), sizeof(s.defaultIcao));
    strncpy(s.carIcaoType, glob.carIcaoType.c_str(), sizeof(s.carIcaoType));
    // copy further fields
    s.maxLabelDist              = glob.maxLabelDist;
    s.logLvl                    = (std::uint8_t)glob.logLvl;
    s.bLogMdlMatch              = glob.bLogMdlMatch;
    s.bObjReplDataRefs          = glob.bObjReplDataRefs;
    s.bObjReplTextures          = glob.bObjReplTextures;
    s.bLabelCutOffAtVisibility  = glob.bLabelCutOffAtVisibility;
    s.bMapEnabled               = glob.bMapEnabled;
    s.bMapLabels                = glob.bMapLabels;
    s.bHaveTCASControl          = XPMPHasControlOfAIAircraft();
    // send the data out
    assert(gpMc);
    if (gpMc->SendMC(&s, sizeof(s)) != sizeof(s))
        throw NetRuntimeError("Could not send Settings multicast");
}

/// Sending function, ie. we are actively sending data out
void RmtSendLoop ()
{
    // when shall settings be sent next? (1st time: right now!)
    std::chrono::time_point<std::chrono::steady_clock> tpSendSettings;
    // time when we last switched interfaces, we don't do that too often, assume we just did
    std::chrono::time_point<std::chrono::steady_clock> tpIntfSwitch = std::chrono::steady_clock::now();
    // lock to use for the condition variable
    std::unique_lock<std::mutex> lkRmtData(gmutexRmtData, std::defer_lock);
    
    // We set the multicast socket to non-blocking so we can easily check on received data
    gpMc->setBlocking(false);
    
    do
    {
        // Check if any received data is waiting
        // (call is non-blocking as we set the socket so earlier)
        bool bChangeIntf = std::chrono::steady_clock::now() - tpIntfSwitch > std::chrono::seconds(REMOTE_RECV_BEACON_INTVL/2);
        bool bIntfChanged = false;
        std::string from;
        if (glob.remoteAutoDiscovery()) bChangeIntf = false;
        // Loop makes sure we read everything, maybe there's a new interface in there somewhere
        while (gpMc->RecvMC(bChangeIntf, &from, nullptr, &bIntfChanged) > 0)
        {
            if (bIntfChanged) {
                LOG_MSG(logINFO, INFO_MC_SEND_RCVD, from.c_str());
                // don't change the interface again soon
                bChangeIntf = false;
                tpIntfSwitch = std::chrono::steady_clock::now();
                // Send settings immediately so the new client can start processing
                tpSendSettings = std::chrono::time_point<std::chrono::steady_clock>();
            }
        }
        
        // Do we need to send out our settings?
        if (tpSendSettings <= std::chrono::steady_clock::now()) {
            tpSendSettings = std::chrono::steady_clock::now() +
                             std::chrono::seconds(REMOTE_SEND_SETTINGS_INTVL);
            RmtSendSettings();
        }
        
        // Is there any data that needs processing?
        if (RmtSendContinue() && !gqueueRmtData.empty())
            RmtSendProcessQueue();
        
        // Wait for a wake-up by the main thread or for a time we need to send settings next
        if (RmtSendContinue()) {
            lkRmtData.lock();
            gcvRmtData.wait_until(lkRmtData, tpSendSettings);
            lkRmtData.unlock();
        }
    }
    while (RmtSendContinue());
}

/// Thread main function for the sender
void RmtSendMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME("XPMP2_Send");
    LOG_MSG(logDEBUG, "Sender thread started");
    int cntMCErr = 0;                       ///< error counter for network thread
    
    do {
        try {
            LOG_ASSERT(gpMc != nullptr);
            
            // Set global status to: we are "waiting" for some action on the multicast group
            glob.remoteStatus = REMOTE_SEND_WAITING;
            
            // Create a multicast socket and listen if there is any traffic in our multicast group
            gpMc->Join(glob.remoteMCGroup, glob.remotePort, glob.remoteSendIntf,
                       glob.remoteTTL, glob.remoteBufSize);
            int maxSock = (int)gpMc->getSocket() + 1;
#if APL == 1 || LIN == 1
            // the self-pipe to shut down the TCP socket gracefully
            if (pipe(gSelfPipe) < 0)
                throw NetRuntimeError("Couldn't create self-pipe");
            fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
            maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
            
            // *** Main listening loop ***
            
            LOG_MSG(logINFO, INFO_MC_SEND_WAIT, gpMc->GetMCAddr().c_str());
            
            while (RmtSendContinue())
            {
                // wait for some signal on either socket (multicast or self-pipe)
                fd_set sRead;
                FD_ZERO(&sRead);
                FD_SET(gpMc->getSocket(), &sRead);      // check our socket
#if APL == 1 || LIN == 1
                FD_SET(gSelfPipe[0], &sRead);        // check the self-pipe
#endif
                int retval = select(maxSock, &sRead, NULL, NULL, NULL);
                
                // short-cut if we are to shut down (return from 'select' due to closed socket)
                if (!RmtSendContinue())
                    break;
                
                // select call failed???
                if (retval == -1)
                    throw NetRuntimeError("'select' failed");
                
                // select successful - there is multicast data!
                if (retval > 0 && FD_ISSET(gpMc->getSocket(), &sRead))
                {
                    // We aren't actually interested in the data as such,
                    // the fact that there was _any_ traffic already means:
                    // there is someone out there interested in our data.
                    // We just read received multicast to clear out the buffers
                    // and switch the sending interface to the interface we received from.
                    std::string from;
                    gpMc->RecvMC(true, &from);
                    LOG_MSG(logINFO, INFO_MC_SEND_RCVD, from.c_str());
                    
                    // Set global status to: we are about to send data, also exits listening loop
                    glob.remoteStatus = REMOTE_SENDING;
                    break;
                }
            }
            
#if APL == 1 || LIN == 1
            // close the self-pipe sockets
            for (SOCKET &s: gSelfPipe) {
                if (s != INVALID_SOCKET) close(s);
                s = INVALID_SOCKET;
            }
#endif
            // Continue? Then send data!
            if (RmtSendContinue())
                RmtSendLoop();
        }
        catch (std::exception& e) {
            ++cntMCErr;
            LOG_MSG(logERR, ERR_MC_THREAD, e.what());
            
#if APL == 1 || LIN == 1
            // close the self-pipe sockets
            for (SOCKET &s: gSelfPipe) {
                if (s != INVALID_SOCKET) close(s);
                s = INVALID_SOCKET;
            }
#endif
        }
        
        // close the multicast socket
        if (gpMc)
            gpMc->Close();
        
        // Error count too high?
        if (cntMCErr >= MC_MAX_ERR) {
            LOG_MSG(logFATAL, ERR_MC_MAX);
            gbRemoteGaveUp = true;          // the SENDER gives up after too many errors
        }
    }
    while (!gbStopMCThread && cntMCErr < MC_MAX_ERR);
        
    // make sure the end of the thread is recognized and joined
    glob.remoteStatus = REMOTE_OFF;
    gbStopMCThread = true;
    LOG_MSG(logDEBUG, "Sender thread stopping");
}

//
// MARK: RECEIVING Remote Data (Worker Thread)
//

#define INFO_MC_RECV_BEGIN  "Receiver started listening to %s"
#define INFO_MC_RECV_RCVD   "Receiver received data from %.*s on %s, will start message processing"

/// The callback function pointers the remote client plugin provided
RemoteCBFctTy gRmtCBFcts;

/// Sends an Interest Beacon
void RmtSendBeacon()
{
    RemoteMsgBeaconTy msgBeacon;
    if (gpMc->SendMC(&msgBeacon, sizeof(msgBeacon)) != sizeof(msgBeacon))
        throw NetRuntimeError("Could not send Interest Beacon multicast");
    LOG_MSG(logDEBUG, "Sent beacon to MC %s",
            gpMc->GetMCAddr().c_str());
}

/// Conditions for continued receive operation
inline bool RmtRecvContinue ()
{ return !gbStopMCThread && glob.RemoteIsListener() && gpMc && gpMc->isOpen(); }


/// Thread main function for the receiver
void RmtRecvMain()
{
    // This is a thread main function, set thread's name
#if IBM
    SET_THREAD_NAME("XPMP2_Recv");          // found no simple way of turning a normal string like glob.logAcronym into a wchar_t as required by SetThreadDescritpion...so in Windows we stay with a constant text here
#else
    SET_THREAD_NAME((glob.logAcronym + "_Recv").c_str());
#endif
    LOG_MSG(logDEBUG, "Receiver thread started");
    int cntMCErr = 0;                      ///< error counter for network thread

    do {
        // We count how often we send beacons.
        // Once we sent 3 beacons without having received reasonable data
        // we re-setup the entire multicast socket. After changes to interfaces
        // it appears that the multicast is not received from newly available
        // interfaces or even no longer from originally available but not
        // previously actively used interfaces.
        constexpr int MAX_BEACONS = 3;
        int nCountBeacons = 0;
    
        try {
            LOG_ASSERT(gpMc != nullptr);
            
            // Set global status to: we are "waiting" for data, but will send the interest beacon every once in a while
            glob.remoteStatus = REMOTE_RECV_WAITING;
            
            // Create a multicast socket
            gpMc->Join(glob.remoteMCGroup, glob.remotePort, glob.remoteSendIntf,
                       glob.remoteTTL, glob.remoteBufSize);
            int maxSock = (int)gpMc->getSocket() + 1;
#if APL == 1 || LIN == 1
            // the self-pipe to shut down the TCP socket gracefully
            if (pipe(gSelfPipe) < 0)
                throw NetRuntimeError("Couldn't create self-pipe");
            fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
            maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
            
            // Send out a first Interest Beacon
            RmtSendBeacon();
            
            // *** Main listening loop ***
            LOG_MSG(logINFO, INFO_MC_RECV_BEGIN, gpMc->GetMCAddr().c_str());
            while (RmtRecvContinue() &&
                   nCountBeacons < MAX_BEACONS &&
                   cntMCErr < MC_MAX_ERR)
            {
                // wait for some data on either socket (multicast or self-pipe)
                fd_set sRead;
                FD_ZERO(&sRead);
                FD_SET(gpMc->getSocket(), &sRead);      // check our socket
#if APL == 1 || LIN == 1
                FD_SET(gSelfPipe[0], &sRead);        // check the self-pipe
#endif
                // Timeout is 15s, ie. we listen for 15 seconds, then send a beacon, then listen again
                struct timeval timeout = { REMOTE_RECV_BEACON_INTVL, 0 };
                int retval = select(maxSock, &sRead, NULL, NULL, &timeout);
                
                // short-cut if we are to shut down (return from 'select' due to closed socket)
                if (!RmtRecvContinue())
                    break;
                
                // select call failed???
                if (retval == -1)
                    throw NetRuntimeError("'select' failed");
                
                // Timeout? Then send (another) interest beacon...maybe sometimes someone responds
                else if (retval == 0) {
                    RmtSendBeacon();
                    ++nCountBeacons;
                }
                
                // select successful - there is multicast data!
                else if (retval > 0 && FD_ISSET(gpMc->getSocket(), &sRead))
                {
                    // Receive the data (if we are still waiting then we're interested in the sender's address purely for logging purposes)
                    SockAddrTy saFrom;
                    const size_t recvSize = gpMc->RecvMC(false, nullptr, &saFrom);
                    if (recvSize >= sizeof(RemoteMsgBaseTy))
                    {
                        static float lastVerErrMsg = 0.0f;          // last time we issued a msg version warning
                        const InetAddrTy from(saFrom);              // extract the numerical address
                        RemoteMsgBaseTy& hdr = *(RemoteMsgBaseTy*)gpMc->getBuf();
                        hdr.bLocalSender = NetwIsLocalAddr(from);
                        
                        if (hdr.msgTy > RMT_MSG_INTEREST_BEACON)    // if we have proper data
                            nCountBeacons = 0;                      // we don't need to reset the multicast socket
                        
                        switch (hdr.msgTy) {
                                // just ignore any interest beacons
                            case RMT_MSG_INTEREST_BEACON:
                                break;
                                
                                // Settings
                            case RMT_MSG_SETTINGS:
                                if (hdr.msgVer == RMT_VER_SETTINGS && recvSize == sizeof(RemoteMsgSettingsTy))
                                {
                                    const std::string sFrom = SocketNetworking::GetAddrString(saFrom);
                                    const RemoteMsgSettingsTy& s = *(RemoteMsgSettingsTy*)gpMc->getBuf();
                                    // Is this the first set of settings we received? Then we switch status!
                                    if (glob.remoteStatus == REMOTE_RECV_WAITING) {
                                        glob.remoteStatus = REMOTE_RECEIVING;
                                        LOG_MSG(logINFO, INFO_MC_RECV_RCVD,
                                                (int)sizeof(s.name), s.name,
                                                sFrom.c_str());
                                    }
                                    // Let the plugin process this message
                                    if (gRmtCBFcts.pfMsgSettings)
                                        gRmtCBFcts.pfMsgSettings(from.addr, sFrom, s);
                                } else {
                                    LOG_MSG(logWARN, "Cannot process Settings message: %lu bytes, version %u, from %s",
                                            (unsigned long)recvSize, hdr.msgVer, SocketNetworking::GetAddrString(saFrom).c_str());
                                }
                                break;
                                
                                // Full A/C Details
                            case RMT_MSG_AC_DETAILED:
                                // v1, v2, and v3 have same size with more and more fields populated
                                if ((hdr.msgVer == RMT_VER_AC_DETAIL || hdr.msgVer == RMT_VER_AC_DETAIL_2 || hdr.msgVer == RMT_VER_AC_DETAIL_1) &&
                                    recvSize >= sizeof(RemoteMsgAcDetailTy))
                                {
                                    if (gRmtCBFcts.pfMsgACDetails) {
                                        RemoteMsgAcDetailTy& s = *(RemoteMsgAcDetailTy*)gpMc->getBuf();
                                        
                                        // v1: Default was "Draw Labels"
                                        if (hdr.msgVer == RMT_VER_AC_DETAIL_1) {
                                            const size_t n = s.NumElem(recvSize);
                                            for (size_t i = 0; i < n; ++i)
                                                s.arr[i].bDrawLabel = true;
                                        }
                                        
                                        // v2 has no contrail fields, ensure they are defaulted to not show contrails
                                        if (hdr.msgVer <= RMT_VER_AC_DETAIL_2) {
                                            const size_t n = s.NumElem(recvSize);
                                            for (size_t i = 0; i < n; ++i) {
                                                s.arr[i].contrailNum = 0;
                                                s.arr[i].contrailDist_m = 0;
                                                s.arr[i].contrailLifeTime = 0;
                                            }
                                        }
                                        
                                        gRmtCBFcts.pfMsgACDetails(from.addr, recvSize, s);
                                    }
                                }
                                // Convert a v0 msg, then process
                                else if (hdr.msgVer == RMT_VER_AC_DETAIL_0 && recvSize >= sizeof(RemoteMsgAcDetailTy_v0))
                                {
                                    if (gRmtCBFcts.pfMsgACDetails) {
                                        const RemoteMsgAcDetailTy_v0& s = *(RemoteMsgAcDetailTy_v0*)gpMc->getBuf();
                                        size_t msgLen = 0;
                                        RemoteMsgAcDetailTy* pMsg = s.convert(recvSize, msgLen);
                                        gRmtCBFcts.pfMsgACDetails(from.addr, msgLen, *pMsg);
                                        std::free(pMsg);
                                    }
                                } else {
                                    if (CheckEverySoOften(lastVerErrMsg, 600.0f))
                                        LOG_MSG(logWARN, "Cannot process A/C Details message: %lu bytes, version %u, from %s\nCheck for an updated version on X-Plane.org",
                                                (unsigned long)recvSize, hdr.msgVer, SocketNetworking::GetAddrString(saFrom).c_str());
                                }
                                break;
                                
                                // A/C Position Update
                            case RMT_MSG_AC_POS_UPDATE:
                                if (hdr.msgVer == RMT_VER_AC_POS_UPDATE && recvSize >= sizeof(RemoteMsgAcPosUpdateTy))
                                {
                                    if (gRmtCBFcts.pfMsgACPosUpdate) {
                                        const RemoteMsgAcPosUpdateTy& s = *(RemoteMsgAcPosUpdateTy*)gpMc->getBuf();
                                        gRmtCBFcts.pfMsgACPosUpdate(from.addr, recvSize, s);
                                    }
                                } else {
                                    if (CheckEverySoOften(lastVerErrMsg, 600.0f))
                                        LOG_MSG(logWARN, "Cannot process A/C Pos Update message: %lu bytes, version %u, from %s\nCheck for an updated version on X-Plane.org",
                                                (unsigned long)recvSize, hdr.msgVer, SocketNetworking::GetAddrString(saFrom).c_str());
                                }
                                break;
                                
                                // A/C Animdation dataRefs
                            case RMT_MSG_AC_ANIM:
                                if (hdr.msgVer == RMT_VER_AC_ANIM && recvSize >= sizeof(RemoteMsgAcAnimTy))
                                {
                                    if (gRmtCBFcts.pfMsgACAnim) {
                                        const RemoteMsgAcAnimTy& s = *(RemoteMsgAcAnimTy*)gpMc->getBuf();
                                        gRmtCBFcts.pfMsgACAnim(from.addr, recvSize, s);
                                    }
                                } else {
                                    if (CheckEverySoOften(lastVerErrMsg, 600.0f))
                                        LOG_MSG(logWARN, "Cannot process A/C Animations message: %lu bytes, version %u, from %s\nCheck for an updated version on X-Plane.org",
                                                (unsigned long)recvSize, hdr.msgVer, SocketNetworking::GetAddrString(saFrom).c_str());
                                }
                                break;
                                
                                // A/C Removal
                            case RMT_MSG_AC_REMOVE:
                                if (hdr.msgVer == RMT_VER_AC_REMOVE && recvSize >= sizeof(RemoteMsgAcRemoveTy))
                                {
                                    if (gRmtCBFcts.pfMsgACRemove) {
                                        const RemoteMsgAcRemoveTy& s = *(RemoteMsgAcRemoveTy*)gpMc->getBuf();
                                        gRmtCBFcts.pfMsgACRemove(from.addr, recvSize, s);
                                    }
                                } else {
                                    if (CheckEverySoOften(lastVerErrMsg, 600.0f))
                                        LOG_MSG(logWARN, "Cannot process A/C Remove message: %lu bytes, version %u, from %s\nCheck for an updated version on X-Plane.org",
                                                (unsigned long)recvSize, hdr.msgVer, SocketNetworking::GetAddrString(saFrom).c_str());
                                }
                                break;
                                
                                // This type is not expected to happen (because it is a marker for the sender queue only)
                            case RMT_MSG_SEND:
                                LOG_MSG(logWARN, "Received unexpected message type RMT_MSG_SEND");
                                break;
                        }
                        
                    } else {
                        LOG_MSG(logWARN, "Received too small message with just %lu bytes", (unsigned long)recvSize);
                    }
                }
            }
        }
        catch (std::exception& e) {
            ++cntMCErr;
            LOG_MSG(logERR, ERR_MC_THREAD, e.what());
        }
        
        // close the multicast socket
        gpMc->Close();
        
#if APL == 1 || LIN == 1
        // close the self-pipe sockets
        for (SOCKET &s: gSelfPipe) {
            if (s != INVALID_SOCKET) close(s);
            s = INVALID_SOCKET;
        }
#endif
        
        // Error count too high?
        if (cntMCErr >= MC_MAX_ERR) {
            LOG_MSG(logFATAL, ERR_MC_MAX);
        }
    }
    while (!gbStopMCThread &&
           cntMCErr < MC_MAX_ERR);
    
    // make sure the end of the thread is recognized and joined
    glob.remoteStatus = REMOTE_OFF;
    gbStopMCThread = true;
    LOG_MSG(logDEBUG, "Receiver thread stopping");
}

//
// MARK: Internal functions (XP Main Thread)
//

/// Start the background thread to listen to multicast, to see if anybody is interested in our data
void RmtStartMCThread(bool bSender)
{
    // Can only do that if currently off
    if (glob.remoteStatus != REMOTE_OFF)
        return;
    
    // Is or was there a thread running?
    if (gThrMC.joinable()) {
        if (!gbStopMCThread)            // it still is running
            return;                     // keep it running
        gThrMC.join();                  // wait for the current thread to exit (which it should because bStopMCListen is true)
        gThrMC = std::thread();
    }
    
    // seen too many errors already?
    if (gbRemoteGaveUp)
        return;
    
    // Start the thread
    gbStopMCThread = false;
    gThrMC = std::thread(bSender ? RmtSendMain : RmtRecvMain);
}

/// Stop all threads and communication with the network
void RmtStopAll()
{
    // Network thread
    if (gThrMC.joinable()) {
        gbStopMCThread = true;
#if APL == 1 || LIN == 1
        // Mac/Lin: Try writing something to the self-pipe to stop gracefully
        if (gSelfPipe[1] == INVALID_SOCKET ||
            write(gSelfPipe[1], "STOP", 4) < 0)
            // if the self-pipe didn't work:
#endif
            if (gpMc)
                gpMc->Close();
        // Make sure the lock is no longer held
        if (glockRmtData)
            glockRmtData.unlock();
        // Trigger the thread to wake up for proper exit
        gcvRmtData.notify_all();
        // wait for the network thread to finish
        gThrMC.join();
        gThrMC = std::thread();
    }
}

//
// MARK: Global public functions (XP Main Thread)
//

// Initialize the module
void RemoteInit ()
{
    // Create the global multicast object
    gbRemoteGaveUp = false;
    if (!gpMc)
        gpMc = new UDPMulticast();
}

// Grace cleanup, stops all threads
void RemoteCleanup ()
{
    RmtStopAll();               // stop everything (multicast listening/sending, thread)
    if (gpMc) {                 // remove the multicast object
        delete gpMc;
        gpMc = nullptr;
    }
    
    // Remove all message caches
    gMsgAcDetail.free();
}

// Returns the current Remote status
RemoteStatusTy RemoteGetStatus()
{
    return glob.remoteStatus;
}

//
// MARK: Global Enqueue/Send functions (XP Main Thread)
//

// Compares current vs. expected status and takes appropriate action
void RemoteSenderUpdateStatus ()
{
    // Don't change anything if in fact we are in listening mode
    if (glob.RemoteIsListener())
        return;
        
    // only anything else than off if there are planes
    RemoteStatusTy expected = REMOTE_OFF;
    if (XPMPCountPlanes() > 0) {
        switch (glob.remoteCfg) {
            // if conditionally on: Test if we are in a networked setup
            case SWITCH_CFG_AUTO:
                if (glob.bXPNetworkedSetup)
                    expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_SEND_WAITING;
                break;
            // forcibly on?
            case SWITCH_CFG_ON:
                expected = glob.remoteStatus == REMOTE_SENDING ? REMOTE_SENDING : REMOTE_SEND_WAITING;
                break;
            // forcibly off?
            case SWITCH_CFG_OFF:
                break;
        }
    }
    
    // So no change then?
    if (expected == glob.remoteStatus)
        return;
    
    // Switch off?
    if (expected == REMOTE_OFF)
        RmtStopAll();
    else
        RmtStartMCThread(true);
}

// Informs us that updating a/c will start now, do some prep work
void RemoteAcEnqueueStarts (float now)
{
    // store the timestampe for later use
    gNow = now;
    
    // Actively sending?
    if (glob.remoteStatus == REMOTE_SENDING)
    {
        // From here on until RemoteAcEnqueueDone() we keep the lock so that
        // the flight loop can run uninterrupted
        if (!glockRmtData)
            glockRmtData.lock();

        // The current group due for full a/c details update
        // (basically current time in seconds modulo interval plus 1,
        //  so the result is in 1..REMOTE_SEND_AC_DETAILS_INTVL)
        unsigned nxtGrp = unsigned(std::fmod(now, REMOTE_SEND_AC_DETAILS_INTVL)) + 1;
        if (gFullUpdLastDone != nxtGrp)          // it's a new group to process!
            gFullUpdDue = nxtGrp;
        else
            gFullUpdDue = 0;                    // not the same again!
    }
    // or are we a receiver?
    else if (glob.remoteStatus == REMOTE_RECEIVING)
    {
        if (gRmtCBFcts.pfBeforeFirstAc)         // Inform client that flight loop processing starts
            gRmtCBFcts.pfBeforeFirstAc();
    }
}

// Regularly called from the flight loop callback
void RemoteAcEnqueue (const Aircraft& ac)
{
    // Can only do anything reasonable if we are to send data
    // and also aren't too fast based on max trasnfer frequency
    if (glob.remoteStatus != REMOTE_SENDING ||
        gNow < gNxtTxfTime)
        return;

    bool bSendFullDetails = false;
    
    // Position: Convert known local coordinates to world coordinates
    double lat, lon, alt_ft;
    XPLMLocalToWorld(ac.drawInfo.x,
                     ac.drawInfo.y - ac.GetVertOfs(),
                     ac.drawInfo.z,
                     &lat, &lon, &alt_ft);
    alt_ft /= M_per_FT;
    
    // Do we know this a/c already?
    mapRmtAcCacheTy::iterator itCache = gmapRmtAcCache.find(ac.GetModeS_ID());
    if (itCache == gmapRmtAcCache.end())  {
        // no, it's a new a/c, so add a record into our cache
        bSendFullDetails = true;
        auto p = gmapRmtAcCache.emplace(ac.GetModeS_ID(),
                                        RmtAcCacheTy(ac,lat,lon,float(alt_ft)));
        itCache = p.first;
    }
    RmtAcCacheTy& acCache = itCache->second;

    if (!bSendFullDetails &&
        // Is this an a/c of the group that shall send full details?
        (acCache.fullUpdGrp == gFullUpdDue       ||
         // Or did visibility/validity change?
         acCache.bRender    != ac.IsRendered()   ||
         acCache.bVisible   != ac.IsVisible()    ||
         acCache.bValid     != ac.IsValid()      ||
         // Did the CSL model change?
         acCache.pCSLMdl    != ac.GetModel()    ||
         // Or are the differences that we need to send too large for a pos update msg?
         std::abs(lat -    acCache.lat)    > REMOTE_MAX_DIFF_DEGREE ||
         std::abs(lon -    acCache.lon)    > REMOTE_MAX_DIFF_DEGREE ||
         std::abs(alt_ft - acCache.alt_ft) > REMOTE_MAX_DIFF_ALT_FT ||
         gNow -            acCache.ts      > REMOTE_MAX_DIFF_TIME))
        bSendFullDetails = true;
        
    // We should own the mutex already...just to be sure
    if (!glockRmtData) glockRmtData.lock();
    
    if (bSendFullDetails) {
        // add to the full data, protected by a lock
        gqueueRmtData.emplace(new RmtDataAcDetailTy(RemoteAcDetailTy(ac,lat,lon,float(alt_ft),
                                                                     (std::uint16_t)std::lround((gNow  - acCache.ts)     / REMOTE_TIME_RES))));
        // Which full update group did we actually really process?
        if (gFullUpdDue > 0)
            gFullUpdLastDone = gFullUpdDue;
    }
    else {
        // add the position update to the queue, containing a delta position
        gqueueRmtData.emplace(new RmtDataAcPosUpdateTy(RemoteAcPosUpdateTy(
            ac.GetModeS_ID(),                                                   // modeS_id
            (std::int16_t) std::lround((lat   - acCache.lat)    / REMOTE_DEGREE_RES),   // dLat
            (std::int16_t) std::lround((lon   - acCache.lon)    / REMOTE_DEGREE_RES),   // dLon
            (std::int16_t) std::lround((alt_ft- acCache.alt_ft) / REMOTE_ALT_FT_RES),   // dAlt_ft
            (std::uint16_t)std::lround((gNow  - acCache.ts)     / REMOTE_TIME_RES),     // dTime
            ac.GetPitch(), ac.GetHeading(), ac.GetRoll()
        )));
        
        // Have animation dataRefs changed? We do a quick check by memcmp first
        LOG_ASSERT(acCache.v.size() >= V_COUNT && ac.v.size() >= V_COUNT);
        if (std::memcmp(acCache.v.data(), ac.v.data(),
                        sizeof(acCache.v[0]) * V_COUNT) != 0) {
            // Let's create the message and put it into the queue
            ptrRmtDataAcAnimTy pAnimData = std::make_unique<RmtDataAcAnimTy>(ac.GetModeS_ID());
            // Loop over all _pre-defined_ animation dataRefs and add changed values to the msg data
            for (std::uint8_t idx = 0; idx < V_COUNT; ++idx)
                if (ac.v[idx] != acCache.v[idx] &&
                    // skip over those dataRef which change all the time and are recalculate in the client on its own
                    idx != XPMP2::V_GEAR_TIRE_ROTATION_ANGLE_DEG &&
                    idx != XPMP2::V_ENGINES_ENGINE_ROTATION_ANGLE_DEG &&
                    idx != XPMP2::V_ENGINES_PROP_ROTATION_ANGLE_DEG &&
                    idx != XPMP2::V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1 &&
                    idx != XPMP2::V_ENGINES_ENGINE_ROTATION_ANGLE_DEG2 &&
                    idx != XPMP2::V_ENGINES_ENGINE_ROTATION_ANGLE_DEG3 &&
                    idx != XPMP2::V_ENGINES_ENGINE_ROTATION_ANGLE_DEG4)
                    pAnimData->add(DR_VALS(idx), ac.v[idx]);
            // Add the data to the queue
            if (pAnimData->data.numVals > 0)
                gqueueRmtData.emplace(std::move(pAnimData));
        }
    }
    acCache.UpdateFrom(ac, lat, lon, alt_ft);
}

// Informs us that all a/c have been processed: All pending messages to be sent now
void RemoteAcEnqueueDone ()
{
    // Actively sending?
    if (glob.remoteStatus == REMOTE_SENDING)
    {
        // Only if we have the lock there's a chance we added anything to be sent now
        // (also a safe-guard against double-execution)
        if (glockRmtData) {
            // Put a signal into the queue that tells the network thread to send out any pending messages
            gqueueRmtData.emplace(new RmtDataBaseTy(RMT_MSG_SEND));
            
            // Release the lock and tell the network thread to wake up for work
            glockRmtData.unlock();
            gcvRmtData.notify_one();
        }
        
        // When to send next earliest?
        if (gNow >= gNxtTxfTime)
            gNxtTxfTime = gNow + 1.0f/float(glob.remoteTxfFrequ);
    }
    // or are we a receiver?
    else if (glob.remoteStatus == REMOTE_RECEIVING)
    {
        if (gRmtCBFcts.pfAfterLastAc)         // Inform client that flight loop processing ends
            gRmtCBFcts.pfAfterLastAc();
    }
    
    // In any case make sure the lock is no longer held
    if (glockRmtData)
        glockRmtData.unlock();
}

// Inform us about an aircraft deletion
void RemoteAcRemove (const Aircraft& ac)
{
    // Can only do anything reasonable if we are to send data
    if (glob.remoteStatus != REMOTE_SENDING) {
        if (!gmapRmtAcCache.empty())           // that's faster than searching for the individual plane...
            gmapRmtAcCache.clear();            // when we are not/no longer sending then the cache is outdated anyway
        return;
    }
    
    // We need the mutex to add to the queue
    bool bDidLock = false;                      // this isn't the most safe solution...but a recursive_mutex doesn't work with the condition variable used to notify the network thread
    if (!glockRmtData) {
        glockRmtData.lock();
        bDidLock = true;
    }
    
    // Add the plane id to the queue, marked for removal
    gqueueRmtData.emplace(new RmtDataAcRemoveTy(RemoteAcRemoveTy(ac.GetModeS_ID())));
    
    // Remove the plane from the cache
    gmapRmtAcCache.erase(ac.GetModeS_ID());
    
    // Undo the lock if we locked earlier
    if (bDidLock)
        glockRmtData.unlock();
}

// Informs us that there are no more aircraft, clear our caches!
void RemoteAcClearAll ()
{
    // Can only do anything reasonable if we are to send data
    if (glob.remoteStatus != REMOTE_SENDING) {
        if (!gmapRmtAcCache.empty())           // that's faster than searching for the individual plane...
            gmapRmtAcCache.clear();            // when we are not/no longer sending then the cache is outdated anyway
        return;
    }
    
    // Put a signal into the queue that tells the network thread to send out any pending messages
    // (at least the last A/C removal message will still wait there)
    gqueueRmtData.emplace(new RmtDataBaseTy(RMT_MSG_SEND));
    
    // Tell the network thread to wake up for work
    gcvRmtData.notify_one();

    // Clear the cache
    gmapRmtAcCache.clear();
}

//
// MARK: Global Receive functions (XP Main Thread)
//

// Starts the listener, will call provided callback functions with received messages
void RemoteRecvStart (const RemoteCBFctTy& _rmtCBFcts)
{
    gRmtCBFcts = _rmtCBFcts;
    RmtStartMCThread(false);
}

// Stops the receiver
void RemoteRecvStop ()
{
    RmtStopAll();
    gRmtCBFcts = RemoteCBFctTy();
}

};

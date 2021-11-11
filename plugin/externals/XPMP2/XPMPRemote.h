/// @file       XPMPRemote.h
/// @brief      Semi-public remote network functionality for master/client operations
/// @details    Technically, this is public functionality from a library
///             point of view. But it is intended for the "XPMP Remote Client" only,
///             not for a standard plugin.\n
///             Network messages are packed for space efficiency, but also to avoid
///             layout differences between compilers/platforms.
///             However, manual layout tries to do reasonable alignment of numeric values
///             and 8-byte-alignment of each structure, so that arrays of structures
///             also align well.
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

#ifndef _XPMPRemote_h_
#define _XPMPRemote_h_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <array>
#include <algorithm>

namespace XPMP2 {

/// The signature of the XPMP2 Remote Client
constexpr const char* REMOTE_SIGNATURE      =  "TwinFan.plugin.XPMP2.Remote";

//
// MARK: Global Helpers
//

/// @brief Produces a reproducible(!) hash value for strings
/// @details Result is the same if the same string is provided, across platform
///          and across executions, unlike what std::hash requires.\n
///          It is implemented as a 16-bit version of the PJW hash:
/// @see https://en.wikipedia.org/wiki/PJW_hash_function
std::uint16_t PJWHash16 (const char *s);

/// @brief Find a model by package name hash and short id
/// @details This approach is used by the remote client to safe network bandwith.
///          If an exact match with `pkgHash` _and_ `shortID` is not found,
///          then a model matching the short id alone is returned if available.
CSLModel* CSLModelByPkgShortId (std::uint16_t _pkgHash,
                                const std::string& _shortId);

/// @brief Clamps `v` between `lo` and `hi`: `lo` if `v` < `lo`, `hi` if `hi` < `v`, otherwise `v`
/// @see C++17, https://en.cppreference.com/w/cpp/algorithm/clamp
/// @note Reimplemented here because Docker clang environment doesn't include it
template<class T>
constexpr const T& clamp( const T& v, const T& lo, const T& hi )
{
    assert( !(hi < lo) );
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

//
// MARK: Network Data Definitions
//

/// Message type
enum RemoteMsgTy : std::uint8_t {
    RMT_MSG_INTEREST_BEACON = 0,    ///< beacon sent by a remote client to signal interest in data
    RMT_MSG_SEND,                   ///< internal indicator telling to send out all pending messages
    RMT_MSG_SETTINGS,               ///< a sender's id and its settings
    RMT_MSG_AC_DETAILED,            ///< aircraft full details, needed to create new a/c objects and to re-synch all remote data
    RMT_MSG_AC_POS_UPDATE,          ///< aircraft differences only
    RMT_MSG_AC_ANIM,                ///< aircraft animation values (dataRef values) only
    RMT_MSG_AC_REMOVE,              ///< aircraft is removed
};

/// Definition for how to map dataRef values to (u)int8, ie. to an integer range of 8 bits
struct RemoteDataRefPackTy {
    float       minV  = 0.0f;       ///< minimum transferred value
    float       range = 0.0f;       ///< range of transferred value = maxV - minV

    /// Constructor sets minimum value and range
    RemoteDataRefPackTy (float _min, float _max) : minV(_min), range(_max - _min) { assert(range != 0.0f); }
    
    /// pack afloat value to integer
    std::uint8_t pack (float f) const   { return std::uint8_t(clamp<float>(f-minV,0.0f,range) * UINT8_MAX / range); }
    /// unpack an integer value to float
    float unpack (std::uint8_t i) const { return minV + range*float(i)/255.0f; }
};

/// An array holding all dataRef packing definitions
extern const std::array<RemoteDataRefPackTy,V_COUNT> REMOTE_DR_DEF;

//
// MARK: Message Header (Base)
//

// To ensure best network capacity usage as well as identical alignment across platform we enforce tightly packed structures.
// The approach is very different between Visual C++ and clang / gcc, though:
#ifdef _MSC_VER                                 // Visual C++
#pragma pack(push,1)                            // set packing once (ie. not per struct)
#define PACKED
#elif defined(__clang__) || defined(__GNUC__)   // Clang (Mac XCode etc) or GNU (Linux)
#define PACKED __attribute__ ((__packed__))
#else
#error Unhandled Compiler!
#endif

/// Message header, identical for all message types
struct RemoteMsgBaseTy {
    RemoteMsgTy  msgTy  : 4;        ///< message type
    std::uint8_t msgVer : 4;        ///< message version
    bool         bLocalSender : 1;  ///< is the sender "local", ie. on same machine?
    std::uint8_t filler1      : 7;  ///< yet unsed
    std::uint16_t pluginId = 0;     ///< lower 16 bit of the sending plugin's id
    std::uint32_t filler2 = 0;      ///< yet unused, fills up to size 8
    /// Constructor just sets the values
    RemoteMsgBaseTy (RemoteMsgTy _ty, std::uint8_t _ver);
} PACKED;

//
// MARK: Beacon of Interest
//

/// Interest Beacon message version number
constexpr std::uint8_t RMT_VER_BEACON = 0;
/// "Beacon of Interest", ie. some message on the multicast just to wake up sender
struct RemoteMsgBeaconTy : public RemoteMsgBaseTy {
    // don't need additional data fields
    /// Constructor sets appropriate message type
    RemoteMsgBeaconTy();
} PACKED;

//
// MARK: Settings
//

/// How often to send settings? [s]
constexpr int   REMOTE_SEND_SETTINGS_INTVL  = 20;

/// Setttings message version number
constexpr std::uint8_t RMT_VER_SETTINGS = 0;
/// Settings message, identifying a sending plugin, regularly providing its settings
struct RemoteMsgSettingsTy : public RemoteMsgBaseTy {
    char            name[16];           ///< plugin's name, not necessarily zero-terminated if using full 12 chars
    float           maxLabelDist;       ///< Maximum distance for drawing labels? [m]
    char            defaultIcao[4];     ///< Default ICAO aircraft type designator if no match can be found
    char            carIcaoType[4];     ///< Ground vehicle type identifier
    std::uint8_t logLvl             :3; ///< logging level
    bool bLogMdlMatch               :1; ///< Debug model matching?
    bool bObjReplDataRefs           :1; ///< Replace dataRefs in `.obj` files on load?
    bool bObjReplTextures           :1; ///< Replace textures in `.obj` files on load if needed?
    bool bLabelCutOffAtVisibility   :1; ///< Cut off labels at XP's reported visibility mit?
    bool bMapEnabled                :1; ///< Do we feed X-Plane's maps with our aircraft positions?
    bool bMapLabels                 :1; ///< Do we show labels with the aircraft icons?
    bool bHaveTCASControl           :1; ///< Do we have AI/TCAS control?
    std::uint16_t filler;               ///< yet unused, fills size up for a multiple of 8
    
    /// Constructor sets most values to zero
    RemoteMsgSettingsTy ();
    
} PACKED;

//
// MARK: A/C Details
//

/// A/C detail message version number
constexpr std::uint8_t RMT_VER_AC_DETAIL = 1;
/// A/C details, packed into an array message
struct RemoteAcDetailTy {
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
    
    // selectively taken from XPMPInfoTexts_t and packed:
    char            tailNum[10];        ///< registration, tail number
    char            manufacturer[40];   ///< a/c manufacturer, human readable
    char            model[40];          ///< a/c model, human readable
    char            airline[40];        ///< airline, human readable
    char            flightNum [10];     ///< flight number
    char            aptFrom [5];        ///< Origin airport (ICAO)
    char            aptTo [5];          ///< Destination airport (ICAO)
    
    std::uint8_t    filler[5];          ///< yet unused
    
    ///< Array of _packed_ dataRef values for CSL model animation
    std::uint8_t    v[XPMP2::V_COUNT];    // 42

    /// Default Constructor sets all to zero
    RemoteAcDetailTy ();
    /// A/c copy constructor fills from passed-in XPMP2::Aircraft object
    RemoteAcDetailTy (const Aircraft& _ac, double _lat, double _lon, float _alt_ft, std::uint16_t _dTime);
    /// Copies values from passed-in XPMP2::Aircraft object
    void CopyFrom (const Aircraft& _ac, double _lat, double _lon, float _alt_ft, std::uint16_t _dTime);
    
    void SetLabelCol (const float _col[4]);     ///< set the label color from a float array (4th number, alpha, is always considered 1.0)
    void GetLabelCol (float _col[4]) const;     ///< writes color out into a float array
    
    void SetPitch (float _p) { pitch = std::int16_t(_p*100.0f); }  ///< sets pitch from float
    float GetPitch () const { return float(pitch) / 100.0f; }                   ///< returns float pitch

    /// @brief Sets heading from float
    /// @note Only works well for `0 <= _h < 360`
    void SetHeading (float _h);
    float GetHeading () const { return float(heading) / 100.0f; }               ///< returns float heading

    void SetRoll (float _r) { roll = std::int16_t(std::lround(_r*100.0f)); }  ///< sets pitch from float
    float GetRoll () const { return float(roll) / 100.0f; }                   ///< returns float pitch

    static constexpr size_t msgSize () { return sizeof(RemoteAcDetailTy); }   ///< message size
} PACKED;

/// A/C detail message, has an inherited header plus an array of XPMP2::RemoteAcDetailTy elements
struct RemoteMsgAcDetailTy : public RemoteMsgBaseTy {
    RemoteAcDetailTy arr[1];                ///< basis for the array of actual details
    
    /// Constructor sets expected message type and version
    RemoteMsgAcDetailTy () : RemoteMsgBaseTy(RMT_MSG_AC_DETAILED, RMT_VER_AC_DETAIL) {}
    /// Convert msg len to number of arr elements
    static constexpr size_t NumElem (size_t _msgLen) { return (_msgLen - sizeof(RemoteMsgBaseTy)) / sizeof(RemoteAcDetailTy); }
} PACKED;

//
// MARK: A/C Position Update
//

/// A/C Position update message version number
constexpr std::uint8_t RMT_VER_AC_POS_UPDATE = 0;

/// What is the maximum difference a RemoteAcPosUpdateTy can hold?
constexpr double REMOTE_DEGREE_RES          = 0.00000001;                   ///< resolution of degree updates
constexpr double REMOTE_MAX_DIFF_DEGREE     = REMOTE_DEGREE_RES * INT16_MAX;///< maximum degree difference that can be represented in a pos update msg
constexpr double REMOTE_ALT_FT_RES          = 0.01;                         ///< resolution of altitude[ft] updates
constexpr double REMOTE_MAX_DIFF_ALT_FT     = REMOTE_ALT_FT_RES * INT16_MAX;///< maximum altitude[ft] difference that can be represented in a pos update msg
constexpr float  REMOTE_TIME_RES            = 0.0001f;                      ///< resolution of time difference
constexpr float  REMOTE_MAX_DIFF_TIME       = REMOTE_TIME_RES * UINT16_MAX; ///< maximum time difference thatn can be represented in a pos update msg

/// @brief A/C Position updates based on global coordinates
/// @details for space efficiency only deltas to last msg are given in
///          0.0000001 degree lat/lon (roughly 1 centimeter resolution) and
///          0.01 ft altitude
struct RemoteAcPosUpdateTy {
    std::uint32_t   modeS_id;           ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
    std::int16_t    dLat;               ///< [0.0000001 degrees] latitude position difference
    std::int16_t    dLon;               ///< [0.0000001 degrees] longitude position difference
    std::int16_t    dAlt_ft;            ///< [0.01 ft] altitude difference
    std::uint16_t   dTime;              ///< [0.0001s] time difference to previous position in 1/10000s
    std::int16_t    pitch;              ///< [0.01 degree] pitch/100
    std::uint16_t   heading;            ///< [0.01 degree] heading/100
    std::int16_t    roll;               ///< [0.01 degree] roll/100
    std::uint16_t   filler1;            ///< not yet used (for 4-byte alignment)
    
    /// Default Constructor sets all 0
    RemoteAcPosUpdateTy () { std::memset(this,0,sizeof(*this)); }
    /// Constructor sets all values
    RemoteAcPosUpdateTy (XPMPPlaneID _modeS_id,
                         std::int16_t _dLat, std::int16_t _dLon,
                         std::int16_t _dAlt_ft, std::uint16_t _dTime,
                         float _pitch, float _heading, float _roll);
    
    void SetPitch (float _p) { pitch = std::int16_t(_p*100.0f); }  ///< sets pitch from float
    float GetPitch () const { return float(pitch) / 100.0f; }               ///< returns float pitch

    /// @brief Sets heading from float
    /// @note Only works well for `0 <= _h < 360`
    void SetHeading (float _h);
    float GetHeading () const { return float(heading) / 100.0f; }           ///< returns float heading

    void SetRoll (float _r) { roll = std::int16_t(std::lround(_r*100.0f)); }///< sets pitch from float
    float GetRoll () const { return float(roll) / 100.0f; }                 ///< returns float pitch

    static constexpr size_t msgSize () { return sizeof(RemoteAcPosUpdateTy); }    ///< message size
} PACKED;

/// A/C detail message, has an inherited header plus an array of XPMP2::RemoteAcDetailTy elements
struct RemoteMsgAcPosUpdateTy : public RemoteMsgBaseTy {
    RemoteAcPosUpdateTy arr[1];         ///< basis for the array of actual position updates
    
    /// Constructor sets expected message type and version
    RemoteMsgAcPosUpdateTy () : RemoteMsgBaseTy(RMT_MSG_AC_POS_UPDATE, RMT_VER_AC_POS_UPDATE) {}
    /// Convert msg len to number of arr elements
    static constexpr size_t NumElem (size_t _msgLen) { return (_msgLen - sizeof(RemoteMsgBaseTy)) / sizeof(RemoteAcPosUpdateTy); }
} PACKED;

//
// MARK: A/C animation dataRefs
//

/// A/C Position update message version number
constexpr std::uint8_t RMT_VER_AC_ANIM = 0;

/// @brief A/C animation dataRef changes
/// @details This structure has variable length depending on the number of
///          actual dataRef values to carry. And several of these variable
///          length structures are added into one variable length network message.
/// @note Structure must stay aligned with XPMP2::RmtDataAcAnimTy
struct RemoteAcAnimTy {
    std::uint32_t   modeS_id = 0;       ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
    std::uint8_t    numVals = 0;        ///< number of dataRef values in the following array
    std::uint8_t    filler = 0;         ///< not yet used
    
    /// dataRef animation types and value
    struct DataRefValTy {
        DR_VALS         idx;            ///< index into XPMP2::Aircraft::v
        std::uint8_t    v;              ///< dataRef animation value
    } v[1];                             ///< array of dataRef animation types and value
    
    /// Constructor
    RemoteAcAnimTy (XPMPPlaneID _id) : modeS_id(_id) { v[0].idx = DR_VALS(0); v[0].v = 0; }
    
    /// message size assuming `num` array elements
    static constexpr size_t msgSize (std::uint8_t num)
    { return sizeof(RemoteAcAnimTy) + (num-1) * sizeof(DataRefValTy); }
    /// current message size
    size_t msgSize() const { return msgSize(numVals); }
} PACKED;

/// A/C animation dataRef message, has an inherited header plus an array of _variable sized_ XPMP2::RemoteAcAnimTy elements
struct RemoteMsgAcAnimTy : public RemoteMsgBaseTy {
    RemoteAcAnimTy animData;            ///< message data starts here but extend beyond this point!
    
    /// Constructor sets expected message type and version
    RemoteMsgAcAnimTy () : RemoteMsgBaseTy(RMT_MSG_AC_ANIM, RMT_VER_AC_ANIM), animData(0) {}
    
    /// Returns a pointer to the first/next animation data element in the message
    const RemoteAcAnimTy* next (size_t _msgLen, const RemoteAcAnimTy* pCurr = nullptr) const;
} PACKED;


//
// MARK: A/C Removal
//


/// A/C removal message version number
constexpr std::uint8_t RMT_VER_AC_REMOVE = 0;

/// A/C Removal only includes the plane id, structure required for msgSize() function
struct RemoteAcRemoveTy {
    std::uint32_t   modeS_id;           ///< plane's unique id at the sender side (might differ remotely in case of duplicates)
    
    /// Constructor sets plane id
    RemoteAcRemoveTy (XPMPPlaneID _id = 0) : modeS_id(_id) {}

    static constexpr size_t msgSize () { return sizeof(RemoteAcRemoveTy); }    ///< message size
} PACKED;

/// A/C removal message, an array of plane ids
struct RemoteMsgAcRemoveTy : public RemoteMsgBaseTy {
    RemoteAcRemoveTy   arr[1];          ///< plane's unique id at the sender side (might differ remotely in case of duplicates)

    /// Constructor sets expected message type and version
    RemoteMsgAcRemoveTy () : RemoteMsgBaseTy(RMT_MSG_AC_REMOVE, RMT_VER_AC_REMOVE) {}
    /// Convert msg len to number of arr elements
    static constexpr size_t NumElem (size_t _msgLen) { return (_msgLen - sizeof(RemoteMsgBaseTy)) / sizeof(arr[0]); }
} PACKED;

#ifdef _MSC_VER                                 // Visual C++
#pragma pack(pop)                               // reseting packing
#endif

// A few static validations just to make sure that no compiler fiddles with my network message layout.
static_assert(sizeof(RemoteMsgBaseTy)       ==   8,     "RemoteMsgBaseTy doesn't have expected size");
static_assert(sizeof(RemoteMsgSettingsTy)   ==  40,     "RemoteMsgSettingsTy doesn't have expected size");
static_assert(sizeof(RemoteAcDetailTy)      == 246+42,  "RemoteAcDetailTy doesn't have expected size");
static_assert(sizeof(RemoteMsgAcDetailTy)   == 254+42,  "RemoteMsgAcDetailTy doesn't have expected size");
static_assert(sizeof(RemoteAcPosUpdateTy)   ==  20,     "RemoteAcPosUpdateTy doesn't have expected size");
static_assert(sizeof(RemoteMsgAcPosUpdateTy)==  28,     "RemoteMsgAcPosUpdateTy doesn't have expected size");
static_assert(sizeof(RemoteAcAnimTy)        ==   8,     "RemoteAcAnimTy doesn't have expected size");
static_assert(RemoteAcAnimTy::msgSize(V_COUNT) == 90,   "RemoteAcAnimTy for V_COUNT dataRefs doesn't have expected size");
static_assert(sizeof(RemoteMsgAcAnimTy)     ==  16,     "RemoteMsgAcAnimTy doesn't have expected size");
static_assert(sizeof(RemoteMsgAcRemoveTy)   ==  12,     "RemoteMsgAcRemoveTy doesn't have expected size");

//
// MARK: Miscellaneous
//

/// Function prototypes for callback functions to handle the received messages
struct RemoteCBFctTy {
    /// Called in flight loop before processing first aircraft
    void (*pfBeforeFirstAc)() = nullptr;
    /// Called in flight loop after processing last aircraft
    void (*pfAfterLastAc)() = nullptr;
    /// Callback for processing Settings messages
    void (*pfMsgSettings) (const std::uint32_t from[4],
                           const std::string& sFrom,
                           const RemoteMsgSettingsTy&) = nullptr;
    /// Callback for processing A/C Details messages
    void (*pfMsgACDetails) (const std::uint32_t from[4], size_t msgLen,
                            const RemoteMsgAcDetailTy&) = nullptr;
    /// Callback for processing A/C Details messages
    void (*pfMsgACPosUpdate) (const std::uint32_t from[4], size_t msgLen,
                              const RemoteMsgAcPosUpdateTy&) = nullptr;
    /// Callback for processing A/C Animation dataRef messages
    void (*pfMsgACAnim) (const std::uint32_t from[4], size_t msgLen,
                         const RemoteMsgAcAnimTy&) = nullptr;
    /// Callback for processing A/C Removal messages
    void (*pfMsgACRemove) (const std::uint32_t from[4], size_t msgLen,
                           const RemoteMsgAcRemoveTy&) = nullptr;
};

/// State of remote communcations
enum RemoteStatusTy : unsigned {
    REMOTE_OFF = 0,                 ///< no remote connectivtiy, not listening, not sending
    REMOTE_SEND_WAITING,            ///< listening for a request to send data, but not actively sending data
    REMOTE_SENDING,                 ///< actively sending aircraft data out to the network
    REMOTE_RECV_WAITING,            ///< waiting to receive data, periodically sending a token of interest
    REMOTE_RECEIVING,               ///< actively receiving data
};

/// Returns the current Remote status
RemoteStatusTy RemoteGetStatus();

/// Starts the listener, will call provided callback functions with received messages
void RemoteRecvStart (const RemoteCBFctTy& _rmtCBFcts);

/// Stops the receiver
void RemoteRecvStop ();

}


#endif

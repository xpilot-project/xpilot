/// @file       XPMP2.h
/// @brief      Header file covering all includes required for compiling XPMP2, basis for pre-compiled headers
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

#ifndef _XPMP2_h_
#define _XPMP2_h_

#if IBM
// In MINGW winsock2 must be included before windows.h, which is included by the mingw.*.h headers
#include <winsock2.h>
#include <ws2ipdef.h>           // required for sockaddr_in6 (?)
#include <iphlpapi.h>           // for GetAdaptersAddresses
#include <ws2tcpip.h>
// On Windows, 'max' and 'min' are defined macros in conflict with C++ library. Let's undefine them!
#include <direct.h>
#undef max
#undef min
#endif

// XPlaneMP 2 - Public Header Files
#include "XPMPMultiplayer.h"
#include "XPMPAircraft.h"
#include "XPCAircraft.h"
#include "XPMPPlaneRenderer.h"
#include "XPMPRemote.h"

// Standard C
#include <sys/stat.h>
#include <cmath>
#include <cstdarg>
#include <cassert>

// Standard C++
#include <string>
#include <list>
#include <map>
#include <set>
#include <array>
#include <vector>
#include <queue>
#include <valarray>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <future>
#include <thread>
#include <shared_mutex>
#include <regex>
#include <bitset>

// FMOD Sound, must be included before XPLMSound.h
#if INCLUDE_FMOD_SOUND + 0 >= 1
#include "fmod_errors.h"            // This one includes everything for the CORE API
#include "fmod_studio.h"
#endif

// X-Plane SDK
#include "XPLMUtilities.h"
#include "XPLMScenery.h"
#include "XPLMProcessing.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMCamera.h"
#include "XPLMPlanes.h"
#include "XPLMMap.h"
#include "XPLMSound.h"

// XPlaneMP 2 - Internal Header Files
#include "Utilities.h"
#include "RelatedDoc8643.h"
#include "CSLModels.h"
#include "Aircraft.h"
#include "Contrail.h"
#include "2D.h"
#include "AIMultiplayer.h"
#include "Map.h"
#include "Network.h"
#include "Remote.h"
#include "Sound.h"
#if INCLUDE_FMOD_SOUND + 0 >= 1
#include "SoundFMOD.h"
#endif

//
// MARK: Global Configurations and variables
//

#define UNKNOWN_PLUGIN_NAME "(unknown)"

namespace XPMP2 {

/// 3-way switch type
enum ThreeWaySwitchTy : int {
    SWITCH_CFG_OFF = -1,            ///< config: force off
    SWITCH_CFG_AUTO = 0,            ///< config: automatic or default
    SWITCH_CFG_ON = 1,              ///< config: force on
};

/// Stores the function and refcon pointer for plane creation/destrcution notifications
struct XPMPPlaneNotifierTy {
    XPMPPlaneNotifier_f func    = nullptr;
    void*               refcon  = nullptr;
    
    XPMPPlaneNotifierTy (XPMPPlaneNotifier_f _func = nullptr, void* _refcon = nullptr) :
    func(_func), refcon(_refcon) {}
    
    bool operator == (const XPMPPlaneNotifierTy& o)
    { return func == o.func && refcon == o.refcon; }
};

typedef std::list<XPMPPlaneNotifierTy> listXPMPPlaneNotifierTy;

/// Send a notification to all observers
void XPMPSendNotification (const Aircraft& plane, XPMPPlaneNotification _notification);

/// All global config settings and variables are kept in one structure for convenient access and central definition
struct GlobVars {
public:
    // Settings:
    /// Logging level
    logLevelTy      logLvl      = logINFO;
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Clamp all planes to the ground? Default is `false` as clamping is kinda expensive due to Y-Testing.
    bool            bClampAll   = false;
    /// Handle duplicate XPMP2::Aircraft::modeS_id by overwriting with unique id
    bool            bHandleDupId= false;
    
    /// Replace dataRefs in `.obj` files on load? (defaults to OFF!)
    bool            bObjReplDataRefs = false;
    /// Replace textures in `.obj` files on load if needed?
    bool            bObjReplTextures = true;
    /// Path to the `Obj8DataRefs.txt` file
    std::string     pathObj8DataRefs;
    /// List of dataRef replacement in `.obj` files
    listObj8DataRefsTy listObj8DataRefs;
    
    /// Name of the plugin we are serving (used as map layer name or for folders)
    std::string     pluginName  = UNKNOWN_PLUGIN_NAME;
    /// Plugin acronym used in log output
    std::string     logAcronym  = UNKNOWN_PLUGIN_NAME;

    /// Configuration callback for integer values
    int (*prefsFuncInt)(const char *, const char *, int) = XPMP2::PrefsFuncIntDefault;
    /// List of notifier functions registered for being notified of creation/destruction/model change
    listXPMPPlaneNotifierTy listObservers;
    
    /// Path to Doc8643.txt file
    std::string     pathDoc8643;
    /// Content of `Doc8643.txt` file
    mapDoc8643Ty    mapDoc8643;
    /// Paths to related.txt, relOp.txt... files
    std::array<std::string,REL_TXT_NUM> pathRelated = {};
    /// Content of `related.txt` and similar files as a map of keys to group id
    std::array<mapRelatedTy,REL_TXT_NUM> mapRelated = {};

    /// Global map of all CSL Packages, indexed by `xsb_aircraft.txt::EXPORT_NAME`
    mapCSLPackageTy mapCSLPkgs;
    /// Global map of all CSL Models, indexed by related group, aircraft type, and model id
    mapCSLModelTy   mapCSLModels;
    /// Default ICAO aircraft type designator if no match can be found
    std::string     defaultICAO = "A320";
    /// Ground vehicle type identifier (map decides icon based on this)
    std::string     carIcaoType = "ZZZC";
    /// Resource directory, to store local definitions of vertical offsets (clamping)
    std::string     resourceDir;
    
    /// Global map of all created planes
    mapAcTy         mapAc;
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    /// Label drawing overriden in `XPMP2.prf`?
    ThreeWaySwitchTy eLabelOverride = SWITCH_CFG_AUTO;
    /// Maximum distance for drawing labels? [m], defaults to 3nm
    float           maxLabelDist = 5556.0f;
    /// Cut off labels at XP's reported visibility mit?
    bool            bLabelCutOffAtVisibility = true;
    /// Label font scaling factor
    float           labelFontScaling = 1.0f;
    
    /// Do we control X-Plane's AI/Multiplayer planes?
    bool            bHasControlOfAIAircraft = false;
    /// AI Control overriden in `XPMP2.prf`?
    ThreeWaySwitchTy eAIOverride = SWITCH_CFG_AUTO;

    /// Do we feed X-Plane's maps with our aircraft positions?
    bool            bMapEnabled = true;
    /// Do we show labels with the aircraft icons?
    bool            bMapLabels = true;
    /// Map of map layer ids, i.e. for each map this is the id of the corresponding layer of ours
    mapMapLayerIDTy mapMapLayers;
    /// path to file containing plane icons for map display
    std::string     pathMapIcons;
    
    /// @brief The multicast group that we use, which is the same X-Plane is using itself for its BEACON
    /// @see <X-Plane>/Instructions/Exchanging Data with X-Plane.rtfd, chapter "DISCOVER X-PLANE BY A BEACON"
    std::string     remoteMCGroup = "239.255.1.1";    // for IPv6 try "FF02::1"
    /// The port we use is _different_ from the port the X-Plane BEACON uses, so we don't get into conflict
    int             remotePort      = 49788;
    /// The interface used to send multicast on. (Empty == auto-discovery)
    std::string     remoteSendIntf;
    /// Use auto-discovery for the MC send interface?
    bool            remoteAutoDiscovery() const { return remoteSendIntf.empty(); }
    /// Time-to-live, or mumber of hops for a multicast message
    int             remoteTTL       = 8;
    /// Buffer size, ie. max message length we send over multicast
    size_t          remoteBufSize   = 8192;
    /// Max transfer frequency per second
    int             remoteTxfFrequ  = 5;
    /// Configuration: Are we to support remote connections?
    ThreeWaySwitchTy     remoteCfg       = SWITCH_CFG_AUTO;
    /// Configuration file entry: Are we to support remote connections?
    ThreeWaySwitchTy     remoteCfgFromIni= SWITCH_CFG_AUTO;
    /// Status of remote connections to networked clients
    RemoteStatusTy  remoteStatus    = REMOTE_OFF;
    /// Are we a listener?
    bool RemoteIsListener() const { return remoteStatus == REMOTE_RECEIVING || remoteStatus == REMOTE_RECV_WAITING; }
    /// Are we a sender?
    bool RemoteIsSender() const { return remoteStatus == REMOTE_SENDING || remoteStatus == REMOTE_SEND_WAITING; }

    // Contrails
    int             contrailAltMin_ft   = 25000;    ///< [ft] Auto Contrails: Minimum altitude
    int             contrailAltMax_ft   = 45000;    ///< [ft] Auto Contrails: Maximum altitude
    int             contrailLifeTime    = 25;       ///< [s] Contrail default time to live
    bool            contrailMulti       = false;    ///< Auto-create multiple or just a single contrail?

    /// Config: Start Sound on startup?
    bool bSoundOnStartup = true;
    /// Config: Force own FMOD instance?
    bool bSoundForceFmodInstance = false;
    /// Config: Mute on Pause?
    bool bSoundMuteOnPause = true;
    /// Is the sound system available?
    bool bSoundAvail = false;
    /// Sound master volume
    float sndMasterVol = 1.0f;
    /// Is sound currently being auto-muted?
    bool bSoundAutoMuted = false;
    
    /// X-Plane's version number (XPLMGetVersions)
    int             verXPlane = -1;
    /// XPLM's SDK version number (XPLMGetVersions)
    int             verXPLM = -1;
    /// Using a modern graphics driver, ie. Vulkan/Metal?
    bool            bXPUsingModernGraphicsDriver = false;
    /// Is X-Plane configured for networked multi-computer or multiplayer setup?
    bool            bXPNetworkedSetup = false;
    /// This plugin's id
    XPLMPluginID    pluginId = 0;
    /// id of X-Plane's thread (when it is OK to use XP API calls)
    std::thread::id xpThread;

    /// Current camera location, updated every flight loop
    XPLMCameraPosition_t posCamera = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    /// Previous camera location of about a second ago
    XPLMCameraPosition_t prevCamera = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    /// Camera velocity
    float vCam_x = 0.0f, vCam_y = 0.0f, vCam_z = 0.0f;
    /// When has `prevCamera` been updated last?
    float prevCamera_ts = 0.0f;

    /// Current XP cycle number (see XPLMGetCycleNumber())
    int             xpCycleNum = 0;
    
protected:
    /// Current plane ID counter
    XPMPPlaneID  planeId = 0;
public:
    /// Get the next unique artifical plane id
    XPMPPlaneID NextPlaneId ();

public:
    /// Constructor
    GlobVars (logLevelTy _logLvl = logINFO, bool _logMdlMatch = false) :
    logLvl(_logLvl), bLogMdlMatch(_logMdlMatch) {}
    /// Read from a generic `XPMP2.prf` or `XPMP2.<logAcronym>.prf` config file
    void ReadConfigFile ();
    /// Update all settings, e.g. for logging level, by calling prefsFuncInt
    void UpdateCfgVals ();
    /// Read version numbers into verXplane/verXPLM
    void ReadVersions ();
    /// Using a modern graphics driver, ie. Vulkan/Metal?
    bool UsingModernGraphicsDriver() const { return bXPUsingModernGraphicsDriver; }
    /// Set current thread as main xp Thread
    void ThisThreadIsXP()
    { xpThread = std::this_thread::get_id(); pluginId = XPLMGetMyID(); }
    /// Is this thread XP's main thread?
    bool IsXPThread() const { return std::this_thread::get_id() == xpThread; }

    /// Update the stored camera position and velocity values
    void UpdateCameraPos ();
};

/// The one and only global variable structure
extern GlobVars glob;

}       // namespace XPMP2



#endif

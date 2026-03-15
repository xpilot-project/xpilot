/// @file       XPMPMultiplayer.cpp
/// @brief      Initialization and general control functions for XPMP2
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

#include <cstring>

#define ERR_RSRC_DIR_INVALID  "Resource directory too short: %s"
#define ERR_RSRC_DIR_UNAVAIL  "Resource directory '%s' does not exist!"
#define ERR_RSRC_FILE_UNAVAIL "File '%s' is missing in resource directory '%s'"
#define INFO_DEFAULT_ICAO       "Default ICAO aircraft type now is %s"
#define INFO_CAR_ICAO           "Ground vehicle ICAO type now is %s"
#define INFO_LOAD_CSL_PACKAGE   "Loading CSL package from %s"

// The global functions implemented here are not in our namespace for legacy reasons,
// but we use our namespace a lot:
using namespace XPMP2;

// returns the current transponder mode as human-readable text
const char* XPMPPlaneRadar_t::GetModeStr() const
{
    switch (mode) {
        case xpmpTransponderMode_Off:           return "off";
        case xpmpTransponderMode_Standby:       return "standby";
        case xpmpTransponderMode_ModeA:         return "Mode A";
        case xpmpTransponderMode_ModeC:         return "Mode C";
        case xpmpTransponderMode_Test:          return "Test";
        case xpmpTransponderMode_ModeS_Gnd:     return "Mode S GND";
        case xpmpTransponderMode_ModeS_TAOnly:  return "Mode S TA-Only";
        case xpmpTransponderMode_ModeS_TARA:    return "Mode S TA/RA";
    }
    return "?";
}


//
// MARK: Initialization
//

namespace XPMP2 {

/// Validates existence of a given file and saves its full path location
static bool FindResFile (const char* fName, std::string& outPath,
                         bool bRequired = true)
{
    std::string path = glob.resourceDir + fName;
    if (ExistsFile(path)) {
        outPath = std::move(path);
        return true;
    } else {
        if (bRequired) {
            LOG_MSG(logFATAL, ERR_RSRC_FILE_UNAVAIL,
                    fName, glob.resourceDir.c_str());
        }
        return false;
    }
}

/// Validate all required files are available in the resource directory
const char* XPMPValidateResourceFiles (const char* resourceDir)
{
    // Store the resource dir
    if (!resourceDir || strlen(resourceDir) < 5) {
        LOG_MSG(logERR, ERR_RSRC_DIR_INVALID, resourceDir ? resourceDir : "<nullptr>");
        return "resourceDir too short / nullptr";
    }
    glob.resourceDir = TOPOSIX(resourceDir);
    
    // for path validation it must not end in the path separator
    if (glob.resourceDir.back() == PATH_DELIM_STD)
        glob.resourceDir.pop_back();    
    // Check for its existence
    if (!IsDir(glob.resourceDir)) {
        LOG_MSG(logERR, ERR_RSRC_DIR_UNAVAIL, glob.resourceDir.c_str());
        return "Resource directory unavailable";
    }
    // now add a separator to the path for ease of future use
    glob.resourceDir += PATH_DELIM_STD;
    
    // Validate and save a few files
    if (!FindResFile(RSRC_RELATED, glob.pathRelated[REL_TXT_DESIGNATOR]))
        return "related.txt not found in resource directory";
    if (!FindResFile(RSRC_REL_OP, glob.pathRelated[REL_TXT_OP], false)) {
        LOG_MSG(logWARN, "Optional file relOp.txt not available in resource directory");
    }
    if (!FindResFile(RSRC_DOC8643, glob.pathDoc8643))
        return "Doc8643.txt not found in resource directory";
    if (!FindResFile(RSRC_MAP_ICONS, glob.pathMapIcons))
        return "MapIcons.png not found in resource directory";
    // this last one is not essentially required:
    if (!FindResFile(RSRC_OBJ8DATAREFS, glob.pathObj8DataRefs, false) &&
        glob.bObjReplDataRefs) {
        LOG_MSG(logWARN, ERR_RSRC_FILE_UNAVAIL,
                RSRC_OBJ8DATAREFS, glob.resourceDir.c_str());

    }
    
    // Success
    return "";
}

};

// This routine initializes legacy portions of the multiplayer library
const char *    XPMPMultiplayerInitLegacyData(const char* inCSLFolder,
                                              const char* inPluginName,
                                              const char* resourceDir,
                                              XPMPIntPrefsFuncTy inIntPrefsFunc,
                                              const char* inDefaultICAO,
                                              const char* inPluginLogAcronym)
{
    // We just pass on the calls to the individual functions:
    
    // Internal init first
    const char* ret = XPMPMultiplayerInit (inPluginName,
                                           resourceDir,
                                           inIntPrefsFunc,
                                           inDefaultICAO,
                                           inPluginLogAcronym);
    if (ret[0])                                     // failed?
        return ret;
    
    // Then try loading the first set of CSL models
    return XPMPLoadCSLPackage(inCSLFolder);
}

// Init preference callback functions
// and storage location for user vertical offset config file
const char *    XPMPMultiplayerInit(const char* inPluginName,
                                    const char* resourceDir,
                                    XPMPIntPrefsFuncTy inIntPrefsFunc,
                                    const char* inDefaultICAO,
                                    const char* inPluginLogAcronym)
{
    // Initialize random number generator
    std::srand(unsigned(std::time(nullptr)));

    /// Assume that this is XP's main thread
    glob.ThisThreadIsXP();
    
    // Store the pointers to the configuration callback functions
    glob.prefsFuncInt   = inIntPrefsFunc    ? inIntPrefsFunc    : PrefsFuncIntDefault;
    
    // Get the plugin's name and store it for later reference
    XPMPSetPluginName(inPluginName, inPluginLogAcronym);
    if (glob.pluginName == UNKNOWN_PLUGIN_NAME) {
        char szPluginName[256];
        XPLMGetPluginInfo(XPLMGetMyID(), szPluginName, nullptr, nullptr, nullptr);
        glob.pluginName = szPluginName;
        if (!inPluginLogAcronym)
            glob.logAcronym = glob.pluginName;
    }

    // Get X-Plane's version numbers
    glob.ReadVersions();    
#if defined(XPMP2_DLLEXPORT)
    const char* MSG_IS_DLL = "(DLL) ";
#else
    const char* MSG_IS_DLL = "";
#endif
#if INCLUDE_FMOD_SOUND + 0 >= 1
    const char* MSG_WITH_FMOD = "with FMOD sound support ";
#else
    const char* MSG_WITH_FMOD = "";
#endif
    LOG_MSG(logINFO, "XPMP2 %d.%d.%d %s%sinitializing under X-Plane version %d/%s and XPLM version %d",
            XPMP2_VER_MAJOR, XPMP2_VER_MINOR, XPMP2_VER_PATCH,
            MSG_IS_DLL, MSG_WITH_FMOD,
            glob.verXPlane, GetGraphicsDriverTxt(), glob.verXPLM);
    
    // Read a potential global XPMP2-specific config file
    glob.ReadConfigFile();

    // And get initial config values (defines, e.g., log level, which we'll need soon)
    glob.UpdateCfgVals();

    // Look for all supplemental files
    const char* ret = XPMPValidateResourceFiles(resourceDir);
    if (ret[0]) return ret;
    
    // Define the default ICAO aircraft type
    XPMPSetDefaultPlaneICAO(inDefaultICAO);
    
    // Initialize all modules
    CSLModelsInit();
    AcInit();
    TwoDInit();
    AIMultiInit();
    MapInit();
    RemoteInit();
    if (glob.bSoundOnStartup)
        XPMPSoundEnable(true);
    
    // Load related.txt
    ret = RelatedLoad(glob.pathRelated.data(), glob.pathRelated.size());
    if (ret[0]) return ret;

    // Load Doc8643.txt
    ret = Doc8643Load(glob.pathDoc8643);
    if (ret[0]) return ret;
    
    // If available (it is not required) load the Obj8DataRefs.txt file
    if (!glob.pathObj8DataRefs.empty()) {
        ret = Obj8DataRefsLoad(glob.pathObj8DataRefs);
        if (ret[0]) return ret;
    }

    // Success
    return "";
}

// Overrides the plugin's name to be used in Log output
void XPMPSetPluginName (const char* inPluginName,
                        const char* inPluginLogAcronym)
{
    if (inPluginName)
        glob.pluginName = inPluginName;
    glob.logAcronym = inPluginLogAcronym ? inPluginLogAcronym : glob.pluginName;
}

// Undoes above init functions
void XPMPMultiplayerCleanup()
{
    LOG_MSG(logINFO, "XPMP2 cleaning up...")

    // Cleanup all modules in revers order of initialization
    SoundCleanup();
    RemoteCleanup();
    MapCleanup();
    AIMultiCleanup();
    TwoDCleanup();
    AcCleanup();
    CSLModelsCleanup();
    
    // Unregister all notification callbacks
    glob.listObservers.clear();
}

// OBJ7 is not supported
const char * XPMPMultiplayerOBJ7SupportEnable(const char *)
{
    return "OBJ7 format is no longer supported";
}

//
// MARK: CSL Package Handling
//

// Loads a collection of planes models
const char *    XPMPLoadCSLPackage(const char * inCSLFolder)
{
    // Do load the CSL Models in the given path
    if (inCSLFolder) {
        LOG_MSG(logINFO, INFO_LOAD_CSL_PACKAGE, StripXPSysDir(inCSLFolder).c_str());
        return CSLModelsLoad(inCSLFolder);
    }
    else
        return "<nullptr> provided";
}

// checks what planes are loaded and loads any that we didn't get
void            XPMPLoadPlanesIfNecessary()
{}

// returns the number of found models
int XPMPGetNumberOfInstalledModels()
{
    return (int)glob.mapCSLModels.size();
}

// return model info (unsafe)
void XPMPGetModelInfo(int inIndex, const char **outModelName, const char **outIcao, const char **outAirline, const char **outLivery)
{
    // sanity check: index too high?
    if (inIndex >= XPMPGetNumberOfInstalledModels()) {
        LOG_MSG(logDEBUG, "inIndex %d too high, have only %d models",
                inIndex, (int)XPMPGetNumberOfInstalledModels());
        return;
    }
    
    // get the inIndex-th model
    mapCSLModelTy::const_iterator iterMdl = glob.mapCSLModels.cbegin();
    std::advance(iterMdl, inIndex);
#if _MSC_VER
#pragma warning(push)
    // I don't know why, but in this function, and only in this, MS warns about throwing an exception
#pragma warning(disable: 4297)
#endif
    LOG_ASSERT(iterMdl != glob.mapCSLModels.cend());
#if _MSC_VER
#pragma warning(pop)
#endif

    // Copy string pointers back. We just pass back pointers into our CSL Model object
    // as we can assume that the CSL Model object exists quite long.
    const CSLModel& csl = iterMdl->second;
    if (outModelName)   *outModelName   = csl.GetId().data();
    if (outIcao)        *outIcao        = csl.GetIcaoType().data();
    if (outAirline)     *outAirline     = csl.GetIcaoAirline().data();
    if (outLivery)      *outLivery      = csl.GetLivery().data();
}

// return model info
void XPMPGetModelInfo2(int inIndex, std::string& outModelName,  std::string& outIcao, std::string& outAirline, std::string& outLivery)
{
    // sanity check: index too high?
    if (inIndex >= XPMPGetNumberOfInstalledModels()) {
        LOG_MSG(logDEBUG, "inIndex %d too high, have only %d models",
                inIndex, (int)XPMPGetNumberOfInstalledModels());
        return;
    }
    
    // get the inIndex-th model
    mapCSLModelTy::const_iterator iterMdl = glob.mapCSLModels.cbegin();
    std::advance(iterMdl, inIndex);
#if _MSC_VER
#pragma warning(push)
    // I don't know why, but in this function, and only in this, MS warns about throwing an exception
#pragma warning(disable: 4297)
#endif
    LOG_ASSERT(iterMdl != glob.mapCSLModels.cend());
#if _MSC_VER
#pragma warning(pop)
#endif

    // Copy strings into provided buffers
    const CSLModel& csl = iterMdl->second;
    outModelName = csl.GetId();
    outIcao      = csl.GetIcaoType();
    outAirline   = csl.GetIcaoAirline();
    outLivery    = csl.GetLivery();
}


// test model match quality for given parameters
int         XPMPModelMatchQuality(const char *              inICAO,
                                  const char *              inAirline,
                                  const char *              inLivery)
{
    CSLModel* pModel;
    return CSLModelMatching(inICAO      ? inICAO : "",
                            inAirline   ? inAirline : "",
                            inLivery    ? inLivery : "",
                            pModel);
}

// is ICAO a valid one according to our records?
bool            XPMPIsICAOValid(const char *                inICAO)
{
    if (!inICAO) return false;
    return Doc8643IsTypeValid(inICAO);
}

//
// MARK: Create and Manage Planes
//

// Create a new plane
XPMPPlaneID XPMPCreatePlane(const char *            inICAOCode,
                            const char *            inAirline,
                            const char *            inLivery,
                            XPMPPlaneData_f         inDataFunc,
                            void *                  inRefcon,
                            XPMPPlaneID             inModeS_id)
{
    // forward to XPMPCreatePlaneWithModelName with no model name
    return XPMPCreatePlaneWithModelName(nullptr,        // no model name
                                        inICAOCode,
                                        inAirline,
                                        inLivery,
                                        inDataFunc,
                                        inRefcon,
                                        inModeS_id);
}

// Create a new plane, providing a model
XPMPPlaneID XPMPCreatePlaneWithModelName(const char *       inModelName,
                                         const char *       inICAOCode,
                                         const char *       inAirline,
                                         const char *       inLivery,
                                         XPMPPlaneData_f    inDataFunc,
                                         void *             inRefcon,
                                         XPMPPlaneID        inModeS_id)
{
    try {
#ifndef __clang_analyzer__
        LegacyAircraft* pAc = new LegacyAircraft(inICAOCode,
                                                 inAirline,
                                                 inLivery,
                                                 inDataFunc,
                                                 inRefcon,
                                                 inModeS_id,
                                                 inModelName);
        // This is not leaking memory, the pointer is in glob.mapAc as taken care of by the constructor
        return pAc->GetModeS_ID();
#endif
    }
    catch (const XPMP2Error& e) {
        // This might be thrown in case of problems creating the object
        LOG_MSG(logERR, "Could not create plane object for %s/%s/%s/0x%06X/%s: %s",
                inICAOCode  ? inICAOCode    : "<null>",
                inAirline   ? inAirline     : "<null>",
                inLivery    ? inLivery      : "<null>",
                inModeS_id,
                inModelName ? inModelName   : "<null>",
                e.what());
        return 0;
    }
}

// Destroy a plane by just deleting the object, the destructor takes care of the rest
void XPMPDestroyPlane(XPMPPlaneID _id)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        delete pAc;
}

// Show/Hide the aircraft temporarily without destroying the object
void XPMPSetPlaneVisibility(XPMPPlaneID _id, bool _bVisible)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        pAc->SetVisible(_bVisible);
}

// Change a plane's model
int     XPMPChangePlaneModel(XPMPPlaneID            _id,
                             const char *           inICAO,
                             const char *           inAirline,
                             const char *           inLivery)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        return pAc->ChangeModel(inICAO      ? inICAO : "",
                                inAirline   ? inAirline : "",
                                inLivery    ? inLivery : "");
    else
        return -1;
}

// return the name of the model in use
int     XPMPGetPlaneModelName(XPMPPlaneID             inPlaneID,
                              char *                  outTxtBuf,
                              int                     outTxtBufSize)
{
    Aircraft* pAc = AcFindByID(inPlaneID);
    if (pAc) {
        std::string mdlName = pAc->GetModelName();
        if (outTxtBuf && outTxtBufSize > 0) {
            strncpy(outTxtBuf, mdlName.c_str(), (size_t)outTxtBufSize);
            outTxtBuf[outTxtBufSize-1] = 0;         // safety measure: ensure zero-termination
        }
        return (int)mdlName.length();
    } else {
        return -1;
    }
}

// return plane's ICAO / livery
void XPMPGetPlaneICAOAndLivery(XPMPPlaneID inPlane,
                               char *      outICAOCode,    // Can be NULL
                               char *      outLivery)      // Can be NULL
{
    Aircraft* pAc = AcFindByID(inPlane);
    if (pAc) {
        // this is not a safe operation...but the way the legay interface was defined:
        if (outICAOCode) strcpy (outICAOCode, pAc->acIcaoType.c_str());
        if (outLivery)   strcpy (outLivery,   pAc->acLivery.c_str());
    }
}

// fetch plane data (unsupported in XPMP2)
XPMPPlaneCallbackResult XPMPGetPlaneData(XPMPPlaneID,
                                         XPMPPlaneDataType,
                                         void *)
{
    LOG_MSG(logERR, "Calling this function from the outside should not be needed!");
    return xpmpData_Unavailable;
}

// This function returns the quality level for the nominated plane's
int         XPMPGetPlaneModelQuality(XPMPPlaneID _inPlane)
{
    Aircraft* pAc = AcFindByID(_inPlane);
    if (pAc)
        return pAc->GetMatchQuality();
    else
        return -1;
}

// number of planes in existence
long XPMPCountPlanes()
{
    return (long)glob.mapAc.size();
}

// return nth plane
XPMPPlaneID XPMPGetNthPlane(long index)
{
    if (index < 0 || index >= XPMPCountPlanes())
        return 0;
    auto iter = glob.mapAc.cbegin();
    std::advance(iter, index);
    return iter->second->GetModeS_ID();
}


// Returns the underlying aircraft object, or `nullptr` if `_id` is invalid
XPMP2::Aircraft* XPMPGetAircraft (XPMPPlaneID _id)
{
    return AcFindByID(_id);
}


// Define default aircraft and ground vehicle ICAO types
void XPMPSetDefaultPlaneICAO(const char* _acIcaoType,
                             const char* _carIcaoType)
{
    // Plane default
    if (_acIcaoType) {
        glob.defaultICAO = _acIcaoType;
        LOG_MSG(logINFO, INFO_DEFAULT_ICAO, _acIcaoType);
    }

    // Car identification
    if (_carIcaoType) {
        glob.carIcaoType = _carIcaoType; 
        LOG_MSG(logINFO, INFO_CAR_ICAO, _carIcaoType);
    }
}

//
// MARK: Plane Observation
//

/*
 * XPMPRegisterPlaneCreateDestroyFunc
 *
 * This function registers a notifier functionfor obeserving planes being created and destroyed.
 *
 */
void            XPMPRegisterPlaneNotifierFunc(XPMPPlaneNotifier_f       inFunc,
                                              void *                    inRefcon)
{
    // Avoid duplicate entries
    XPMPPlaneNotifierTy observer (inFunc, inRefcon);
    if (std::find(glob.listObservers.begin(),
                  glob.listObservers.end(),
                  observer) == glob.listObservers.end()) {
        glob.listObservers.emplace_back(std::move(observer));
        LOG_MSG(logDEBUG, "%lu observers registered",
                (unsigned long)glob.listObservers.size());
    }
}

/*
 * XPMPUnregisterPlaneCreateDestroyFunc
 *
 * This function canceles a registration for a notifier functionfor obeserving
 * planes being created and destroyed.
 */
void            XPMPUnregisterPlaneNotifierFunc(XPMPPlaneNotifier_f     inFunc,
                                                void *                  inRefcon)
{
    auto iter = std::find(glob.listObservers.begin(),
                          glob.listObservers.end(),
                          XPMPPlaneNotifierTy (inFunc, inRefcon));
    if (iter != glob.listObservers.cend()) {
        glob.listObservers.erase(iter);
        LOG_MSG(logDEBUG, "%lu observers registered",
                (unsigned long)glob.listObservers.size());
    }
}

namespace XPMP2 {
// Send a notification to all observers
void XPMPSendNotification (const Aircraft& plane, XPMPPlaneNotification _notification)
{
    for (const XPMPPlaneNotifierTy& n: glob.listObservers)
        n.func(plane.GetModeS_ID(),
               _notification,
               n.refcon);
}
}


//
// MARK: PLANE RENDERING API
//       Completely unsupported in XPMP2
//

// This function would set the plane renderer.
void        XPMPSetPlaneRenderer(XPMPRenderPlanes_f, void *)
{
    LOG_MSG(logERR, "XPMPSetPlaneRenderer() is NOT SUPPORTED in XPMP2");
}

void XPMPDumpOneCycle(void)                 {}
void XPMPInitDefaultPlaneRenderer(void)     {}
void XPMPDefaultPlaneRenderer(int)          {}
void XPMPDeinitDefaultPlaneRenderer(void)   {}

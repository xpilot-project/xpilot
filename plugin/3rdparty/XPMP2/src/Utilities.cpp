/// @file       Utilities.cpp
/// @brief      Miscellaneous utility functions, including logging
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

namespace XPMP2 {

#if DEBUG
GlobVars glob (logDEBUG, true);
#else
/// The one and only global variable structure
GlobVars glob;
#endif

/// Store a message for immediate (XP thread) or later output (worker thread)
void PushMsg (const std::string& sMsg);

/// Flush all pending messages to Log.txt
void FlushMsgs ();

//
// MARK: Configuration
//

/// Get the next unique artifical plane id
XPMPPlaneID GlobVars::NextPlaneId ()
{
    // increment until we find an unused number
    bool bWrappedAround = false;
    while (mapAc.count(++planeId) != 0) {
        if (planeId >= MAX_MODE_S_ID) {
            if (bWrappedAround)         // we already wrapped around once and found nothing free???
                THROW_ERROR("Found no available mode S id!!!");
            bWrappedAround = true;
            planeId = MIN_MODE_S_ID - 1;
        }
    }
    // found something, which is not yet taken, return it:
    return planeId;
}

/// Convert strings `on`, `auto`, `off` to enums of type `ThreeWaySwitchTy`
ThreeWaySwitchTy Read3WaySwitch (std::string& sVal,
                                 const std::string& sKey,
                                 const std::string& cfgFileName,
                                 ThreeWaySwitchTy defVal)
{
    str_tolower(sVal);
    if (sVal == "on")              return SWITCH_CFG_ON;
    else if (sVal == "auto")       return SWITCH_CFG_AUTO;
    else if (sVal == "off")        return SWITCH_CFG_OFF;
    else {
        LOG_MSG(logWARN, "Ignored unknown value '%s' for '%s' in file '%s'",
                sVal.c_str(), sKey.c_str(), cfgFileName.c_str());
    }
    return defVal;
}

// Read from a generic `XPMP2.prf` or `XPMP2.<logAcronym>.prf` config file
/// @details Goal is to provide some few configuration options independend
///          of the plugin using XPMP2. This is useful e.g. when configuring
///          non-standard network settings for remote connections.\n
///          For even greater flexibility the function will try a plugin-specific
///          file first, named "XPMP2.<logAcronym>.prf" (with spaces replaced
///          with underscores) and if that is not found
///          it tries the generic file "XPMP2.prf".\n
///          A config file is completely optional, none is required.
void GlobVars::ReadConfigFile ()
{
    // Put together the plugin-spcific file name
    std::string cfgFileName = logAcronym;
    trim(cfgFileName);
    cfgFileName += ".prf";
    std::replace(cfgFileName.begin(), cfgFileName.end(), ' ', '_');
    cfgFileName = GetXPSystemPath() + "Output/preferences/XPMP2." + cfgFileName;
    
    // If that file doesn't exist try the generic version
    if (!ExistsFile(cfgFileName)) {
        cfgFileName = GetXPSystemPath() + "Output/preferences/XPMP2.prf";
        if (!ExistsFile(cfgFileName))       // does also not exist, that's OK then
            return;
    }
    
    // Open that config file, which we think does exist
    LOG_MSG(logINFO, "Reading configuration from %s", cfgFileName.c_str());
    std::ifstream fIn (cfgFileName);
    if (!fIn) {
        char sErr[100];
        strerror_s(sErr, sizeof(sErr), errno);
        LOG_MSG(logERR, "Could not open config file '%s': %s", cfgFileName.c_str(), sErr);
        return;
    }
    
    // Read from the file
    std::string lnBuf;
    int errCnt = 0;
    while (fIn && errCnt <= 3) {
        safeGetline(fIn, lnBuf);            // read line and break into tokens, delimited by spaces
        if (lnBuf.empty() ||                // skip empty lines or comments without warning
            lnBuf[0] == '#')
            continue;

        // otherwise should be 2 tokens, separated by the first space
        size_t spcPos = lnBuf.find(" ");
        if (spcPos == std::string::npos || spcPos == 0 || spcPos == lnBuf.length()-1) {
            // Too few words in that line
            LOG_MSG(logWARN, "Expected at least two words (key, value) in config file '%s', line '%s' -> ignored",
                cfgFileName.c_str(), lnBuf.c_str());
            errCnt++;
            continue;
        }
        const std::string sKey = lnBuf.substr(0, spcPos);
        std::string sVal = lnBuf.substr(spcPos + 1);

        // *** Process actual configuration entries ***
        
        int iVal = 0;
        try {iVal = (int) std::stol(sVal); }
        catch (...) { iVal = 0; }
        if (sKey == "logLvl")              logLvl = (logLevelTy) clamp<int>(iVal, int(logDEBUG), int(logFATAL));
        else if (sKey == "defaultICAO")    defaultICAO = sVal;
        else if (sKey == "carIcaoType")    carIcaoType = sVal;
        else if (sKey == "overrideLabelsDraw")
            switch (glob.eLabelOverride = Read3WaySwitch(sVal, sKey, cfgFileName, glob.eLabelOverride)) {
                case SWITCH_CFG_ON:     glob.bDrawLabels = true; break;
                case SWITCH_CFG_OFF:    glob.bDrawLabels = false; break;
                case SWITCH_CFG_AUTO:   break;
            }
        else if (sKey == "overrideTCAS_Control")
            glob.eAIOverride = Read3WaySwitch(sVal, sKey, cfgFileName, glob.eAIOverride);
        else if (sKey == "remoteSupport")
            glob.remoteCfg = glob.remoteCfgFromIni = Read3WaySwitch(sVal, sKey, cfgFileName, glob.remoteCfg);
        else if (sKey == "remoteMCGroup")  remoteMCGroup = sVal;
        else if (sKey == "remotePort")     remotePort = iVal;
        else if (sKey == "remoteSendIntf") remoteSendIntf = sVal;
        else if (sKey == "remoteTTL")      remoteTTL = iVal;
        else if (sKey == "remoteBufSize")  remoteBufSize = (size_t)iVal;
        else if (sKey == "remoteTxfFrequ") remoteTxfFrequ = iVal;
        else {
            LOG_MSG(logWARN, "Ignored unknown config item '%s' from file '%s'",
                    sKey.c_str(), cfgFileName.c_str());
        }
    }

    // problem was not just eof?
    if (!fIn && !fIn.eof()) {
        char sErr[100];
        strerror_s(sErr, sizeof(sErr), errno);
        LOG_MSG(logERR, "Could not read from config file '%s': %s",
                cfgFileName.c_str(), sErr);
        return;
    }
    
    // close file
    fIn.close();

    // too many warnings?
    if (errCnt > 3) {
        LOG_MSG(logERR, "Too many errors while trying to process config file '%s'",
                cfgFileName.c_str());
        return;
    }
    
}


/// Default config function just always returns the provided default value
int     PrefsFuncIntDefault     (const char *, const char *, int _default)
{
    return _default;
}

// Update all config values, e.g. for logging level, by calling prefsFuncInt
void GlobVars::UpdateCfgVals ()
{
    // Let's do the update only about every 2 seconds
    static float timLstCheck = 0.0f;
    if (!CheckEverySoOften(timLstCheck, 2.0f))
        return;
    
    LOG_ASSERT(prefsFuncInt);
    
    // Ask for logging level
    int i = prefsFuncInt(XPMP_CFG_SEC_DEBUG, XPMP_CFG_ITM_LOGLEVEL, logLvl);
    if (i < logDEBUG) i = logDEBUG;
    if (i > logMSG) i = logMSG;
    logLvl = logLevelTy(i);
    
    // Ask for replacing dataRefs in OBJ8 files
    bObjReplDataRefs = prefsFuncInt(XPMP_CFG_SEC_MODELS, XPMP_CFG_ITM_REPLDATAREFS, bObjReplDataRefs) != 0;
    
    // Ask for replacing textures in OBJ8 files
    bObjReplTextures = prefsFuncInt(XPMP_CFG_SEC_MODELS, XPMP_CFG_ITM_REPLTEXTURE, bObjReplTextures) != 0;
    
    // Ask for clam-to-ground config
    bClampAll = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CLAMPALL, bClampAll) != 0;
    
    // Ask for handling of duplicate XPMP2::Aircraft::modeS_id
    bHandleDupId = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_HANDLE_DUP_ID, bHandleDupId) != 0;

    // Ask for remote support
    i = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_SUPPORT_REMOTE, remoteCfg);
    if (i == 0)     remoteCfg = remoteCfgFromIni;       // if plugin says "AUTO", then use config file's value
    else if (i < 0) remoteCfg = SWITCH_CFG_OFF;
    else            remoteCfg = SWITCH_CFG_ON;

    // Contrails
    contrailAltMin_ft = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CONTR_MIN_ALT, contrailAltMin_ft);
    contrailAltMax_ft = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CONTR_MAX_ALT, contrailAltMax_ft);
    contrailLifeTime  = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CONTR_LIFE,    contrailLifeTime);
    contrailMulti     = prefsFuncInt(XPMP_CFG_SEC_PLANES, XPMP_CFG_ITM_CONTR_MULTI,   contrailMulti) != 0;
    
    // Ask for enabling sound and mute-on-pause
    bSoundOnStartup = prefsFuncInt(XPMP_CFG_SEC_SOUND, XPMP_CFG_ITM_ACTIVATE_SOUND, bSoundOnStartup) != 0;
    bSoundForceFmodInstance = prefsFuncInt(XPMP_CFG_SEC_SOUND, XPMP_CFG_ITM_FMOD_INSTANCE, bSoundForceFmodInstance) != 0;
    bSoundMuteOnPause = prefsFuncInt(XPMP_CFG_SEC_SOUND, XPMP_CFG_ITM_MUTE_ON_PAUSE, bSoundMuteOnPause) != 0;

    // Ask for model matching logging
    bLogMdlMatch = prefsFuncInt(XPMP_CFG_SEC_DEBUG, XPMP_CFG_ITM_MODELMATCHING, bLogMdlMatch) != 0;
    
    // Fetch the network / multiplayer setup from X-Plane, which theoretically can change over time
    static XPLMDataRef drIsExternalVisual       = XPLMFindDataRef("sim/network/dataout/is_external_visual");        // int/boolean
    static XPLMDataRef drIsMultiplayer          = XPLMFindDataRef("sim/network/dataout/is_multiplayer_session");    // int/boolean
    static XPLMDataRef drTrackExternalVisual    = XPLMFindDataRef("sim/network/dataout/track_external_visual");     // int[20]/boolean
    const bool bWasNetworkedSetup = bXPNetworkedSetup;
    bXPNetworkedSetup = false;
    if (XPLMGetDatai(drIsExternalVisual)) {
        if (!bWasNetworkedSetup) LOG_MSG(logINFO, "This X-Plane instance is configured as an External Visual.");
        bXPNetworkedSetup = true;
    }
    else if (XPLMGetDatai(drIsMultiplayer)) {
        if (!bWasNetworkedSetup) LOG_MSG(logINFO, "This X-Plane instance is part of a multiplayer session.");
        bXPNetworkedSetup = true;
    }
    else {
        static int aNull[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        int ai[20];
        XPLMGetDatavi(drTrackExternalVisual, ai, 0, sizeof(ai)/sizeof(ai[0]));
        if (std::memcmp(ai, aNull, sizeof(ai)) != 0) {
            if (!bWasNetworkedSetup) LOG_MSG(logINFO, "This X-Plane instance is a Master with one or more External Visuals.");
            bXPNetworkedSetup = true;
        }
        // Last thing we check: Is the XPLM2 Remote Client running?
        // Because if it is then the user clearly intended communication with it, even locally
        else {
            // Note: Finding a plugin can fail during startup depending on startup sequence!
            //       So it is never sufficient to search for a plugin once only.
            static XPLMPluginID idRemoteClient = XPLM_NO_PLUGIN_ID;
            static int nSearches = 5;           // we search up to 5 times for this reason
            if (idRemoteClient == XPLM_NO_PLUGIN_ID && nSearches-- > 0)
                idRemoteClient = XPLMFindPluginBySignature(REMOTE_SIGNATURE);
            if (idRemoteClient != XPLM_NO_PLUGIN_ID) {
                if (!bWasNetworkedSetup) LOG_MSG(logINFO, "The XPMP2 Remote Client is running locally.");
                bXPNetworkedSetup = true;
            }
        }
    }
    // Previously we were in a networked setup, but now no longer?
    if (bWasNetworkedSetup && !bXPNetworkedSetup) {
        LOG_MSG(logINFO, "This X-Plane instance is no longer in any networked setup.");
    }
    // Give the Remote module a chance to handle any changes in status
    // (which not only depends on bXPNetworkedSetup but also on number of planes)
    RemoteSenderUpdateStatus();

}

// Read version numbers into verXplane/verXPLM
void GlobVars::ReadVersions ()
{
    XPLMHostApplicationID hostID = -1;
    XPLMGetVersions(&glob.verXPlane, &glob.verXPLM, &hostID);
    
    // Also read if we are using Vulkan/Metal
    XPLMDataRef drUsingModernDriver = XPLMFindDataRef("sim/graphics/view/using_modern_driver");
    if (drUsingModernDriver)            // dataRef not defined before 11.50
        bXPUsingModernGraphicsDriver = XPLMGetDatai(drUsingModernDriver) != 0;
    else
        bXPUsingModernGraphicsDriver = false;
}


// Update the stored camera position and velocity values
void GlobVars::UpdateCameraPos ()
{
    // Always update current camera position
    XPLMReadCameraPosition(&posCamera);
    
    // Compare to previous position: If we moved more than 100m
    // (in a flight loop, so at max 1/20s) then camera position was changed manually
    const float prev_ts = prevCamera_ts;
    const float dx = posCamera.x - prevCamera.x;
    const float dy = posCamera.y - prevCamera.y;
    const float dz = posCamera.z - prevCamera.z;
    if (sqr(dx) + sqr(dy) + sqr(dz) > 10000.0f)
    {
        vCam_x = 0.0f;                                  // velocity is invalid, avoid too high values causing too high Doppler effects
        vCam_y = 0.0f;
        vCam_z = 0.0f;
        prevCamera_ts = 0.0f;                           // ensure new velocity is calculated next time round
        prevCamera = posCamera;
    }
    // Update velocity only every second
    else if (CheckEverySoOften(prevCamera_ts, 1.0f))
    {
        const float dt = prevCamera_ts - prev_ts;       // delta time (about 1s)
        vCam_x = dx / dt;
        vCam_y = dy / dt;
        vCam_z = dz / dt;
        prevCamera = posCamera;
    }
}


//
// MARK: File access helpers
//

// Windows is missing a few simple macro definitions
#if !defined(S_ISDIR)
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

// Does a file path exist? (in absence of <filesystem>, which XCode refuses to ship)
/// @see https://stackoverflow.com/a/51301928
bool ExistsFile (const std::string& filename)
{
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

// Is path a directory?
bool IsDir (const std::string& path)
{
    struct stat buffer;
    if (stat (path.c_str(), &buffer) != 0)  // get stats...error?
        return false;                       // doesn't exist...no directory either
    return S_ISDIR(buffer.st_mode);         // check for S_IFDIR mode flag
}

// Create directory if it does not exist
bool CreateDir(const std::string& path)
{
    // Just try creating it...
#if IBM
    if (_mkdir(path.c_str()) == 0)
#else
    if (mkdir(path.c_str(), 0775) == 0)
#endif
        return true;

    // If creation failed, directory might still have already existed
    return IsDir(path);
}

// Copy file if source is newer or destination missing
bool CopyFileIfNewer(const std::string& source, const std::string& destDir)
{
    // Check source
    struct stat bufSource;
    if (stat(source.c_str(), &bufSource) != 0)
        // source doesn't exist
        return false;               
    size_t nPos = source.find_last_of("/\\");       // find beginning of file name
    if (nPos == std::string::npos)
        return false;

    // Check destination directory
    if (destDir.empty())
        return false;
    std::string target = destDir;
    if (target.back() == '\\' ||
        target.back() == '/')                       // remove traling (back)slash
        target.pop_back();
    if (!CreateDir(target))
        // directory does not exist and could be be created
        return false;

    // Check target file
    target += source.substr(nPos);
    struct stat bufTarget;
    if (stat(target.c_str(), &bufTarget) == 0)                  // file exists?
    {
        // is target not older than source?
        if (bufTarget.st_mtime >= bufSource.st_mtime)
            // Skip copy, all good already
            return true;
    }

    // Target dir exists, target file does not or is older -> Do copy!
    // https://stackoverflow.com/a/10195497
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(target, std::ios::binary | std::ios::trunc);
    if (!src || !dst)
        return false;
    dst << src.rdbuf();

    // all good?
    if (src.good() && dst.good()) {
        LOG_MSG(logINFO, "Copied %s to %s", source.c_str(), destDir.c_str());
        return true;
    }
    else {
        LOG_MSG(logERR, "FAILED copying %s to %s", source.c_str(), destDir.c_str());
        return true;
    }
}

// List of files in a directory (wrapper around XPLMGetDirectoryContents)
std::list<std::string> GetDirContents (const std::string& path)
{
    std::list<std::string> l;               // the list to be returned
    char szNames[4048];                     // buffer for file names
    char* indices[256];                     // buffer for indices to beginnings of names
    int start = 0;                          // first file to return
    int numFiles = 0;                       // number of files returned (per batch)
    bool bFinished = false;
    
    // Call XPLMGetDirectoryContents as often as needed to read all directory content
    do {
        numFiles = 0;
        bFinished = XPLMGetDirectoryContents(path.c_str(),
                                             start,
                                             szNames, sizeof(szNames),
                                             indices, sizeof(indices)/sizeof(*indices),
                                             NULL, &numFiles);
        // process (the batch of) files we received now
        for (int i = 0; i < numFiles; ++i)
            if (indices[i][0] != '.')           // skip parent_dir and hidden entries
                l.push_back(indices[i]);
        // next batch start (if needed)
        start += numFiles;
    } while(!bFinished);
    
    // return the list of files
    return l;
}

/// @details Read a text line, handling both Windows (CRLF) and Unix (LF) ending
/// Code makes use of the fact that in both cases LF is the terminal character.
/// So we read from file until LF (_without_ widening!).
/// In case of CRLF files there then is a trailing CR, which we just remove.
std::istream& safeGetline(std::istream& is, std::string& t)
{
    // read a line until LF
    std::getline(is, t, '\n');
    
    // if last character is CR then remove it
    if (!t.empty() && t.back() == '\r')
        t.pop_back();
    
    return is;
}

// Returns XP's system directory, including a trailing slash
const std::string& GetXPSystemPath ()
{
    // Fetch XP's system dir once
    static std::string sysDir;
    if (sysDir.empty()) {
        char s[512];
        XPLMGetSystemPath(s);
        sysDir = s;
    }
    return sysDir;
}

// If a path starts with X-Plane's system directory it is stripped
std::string StripXPSysDir (const std::string& path)
{
    // does the path begin with it?
    if (path.find(GetXPSystemPath()) == 0)
        return path.substr(GetXPSystemPath().length());
    else
        return path;
}

// Removes everything after the last dot, the dot including
void RemoveExtension (std::string& path)
{
    std::string::size_type dotPos = path.rfind('.');
    if (dotPos != std::string::npos)
        path.erase(dotPos);
}

//
// MARK: HFS-to-Posix path conversion (Mac only)
//

#if APL

#include <Carbon/Carbon.h>

// Funnily, even Apple deprecated HFS style...we need to ignore that warning here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/// Convert a path from HFS (colon separated) to Posix (slash separated)
bool HFS2PosixPath(const char *path, char *result, int resultLen);
/// Convert a path from Posix (slash separated) to HFS (colon separated)
bool Posix2HFSPath(const char *path, char *result, int resultLen);

/// A simple smart pointer structure, which makes sure to correctly release a CF pointer
template <typename T>
struct CFSmartPtr {
    CFSmartPtr(T p) : p_(p) {                          }
    ~CFSmartPtr()             { if (p_) CFRelease(p_); }
    operator T ()             { return p_; }
    T p_;
};


bool HFS2PosixPath(const char *path, char *result, int resultSize)
{
    bool is_dir = (path[strlen(path)-1] == ':');

    CFSmartPtr<CFStringRef>        inStr(CFStringCreateWithCString(kCFAllocatorDefault, path ,kCFStringEncodingMacRoman));
    if (inStr == NULL) return false;
    
    CFSmartPtr<CFURLRef>        url(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0));
    if (url == NULL) return false;
    
    CFSmartPtr<CFStringRef>        outStr(CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle));
    if (outStr == NULL) return false;
    
    if (!CFStringGetCString(outStr, result, resultSize, kCFStringEncodingMacRoman))
        return false;

    if(is_dir) strcat(result, "/");

    return true;
}

bool Posix2HFSPath(const char *path, char *result, int resultSize)
{
    CFSmartPtr<CFStringRef>        inStr(CFStringCreateWithCString(kCFAllocatorDefault, path ,kCFStringEncodingMacRoman));
    if (inStr == NULL) return false;
    
    CFSmartPtr<CFURLRef>        url(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle,0));
    if (url == NULL) return false;
    
    CFSmartPtr<CFStringRef>        outStr(CFURLCopyFileSystemPath(url, kCFURLHFSPathStyle));
    if (outStr == NULL) return false;
    
    if (!CFStringGetCString(outStr, result, resultSize, kCFStringEncodingMacRoman))
        return false;

    return true;
}

// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path from HFS to POSIX
std::string TOPOSIX (const std::string& p)
{
    // no actual conversion if XPLM_USE_NATIVE_PATHS is activated
    if (XPLMIsFeatureEnabled("XPLM_USE_NATIVE_PATHS"))
        return p;
    else {
        char posix[1024];
        if (HFS2PosixPath(p.c_str(), posix, sizeof(posix)))
            return posix;
        else
            return p;
    }
}

// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path from POSIX to HFS
std::string FROMPOSIX (const std::string& p)
{
    // no actual conversion if XPLM_USE_NATIVE_PATHS is activated
    if (XPLMIsFeatureEnabled("XPLM_USE_NATIVE_PATHS"))
        return p;
    else {
        char hfs[1024];
        if (Posix2HFSPath(p.c_str(), hfs, sizeof(hfs)))
            return hfs;
        else
            return p;
    }
}

#pragma clang diagnostic pop

#endif

//
// MARK: String helpers
//

// change a string to lowercase
std::string& str_tolower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) -> unsigned char { return (unsigned char) tolower(c); });
    return s;
}

// separates string into tokens
std::vector<std::string> str_tokenize (const std::string s,
                                       const std::string tokens,
                                       bool bSkipEmpty)
{
    std::vector<std::string> v;
 
    // find all tokens before the last
    size_t b = 0;                                   // begin
    for (size_t e = s.find_first_of(tokens);        // end
         e != std::string::npos;
         b = e+1, e = s.find_first_of(tokens, b))
    {
        if (!bSkipEmpty || e != b)
            v.emplace_back(s.substr(b, e-b));
    }
    
    // add the last one: the remainder of the string (could be empty!)
    v.emplace_back(s.substr(b));
    
    return v;
}




// Produces a very simple hash, which is the same if the same string is provided, across platform and across executions (unlike std::hash)
std::uint16_t PJWHash16(const char *pc)
{
    std::uint16_t h = 0, high;
    const unsigned char* s = reinterpret_cast<const unsigned char*>(pc);
    while (*s)
    {
        h <<= 2;                    // shift by 2 bits only, not 4
        h += *s++;
        high = h & 0xC000;          // pick the upper 2 bits only, not 4
        if (high)
            h ^= high >> 14;
        h &= ~high;
    }
    return h;
}

//
// MARK: Math helpers
//

// (Shortest) difference between 2 angles: How much to turn to go from h1 to h2?
float headDiff (float head1, float head2)
{
    // if either value is nan return nan
    if (std::isnan(head1) || std::isnan(head2)) return NAN;
    
    // if 0° North lies between head1 and head2 then simple
    // diff doesn't work
    if ( std::abs(head2-head1) > 180 ) {
        // add 360° to the lesser value...then diff works
        if ( head1 < head2 )
            head1 += 360;
        else
            head2 += 360;
    }
    
    return head2 - head1;
}


// Convert heading/pitch to x/y/z unit vector
std::valarray<float> HeadPitch2Vec (const float head, const float pitch)
{
    // Subtracting 90 degress is because x coordinate is heading east,
    // so 90 degrees is equivalent to x = 0
    const float radHead = deg2rad(head - 90.0f);
    const float radPitch = deg2rad(pitch);
    // Have sinus/cosinus pre-computed and let the optimizer deal with reducing local variables
    const float sinHead = std::sin(radHead);
    const float cosHead = std::cos(radHead);
    const float sinPitch = std::sin(radPitch);
    const float cosPitch = std::cos(radPitch);
    return std::valarray ({
        cosHead * cosPitch,         // x
        sinPitch,                   // y
        sinHead * cosPitch          // z
    });
}


// Convert heading/pitch/roll to unit normal vector
/// @see https://math.stackexchange.com/a/1637853, however with different order of values (x = 2nd row, y = 3rd row, z = 1st row of vectors) as X-Plane is using a different coordinate system
std::valarray<float> HeadPitchRoll2Normal(const float head, const float pitch, const float roll)
{
    // Subtracting 90 degress is because x coordinate is heading east,
    // so 90 degrees is equivalent to x = 0
    const float radHead = deg2rad(head - 90.0f);
    const float radPitch = deg2rad(pitch);
    const float radRoll  = deg2rad(roll);
    // Have sinus/cosinus pre-computed and let the optimizer deal with reducing local variables
    const float sinHead = std::sin(radHead);
    const float cosHead = std::cos(radHead);
    const float sinPitch = std::sin(radPitch);
    const float cosPitch = std::cos(radPitch);
    const float sinRoll = std::sin(radRoll);
    const float cosRoll = std::cos(radRoll);
    return std::valarray({
        // directional vector, same as HeadPitch2Vec
        cosHead* cosPitch,                                  // x
        sinPitch,                                           // y
        sinHead* cosPitch,                                  // z
        // normal vector
         sinRoll * sinHead - cosRoll * sinPitch * cosHead,  // x
         cosRoll * cosPitch,                                // y
        -sinRoll * cosHead - cosRoll * sinPitch * sinHead   // z
        });
}



//
// MARK: Misc
//

// Update cached values during a flight loop callback in XP's main thread to have them when called from a non-main thread
float UpdateCachedValuesGetNetwTime ()
{
    // Flush msg buffer
    FlushMsgs();
    // Cache and return current network time
    return GetMiscNetwTime();
}

// Get total running time from X-Plane (sim/time/total_running_time_sec)
float GetMiscNetwTime()
{
    static XPLMDataRef drMiscNetwTime = nullptr;
    static float fCacheVal = NAN;
    // If running in main thread we can ask X-Plane
    if (glob.IsXPThread()) {
        if (!drMiscNetwTime)
            drMiscNetwTime = XPLMFindDataRef("sim/network/misc/network_time_sec");
        return fCacheVal = XPLMGetDataf(drMiscNetwTime);
    }
    // Otherwise we revert to a cached value
    return fCacheVal;
}

// Return the network time as a string like used in the XP's Log.txt
std::string GetMiscNetwTimeStr (float _time)
{
    char aszTimeStr[20];
    if (std::isnan(_time))
        _time = GetMiscNetwTime();
    
    const unsigned runH = unsigned(_time / 3600.0f);
    _time -= runH * 3600.0f;
    const unsigned runM = unsigned(_time / 60.0f);
    _time -= runM * 60.0f;
    
    snprintf(aszTimeStr, sizeof(aszTimeStr), "%u:%02u:%06.3f",
             runH, runM, _time);
    return aszTimeStr;
}

// Text string for current graphics driver in use
const char* GetGraphicsDriverTxt ()
{
    if (glob.UsingModernGraphicsDriver())
#if APL
        return "Metal";
#else
        return "Vulkan";
#endif
    else
        return "OpenGL";
}

// X-Plane in a Pause state?
bool IsPaused()
{
    static XPLMDataRef drPause = XPLMFindDataRef("sim/time/paused");
    return XPLMGetDatai(drPause) != 0;
}

// Is current X-Plane view an external view (outside a cockpit)?
bool IsViewExternal()
{
    static XPLMDataRef drExternalView = XPLMFindDataRef("sim/graphics/view/view_is_external");
    return XPLMGetDatai(drExternalView) != 0;
}

// Convenience function to check on something at most every x seconds
bool CheckEverySoOften (float& _lastCheck, float _interval, float _now)
{
    if (_lastCheck < 0.00001f ||
        _now >= _lastCheck + _interval) {
        _lastCheck = _now;
        return true;
    }
    return false;
}

//
// MARK: LiveTraffic Exception classes
//

// standard constructor
XPMP2Error::XPMP2Error (const char* _szFile, int _ln, const char* _szFunc,
                        const char* _szMsg, ...) :
std::logic_error(LogGetString(_szFile, _ln, _szFunc, logFATAL, _szMsg, NULL)),
fileName(_szFile), ln(_ln), funcName(_szFunc)
{
    va_list args;
    va_start (args, _szMsg);
    msg = LogGetString(_szFile, _ln, _szFunc, logFATAL, _szMsg, args);
    va_end (args);
    
    // write to log
    if (logFATAL >= glob.logLvl)
        PushMsg ( msg.c_str() );
}

const char* XPMP2Error::what() const noexcept
{
    return msg.c_str();
}

//
//MARK: Log
//

/// Logging level text
const char* LOG_LEVEL[] = {
    "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL", "MSG  "
};

/// The temporary store for messages coming from worker threads
static std::queue<std::string> qMsgs;

/// Lock to control access to qMsgs
static std::mutex mtxMsgs;
/// atomic flag for faster test if there are any msgs to be flushed
static std::atomic_flag fMsgsEmpty;

// returns ptr to static buffer filled with log string
const char* LogGetString (const char* szPath, int ln, const char* szFunc,
                          logLevelTy lvl, const char* szMsg, va_list args )
{
     static char aszMsg[2048];

    // prepare timestamp
    if ( lvl < logMSG )                             // normal messages without, all other with location info
    {
        const char* szFile = strrchr(szPath, PATH_DELIM_STD);  // extract file from path
        if ( !szFile ) szFile = szPath; else szFile++;
        snprintf(aszMsg, sizeof(aszMsg), "%s %s/XPMP2 %s %s:%d/%s: ",
                 GetMiscNetwTimeStr().c_str(),      // Running time stamp
                 glob.logAcronym.c_str(), LOG_LEVEL[lvl],
                 szFile, ln, szFunc);
    }
    else
        snprintf(aszMsg, sizeof(aszMsg), "%s %s/XPMP2: ",
                 GetMiscNetwTimeStr().c_str(),          // Running time stamp
                 glob.logAcronym.c_str());
    
    // append given message
    if (args) {
        vsnprintf(&aszMsg[strlen(aszMsg)],
                  sizeof(aszMsg)-strlen(aszMsg)-1,      // we save one char for the CR
                  szMsg,
                  args);
    }

    // ensure there's a trailing CR
    size_t l = strlen(aszMsg);
    if ( aszMsg[l-1] != '\n' )
    {
        aszMsg[l]   = '\n';
        aszMsg[l+1] = 0;
    }

    // return the static buffer
    return aszMsg;
}

// Log Text to log file
void LogMsg ( const char* szPath, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, ... )
{
    va_list args;
    
    va_start (args, szMsg);
    PushMsg ( LogGetString(szPath, ln, szFunc, lvl, szMsg, args) );
    va_end (args);
}

// Store a message for immediate (XP thread) or later output (worker thread)
void PushMsg (const std::string& sMsg)
{
    // If we are in XP's main thread we can just write...but only after waiting msgs
    if (glob.IsXPThread()) {
        FlushMsgs();
        XPLMDebugString(sMsg.c_str());
    }
    // In a worker thread we have to add the message to the queue
    else {
        std::lock_guard<std::mutex> lock(mtxMsgs);  // access to queue guarded by lock
        qMsgs.push(sMsg);                           // add msg to queue
        fMsgsEmpty.clear();                         // clear the atomic flag...the queue is no longer empty
    }
}

// Flush all pending messages to Log.txt
void FlushMsgs ()
{
    // only if the flag says there's something waiting will we do the more expensive lock operation
    if (!fMsgsEmpty.test_and_set())
    {
        LOG_ASSERT(glob.IsXPThread());
        std::lock_guard<std::mutex> lock(mtxMsgs);  // access to queue guarded by lock
        while (!qMsgs.empty()) {                    // flush all waiting queue entries out to Log.txt
            XPLMDebugString(qMsgs.front().c_str());
            qMsgs.pop();
        }
    }
}

}       // namespace XPMP2

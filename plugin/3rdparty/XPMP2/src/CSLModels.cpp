/// @file       CSLModels.cpp
/// @brief      Managing CSL Models, including matching and loading
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

#define WARN_ASYNC_LOAD_ONGOING "Async load operation ongoing while object is being destroyed for %s"
#define WARN_REF_CNT            "Reference counter is %u while object is being destroyed for %s"
#define WARN_REF_CNT_ZERO       "Reference counter already 0, can't be decreaed, for %s"
#define DEBUG_OBJ_LOADING       "Async load starting  for %s from %s"
#define DEBUG_OBJ_LOADED        "Async load succeeded for %s from %s"
#define DEBUG_OBJ_UNLOADED      "Object %s / %s unloaded"
#define ERR_OBJ_OBJ_NOT_FOUND   "Async load for %s: CSLObj in CSLModel object not found!"
#define ERR_OBJ_NOT_FOUND       "Async load for %s: CSLModel object not found!"
#define ERR_OBJ_NOT_LOADED      "Async load FAILED for %s from %s"

/// The file holding package information
#define XSB_AIRCRAFT_TXT        "xsb_aircraft.txt"
#define DEBUG_XSBACTXT_READ     "Processing %s"
#define INFO_XSBACTXT_DONE      "Read %3d aircraft %s from %s"
#define WARN_XSBACTXT_IGNORED   "Ignored %d aircraft %s due to outdated format (OBJECT or AIRCRAFT) from %s"
#define INFO_TOTAL_NUM_MODELS   "Total number of known models now is %lu"
#define WARN_NO_XSBACTXT_FOUND  "No xsb_aircraft.txt found"
#define WARN_DUP_PKG_NAME       "Package name (EXPORT_NAME) '%s' in folder '%s' is already in use by '%s'"
#define WARN_DUP_MODEL          "Duplicate model '%s', additional definitions ignored, originally defined in line %d of %s"
#define WARN_OBJ8_ONLY          "Line %d: Only supported format is OBJ8, ignoring model definition %s"
#define WARN_PKG_DPDCY_FAILED   "Line %d: Dependent package '%s' not known! May lead to follow-up issues when proessing this package."
#define WARN_DIFF_TYPE          "Line %d: Different aircraft type (%s instead of %s) for model '%s' ignored, rest  processed"
#define ERR_TOO_FEW_PARAM       "Line %d: %s expects at least %d parameters"
#define ERR_PKG_NAME_INVALID    "Line %d: Package path invalid: %s"
#define ERR_PKG_UNKNOWN         "Line %d: Package '%s' unknown in package path %s"
#define ERR_OBJ_FILE_NOT_FOUND  "Line %d: The file '%s' could not be found at %s"
#define ERR_COULD_NOT_OPEN      "Could not open '%s' for reading!"
#define WARN_IGNORED_COMMANDS   "Following commands ignored: "
#define WARN_OBJ8_ONLY_VERTOFS  "Version is '%s', unsupported for reading vertical offset, file %s"

#define ERR_MATCH_NO_MODELS     "MATCH ABORTED - There is not any single CSL model available!"
#define DEBUG_MATCH_INPUT       "MATCH INPUT: Type=%s (WTC=%s,Class=%s,Related=%d), Airline=%s (relOp=%d), Livery=%s"
#define DEBUG_MATCH_FOUND       "MATCH FOUND: Type=%s (WTC=%s,Class=%s,Related=%d), Airline=%s (relOp=%d), Livery=%s / Quality = %d -> %s"
#define DEBUG_MATCH_NOTFOUND    "MATCH ERROR: Using a RANDOM model: %s %s %s - model %s"

/// The ids of our garbage collection flight loop callback
XPLMFlightLoopID gGarbageCollectionID = nullptr;
/// How often to call the garbage collection [s]
constexpr float GARBAGE_COLLECTION_PERIOD = 60.0f;
/// Unload an unused object after how many seconds?
constexpr float GARBAGE_COLLECTION_TIMEOUT = 180.0f;

/// a map of a text and a counter
typedef std::map<std::string, int> mapStrIntTy;

//
// MARK: CSL Model Info implementation
//       A small public structure to pass back CSL model information to the calling plugin
//

CSLModelInfo_t::CSLModelInfo_t(const CSLModel& csl) :
    cslId(csl.cslId), modelName(csl.modelName),
    xsbAircraftPath(csl.xsbAircraftPath), xsbAircraftLn(csl.xsbAircraftLn),
    icaoType(csl.GetIcaoType()),
    doc8643Classification(csl.GetDoc8643().classification),
    doc8643WTC(csl.GetDoc8643().wtc)
{
    // copy all match criteria
    for (const CSLModel::MatchCritTy& crit : csl.vecMatchCrit)
        vecMatchCrit.emplace_back(CSLModelInfo_t::MatchCrit_t{crit.getAirline(), crit.livery});
}

//
// MARK: CSL Object Implementation
//

// Destructor makes sure the XP object is removed, too
CSLObj::~CSLObj ()
{
    Unload();
}

// Load and return the underlying X-Plane object.
XPLMObjectRef CSLObj::GetAndLoadObj ()
{
    // (Try) Loading, then return what we have, even if it is NULL
    Load();
    return xpObj;
}

// Determine which file to load and if we need a copied .obj file
/// @details 1. Determine if we need to access a copied file at all
///          2. Avoid double-execution by testing if the file name is one for a copied file already
///          3. Compute that copied file name
///             It will include one of the texture ids if texture replacement is involved
///             It will always end on ".xpmp2.obj"
///          4. Test if that copied file already exists
///          The actual copy operation will only be peformed upon load.
void CSLObj::DetermineWhichObjToLoad ()
{
    // 1. Determine if we need to access a copied file at all
    const bool bDoReplTextures = glob.bObjReplTextures && (!texture.empty() || !text_lit.empty());
    if (!glob.bObjReplDataRefs && !bDoReplTextures)
        return;
    
    // 2. Test if this determination has previously already led to a copied file name
    if (path.find(".xpmp2.obj") != std::string::npos)
        return;
    
    // 3. Compute that copied file name
    pathOrig = path;                // Save the original name
    // remove the current extension
    RemoveExtension(path);
    // if we need to replace texture then a texture id should become part of the file name
    if (bDoReplTextures) {
        std::string addTxt = texture.empty() ? text_lit : texture;
        RemoveExtension(addTxt);
        path += '.';
        path += addTxt;
    }
    // always add 'xpmp2.obj' as the final extension
    path += ".xpmp2.obj";
    
    // 4. Test if that copied file already exists
    if (ExistsFile(path))
        // It does exist, so no new copy is needed
        pathOrig.clear();
}

// Read the obj file to calculate its vertical offset
/// @details The idea behind doing this is taken from the original libxplanemp
///          implementation, particularly
///          `XPMPMultiplayerCSLOffset.cpp: CslModelVertOffsetCalculator::findOffsetInObj8`:
///          The vertices and lines defined in the file have coordinates,
///          their outer edges define how much the aircraft would need to be
///          lifted to make its lowest part (typically the gear) appear
///          on the surfaces. So it searches for min/max values of
///          vertical (y) coordinates.
/// @note I am not sure why the original code returns `-max` if `min > 0`,
///       my understanding would be to always return `-min` and don't need `max`.
///       But I just hope that the original author knows better and I stick to it.
float CSLObj::FetchVertOfsFromObjFile () const
{
    float min = 0.0f, max = 0.0f;
    
    // Which file to read? Use pathOrig if defined because it could be that path doesn't exist yet
    const std::string& _path = pathOrig.empty() ? path : pathOrig;
    
    // Try opening our `.obj` file...that should actually work,
    // CSLObj only exists if the `.obj` file exists,
    // so we deal with errors but don't issue a lot of warnings here
    int lnNr = 0;
    std::ifstream fIn (TOPOSIX(_path));
    if (!fIn.good()) {
        LOG_MSG(logERR, ERR_COULD_NOT_OPEN, StripXPSysDir(_path).c_str());
        return 0.0f;
    }
    while (fIn)
    {
        std::string ln;
        safeGetline(fIn, ln);
        lnNr++;
        
        // line number 2 must define the version
        if (lnNr == 2) {
            // we can only read OBJ8 files
            if (std::stol(ln) < 800) {
                LOG_MSG(logWARN, WARN_OBJ8_ONLY_VERTOFS,
                        ln.c_str(), StripXPSysDir(_path).c_str());
                return 0.0f;
            }
        }
        
        // We try to decide really fast...obj files can be long
        // We are looking for "VT" and "VLINE", each will have at least 20 chars
        // and the first one must be 'V', the 2nd one either T or L
        if (ln.size() < 20 || ln[0] != 'V' ||
            (ln[1] != 'T' && ln[1] != 'L'))
            continue;
        
        // Chances are good we process it, so let's break it up into tokens
        std::vector<std::string> tokens = str_tokenize(ln, " \t");
        if (tokens.size() < 7 ||            // VT/VLINE must have 7 elements
            (tokens[0] != "VT" && tokens[0] != "VLINE"))
            continue;
        
        // Get and process the Y coordinate
        const float y = std::stof(tokens[2]);
        if (y < min)
            min = y;
        else if (y > max)
            max = y;
    }
    
    // return the proper VERT_OFFSET based on the Y coordinates we have read
    const float vertOfs = min < 0.0f ? -min : -max;
    LOG_MSG(logDEBUG, "Fetched VERT_OFFSET=%.1f from %s", vertOfs, StripXPSysDir(_path).c_str());
    return vertOfs;
}

// Starts loading the XP object asynchronously
/// @details    Async load takes time. Maybe this object exists no longer when the
///             load operaton returns. That's why we do not just pass `this`
///             to the XPLMLoadObjectAsync() call,
///             but a separate object, here a pair of copies of the id strings, by which we can try to
///             find the object again in the callback XPObjLoadedCB().
void CSLObj::Load ()
{
    // only if not already requested or even available
    if (GetObjState() != OLS_UNAVAIL &&
        GetObjState() != OLS_COPYING)
        return;
    
    // If needed it is now the time to copy the .obj file
    DetermineWhichObjToLoad();
    if (!TriggerCopyAndReplace())
        return;
    
    // Prepare to load the CSL model from the .obj file
    LOG_MSG(logDEBUG, DEBUG_OBJ_LOADING,
            cslKey.c_str(), StripXPSysDir(path).c_str());
    
    // Based on experience it seems XPLMLoadObjectAsync() does not
    // properly support HFS file paths. It just replaces all ':' with '/',
    // which is incomplete.
    // That's why we store paths to .obj already in POSIX format:
    XPLMLoadObjectAsync(path.c_str(),                   // path to .obj
                        &XPObjLoadedCB,                 // static callback function
                        new pairOfStrTy(cslKey.c_str(), // _copy_ of the key string
                                        path.c_str()));
    xpObjState = OLS_LOADING;
}

// Free up the object
void CSLObj::Unload ()
{
    if (xpObj) {
        XPLMUnloadObject(xpObj);
        xpObj = NULL;
        xpObjState = OLS_UNAVAIL;
        LOG_MSG(logDEBUG, DEBUG_OBJ_UNLOADED, cslKey.c_str(), StripXPSysDir(path).c_str());
    }
}

// Static: callback function called when loading is done
void CSLObj::XPObjLoadedCB (XPLMObjectRef inObject,
                            void *        inRefcon)
{
    // This is a plugin entry function, so we try to catch all exceptions
    try {
        // the refcon is a pointer to a pair object created just for us
        pairOfStrTy* p = reinterpret_cast<pairOfStrTy*>(inRefcon);
    
        // try finding the CSL model object in the global map
        mapCSLModelTy::iterator cslIter;
        CSLModel* pCsl = CSLModelByKey(p->first, &cslIter);
        if (!pCsl) {
            // CSL model not found in global map -> release the X-Plane object right away
            LOG_MSG(logERR, ERR_OBJ_NOT_FOUND, p->first.c_str());
            if (inObject)
                XPLMUnloadObject(inObject);
        }
        else
        {
            // find the object by path
            listCSLObjTy::iterator iter = std::find_if(pCsl->listObj.begin(),
                                                       pCsl->listObj.end(),
                                                       [p](const CSLObj& o)
                                                       { return o.path == p->second; });
        
            // found?
            if (iter != pCsl->listObj.end()) {
                // Loading succeeded -> save the object and state
                LOG_ASSERT(iter->GetObjState() == OLS_LOADING);
                if (inObject) {
                    iter->xpObj      = inObject;
                    iter->xpObjState = OLS_AVAILABLE;
                    LOG_MSG(logDEBUG, DEBUG_OBJ_LOADED,
                            iter->cslKey.c_str(), StripXPSysDir(iter->path).c_str());
                }
                // Loading of CSL object failed! -> remove the entire CSL model
                // so we don't try again and don't use it in matching
                else {
                    iter->Invalidate();
                    glob.mapCSLModels.erase(cslIter);
                }
            } else {
                LOG_MSG(logERR, ERR_OBJ_OBJ_NOT_FOUND, p->first.c_str());
                XPLMUnloadObject(inObject);
            }
        }
    
        // free up the memory for the copy of the ids
        delete p;
    }
    catch (const std::exception& e) { LOG_MSG(logFATAL, ERR_EXCEPTION, e.what()); }
    catch (...) { LOG_MSG(logFATAL, ERR_EXCEPTION, "<unknown>"); }
}

// Marks the CSL Model invalid in case object loading once failed
void CSLObj::Invalidate ()
{
    xpObj       = NULL;
    xpObjState  = OLS_INVALID;
    LOG_MSG(logERR, ERR_OBJ_NOT_LOADED, cslKey.c_str(), StripXPSysDir(path).c_str());
}

//
// MARK: CSLModel Implementation
//

// Am I more specific in terms of matching than o?
bool CSLModel::MatchCritTy::merge (const MatchCritTy& o)
{
    // We cannot merge if both sides have defined a value and these values are different
    if (!icaoAirline.empty() && !o.icaoAirline.empty() &&
        icaoAirline != o.icaoAirline)
        return false;
    if (!livery.empty() && !o.livery.empty() &&
        livery != o.livery)
        return false;
    
    // Otherwise, we take over the respective non-empty values
    if (icaoAirline.empty()) {
        icaoAirline = o.icaoAirline;
        relOp       = o.relOp;
    }
    if (livery.empty())
        livery = o.livery;
    return true;
}

/// compiles the string used as key in the CSL model map
std::string CSLModelGetKeyStr (int _related,
                               const std::string& _type,
                               const std::string& _id)
{
    char buf[255];
    snprintf(buf, sizeof(buf), "%04d|%s|%s",
             _related,
             _type.c_str(),
             _id.c_str());
    
    // remove trailing pipe symbols, need for proper lower-bound search
    while (buf[strlen(buf)-1] == '|')
        buf[strlen(buf)-1] = '\0';
    
    return buf;
}

// Destructor frees resources
CSLModel::~CSLModel ()
{
    // All aircrafts that still use me need a new model!
    // (Note: During shutdown, wenn all destructors are called,
    //        all aircraft should already be gone anyway and, hence,
    //        this loop is actually empty. This is only a safeguard
    //        during operations, e.g. due to invalidation after
    //        a failed object load, which is a rare case.)
    // Note: This assumes, that the model to-be-deleted is already
    //       technically removed from the map of models.
    for (auto& p: glob.mapAc)
        if (p.second->GetModel() == this &&
            p.second->IsValid())
            p.second->ReMatchModel();
    
    // last chance to unload the objects
    Unload();
    
    // Output a warning if there is still an async load operation going on
    if (GetObjState() == OLS_COPYING ||
        GetObjState() == OLS_LOADING) {
        LOG_MSG(logWARN, WARN_ASYNC_LOAD_ONGOING, cslId.c_str());
    }
    // Output a warning if reference counter is not zero
    if (refCnt > 0) {
        LOG_MSG(logWARN, WARN_REF_CNT, refCnt, cslId.c_str());
    }
}

// Set the a/c type model, which also fills `doc8643` and `related`, and add other match criteria
void CSLModel::AddMatchCriteria (const std::string& _type,
                                 const MatchCritTy& _matchCrit,
                                 int lnNr)
{
    // We cannot overwrite with a _different_ a/c ICAO type!
    if (!icaoType.empty() &&            // already defined
        icaoType != _type)              // but wanted something different now?
    {
        LOG_MSG(logWARN, WARN_DIFF_TYPE, lnNr,
                _type.c_str(), icaoType.c_str(), 
                modelName.c_str());
    }
    // set the ICAO aircraft type once and forever
    else if (icaoType.empty()) {
        icaoType = _type;
        doc8643 = & Doc8643Get(_type);
        related = RelatedGet(REL_TXT_DESIGNATOR, _type);
    }
    
    // See if we need to add the other match criterion
    // We try to keep as few and as specific match criteria as possible
    for (MatchCritTy& mc: vecMatchCrit)
        // can we merge the new with existing criteria?
        if (mc.merge(_matchCrit))
            return;

    // not merged: then add
    vecMatchCrit.push_back(_matchCrit);
}

// Puts together the model name string from a path component and the model's id
void CSLModel::CompModelName ()
{
    // Find the last component of the path
    size_t sep = xsbAircraftPath.find_last_of("\\/:");
    modelName = sep == std::string::npos ? xsbAircraftPath : xsbAircraftPath.substr(sep+1);
    // Add the id to it
    modelName += ' ';
    modelName += shortId;
}

// compiles the string used as key in the CSL model map
std::string CSLModel::GetKeyString () const
{
    return CSLModelGetKeyStr(related, icaoType, cslId);
}

// (Minimum) )State of the X-Plane objects: Is it being loaded or available?
ObjLoadStateTy CSLModel::GetObjState () const
{
    // must have some objects...
    if (listObj.empty()) return OLS_INVALID;
    
    // Otherwise find the smallest status
    return
    std::min_element(listObj.cbegin(), listObj.cend(),
                     [](const CSLObj& a, const CSLObj& b)->bool
                     { return a.GetObjState() < b.GetObjState();})
    ->GetObjState();
}

// Try get ALL object handles, only returns anything if it is the complete list
std::list<XPLMObjectRef> CSLModel::GetAllObjRefs ()
{
    // This loads the object, so we should make sure to have a VERT_OFFSET, too
    if (bVertOfsReadFromFile) {
        // span a job to read it from the object files
        bVertOfsReadFromFile = false;
        futVertOfs = std::async(std::launch::async,
                                &CSLModel::FetchVertOfsFromObjFile, this);
    }
    
    bool bFullyLoaded = true;
    std::list<XPLMObjectRef> l;
    for (CSLObj& obj: listObj)                  // loop over all object
    {
        if (obj.IsInvalid())                    // if any object is considered valid
            return std::list<XPLMObjectRef>();  // return an empty list
        
        l.push_back(obj.GetAndLoadObj());       // load and save the handle
        if (l.back() == NULL)                   // not (yet) loaded?
            bFullyLoaded = false;               // return an empty list in the end
    }
    
    // Also check if our vertical offset is available now
    if (futVertOfs.valid()) {                   // we are waiting for a result
        if (futVertOfs.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            bFullyLoaded = false;               // not yet available
        else
            vertOfs = futVertOfs.get();         // avaiable, get it
    }
    
    // return the complete list of handles if all was successful
    return bFullyLoaded ? l : std::list<XPLMObjectRef>();
}

// Decrease the reference counter for Aircraft usage
void CSLModel::DecRefCnt ()
 {
     if (refCnt>0) {
         if (--refCnt == 0) {
             // reached zero...remember when for later garbage collection
             refZeroTs = GetMiscNetwTime();
         }
     } else {
         LOG_MSG(logWARN, WARN_REF_CNT_ZERO, cslId.c_str());
     }
 }

// Static functions: Unload all objects which haven't been used for a while
float CSLModel::GarbageCollection (float, float, int, void*)
{
    const float now = GetMiscNetwTime();
    // loop all models
    for (auto& p: glob.mapCSLModels) {
        CSLModel& mdl = p.second;
        // loaded, but reference counter zero, and timeout reached
        if (mdl.GetObjState() == OLS_AVAILABLE &&
            mdl.GetRefCnt() == 0 &&
            now - mdl.refZeroTs > GARBAGE_COLLECTION_TIMEOUT)
            // unload the object
            mdl.Unload();
    }
    
    return GARBAGE_COLLECTION_PERIOD;
}

// Unload all objects
void CSLModel::Unload ()
{
    for (CSLObj& o: listObj)
        o.Unload();
}

// Read the obj files to fill CSLModel::vertOfs
/// @note Expected to be called in a separate thread via std::async
float CSLModel::FetchVertOfsFromObjFile () const
{
    // This is a thread main function, set thread's name and try to catch all exceptions
    SET_THREAD_NAME("XPMP2_VertOfs");

    float ret = 0.0f;
    try {
        for (const CSLObj& obj: listObj) {
            const float o = obj.FetchVertOfsFromObjFile();
            if (o > ret)
                ret = o;
        }
    }
    catch(const std::exception& e) {
        LOG_MSG(logERR, "Reading vertical offsets failed: %s", e.what());
    }
    catch (...) {
        LOG_MSG(logERR, "Reading vertical offsets failed with an exception");
    }
    return ret;
}

//
// MARK: Internal Functions
//

/// @brief Turn a relative package path into a full path
/// @details Turns a relative package path in `xsb_aircraft.txt` (like "__Bluebell_Airbus:A306/A306_AAW.obj")
///          into a full path pointing to a concrete file and verifies the file's existence.
/// @return Empty if any validation fails, otherwise a full path to an existing .obj file
std::string CSLModelsConvPackagePath (const std::string& pkgPath,
                                      int lnNr,
                                      bool bPkgOptional = false)
{
    // find the first element, which shall be the package
    std::string::size_type pos = pkgPath.find_first_of(":/\\");
    if (pos == std::string::npos ||             // not found???
        pos == 0 ||                             // or the very first char?
        pos == pkgPath.length()-1)              // or the very last char only?
    {
        if (bPkgOptional)
            // that is OK if the package is optional
            return pkgPath;
        else
        {
            LOG_MSG(logERR, ERR_PKG_NAME_INVALID, lnNr, StripXPSysDir(pkgPath).c_str());
            return "";
        }
    }
    
    // Let's try finding the full path for the package
    const std::string pkg(pkgPath.substr(0,pos));
    const auto pkgIter = glob.mapCSLPkgs.find(pkg);
    if (pkgIter == glob.mapCSLPkgs.cend()) {
        LOG_MSG(logERR, ERR_PKG_UNKNOWN, lnNr, pkg.c_str(), StripXPSysDir(pkgPath).c_str());
        return "";
    }
    
    // Found the package, so now let's make a proper path
    // The relative path to the file is
    std::string relFilePath = pkgPath.substr(pos+1);
    // All 'wrong' path separators need to be changed to the correct separator
    std::replace_if(relFilePath.begin(), relFilePath.end(),
                    [](char c)
                    {
                        if (c == XPLMGetDirectorySeparator()[0])        // don't touch correct directory separator
                            return false;
                        return c==':' || c=='\\' || c=='/';             // but do touch all others
                    },
                    XPLMGetDirectorySeparator()[0]);    // replace with 'correct' dir separator

    // Full path is package path plus relative path
    std::string path = pkgIter->second + relFilePath;
    
    // We do check here already if that target really exists
    if (!ExistsFile(TOPOSIX(path))) {
        LOG_MSG(logERR, ERR_OBJ_FILE_NOT_FOUND, lnNr,
                StripXPSysDir(pkgPath).c_str(), StripXPSysDir(path).c_str());
        return "";
    }
    
    // Success, return the converted path
    return path;
}


/// Adds a readily defined CSL model to all the necessary maps
void CSLModelsAdd (CSLModel& _csl)
{
    // the main map, which actually "owns" the object, indexed by key
    const std::string cslKey = _csl.GetKeyString();
    auto p = glob.mapCSLModels.emplace(cslKey, std::move(_csl));
    if (!p.second) {                    // not inserted, ie. not a new entry!
        LOG_MSG(logWARN, WARN_DUP_MODEL, p.first->second.GetModelName().c_str(),
                p.first->second.xsbAircraftLn,
                StripXPSysDir(p.first->second.xsbAircraftPath).c_str());
    } else {
        // Need to update the cslKey that is stored in the CSL objects as only now we have the final value
        for (CSLObj& obj : p.first->second.listObj)
            obj.cslKey = cslKey;
    }
}


/// Scans an `xsb_aircraft.txt` file for `EXPORT_NAME` entries to fill the list of packages
const char* CSLModelsReadPkgId (const std::string& path)
{
    // Open the xsb_aircraft.txt file
    const std::string xsbName (path + XPLMGetDirectorySeparator()[0] + XSB_AIRCRAFT_TXT);
    std::ifstream fAc (TOPOSIX(xsbName));
    if (!fAc || !fAc.is_open())
        return WARN_NO_XSBACTXT_FOUND;
    
    // read the file line by line
//    LOG_MSG(logINFO, INFO_XSBACTXT_READ, xsbName.c_str());
    while (fAc)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fAc, ln);
        trim(ln);
        
        // skip over empty lines
        if (ln.empty())
            continue;
        
        // We are only looking for EXPORT_NAME here
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        if (tokens.size() == 2 &&
            tokens[0] == "EXPORT_NAME")
        {
            // Found a package id -> save an entry for a package
            auto p = glob.mapCSLPkgs.insert(std::make_pair(tokens[1], path + XPLMGetDirectorySeparator()[0]));
            if (!p.second) {                // not inserted, ie. package name existed already?
                LOG_MSG(logWARN, WARN_DUP_PKG_NAME,
                        tokens[1].c_str(), StripXPSysDir(path).c_str(),
                        p.first->second.c_str());
            } else {
                LOG_MSG(logDEBUG, "Added package '%s' from %s",
                        tokens[1].c_str(), StripXPSysDir(path).c_str());
            }
        }
    }
    
    // Close the xsb_aircraft.txt file
    fAc.close();
    
    // Success
    return "";
}

/// @brief Recursively scans folders to find `xsb_aircraft.txt` files of CSL packages
/// @param _path The path to start the search in
/// @param[out] paths List of paths in which an xsb_aircraft.txt file has actually been found
/// @param _maxDepth How deep into the folder hierarchy shall we search? (defaults to 5)
const char* CSLModelsFindPkgs (const std::string& _path,
                               std::vector<std::string>& paths,
                               int _maxDepth = 5)
{
    // Search the current given path for an xsb_aircraft.txt file
    std::list<std::string> files = GetDirContents(_path);
    if (std::find(files.cbegin(), files.cend(), XSB_AIRCRAFT_TXT) != files.cend())
    {
        // Found a "xsb_aircraft.txt"! Let's process this path then!
        paths.push_back(_path);
        return CSLModelsReadPkgId(_path);
    }
    
    // Are we still allowed to dig deeper into the folder hierarchy?
    bool bFoundAnything = false;
    if (_maxDepth > 0) {
        // The let's see if there were some directories
        for (const std::string& f: files) {
            const std::string nextPath(_path + XPLMGetDirectorySeparator()[0] + f);
            if (IsDir(TOPOSIX(nextPath))) {
                // recuresively call myself, allow one level of hierarchy less
                const char* res = CSLModelsFindPkgs(nextPath, paths, _maxDepth-1);
                // Not the message "nothing found"?
                if (strcmp(res, WARN_NO_XSBACTXT_FOUND) != 0) {
                    // if any other error: stop here and return that error
                    if (res[0])
                        return res;
                    // ...else: We did find and process something!
                    bFoundAnything = true;
                }
            }
        }
    }
    
    // Well...did we find anything?
    return bFoundAnything ? "" : WARN_NO_XSBACTXT_FOUND;
}


//
// MARK: Interpret individual xsb_aircraft lines
//

/// Process an DEPENDENCY line of an `xsb_aircraft.txt` file
void AcTxtLine_DEPENDENCY (std::vector<std::string>& tokens,
                           int lnNr)
{
    if (tokens.size() >= 2) {
        // We try finding the package and issue a warning if we didn't...but continue anyway
        if (glob.mapCSLPkgs.find(tokens[1]) == glob.mapCSLPkgs.cend()) {
            LOG_MSG(logWARN, WARN_PKG_DPDCY_FAILED, lnNr, tokens[1].c_str());
        }
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
}

/// Process an OBJ8_AIRCRAFT line of an `xsb_aircraft.txt` file
void AcTxtLine_OBJ8_AIRCRAFT (CSLModel& csl,
                              const std::string& ln,
                              const std::string& xsbAircraftPath,
                              const std::string& exportName,
                              int lnNr)
{
    // First of all, save the previously read aircraft
    if (csl.IsValid())
        CSLModelsAdd(csl);
    // properly reset the passed-in CSLModel reference
    csl = CSLModel();
    
    // Properly set the xsb_aircraft.txt location
    csl.xsbAircraftPath = xsbAircraftPath;
    csl.xsbAircraftLn   = lnNr;
    
    // Second parameter (actually we take all the rest of the line) is the short id:
    if (ln.length() >= 15) {
        csl.shortId = ln.substr(14);
        trim(csl.shortId);
        
        // sometimes (e.g. X-CSL) the name already contains a package name, that's superflous, take the last part only as short id
        std::string::size_type sepPos = csl.shortId.find_last_of(":/\\");
        if (sepPos != std::string::npos)
            csl.shortId.erase(0,sepPos+1);
        
        // save a hash value for the package name, needed for synching with remote client
        csl.pkgHash = PJWHash16(exportName.c_str());
        // full id is package name (EXPORT) plus the short id
        csl.cslId = exportName + '/' + csl.shortId;
        // human readable model name is last part of path plus short id
        csl.CompModelName();
    }
    else LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, "OBJ8_AIRCRAFT", 1);
}

/// Process an OBJECT or AIRCRAFT  line of an `xsb_aircraft.txt` file (which are no longer supported)
void AcTxtLine_OBJECT_AIRCRAFT (CSLModel& csl,
                                const std::string& /*ln*/,
                                int /*lnNr*/)
{
    // First of all, save the previously read aircraft
    if (csl.IsValid())
        CSLModelsAdd(csl);
    // properly reset the passed-in CSLModel reference
    csl = CSLModel();

    // Then add a warning into the log as we will NOT support this model
    // Could be too many and clog up the log - LOG_MSG(logWARN, WARN_OBJ8_ONLY, lnNr, ln.c_str());
}

/// Process an OBJ8 line of an `xsb_aircraft.txt` file
/// We don't care what type of object it is (ignoring the 1st parameter)
void AcTxtLine_OBJ8 (CSLModel& csl,
                     std::vector<std::string>& tokens,
                     int lnNr)
{
    if (tokens.size() >= 4) {
        // translate the path (replace the package with the full path)
        std::string path = CSLModelsConvPackagePath(tokens[3], lnNr);
        if (!path.empty()) {
            // save the path as an additional object to the model
            // (Paths  to .obj are always stored in POSIX format)
            csl.listObj.emplace_back(csl.GetKeyString(), TOPOSIX(path));
            CSLObj& obj = csl.listObj.back();

            // we can already read the TEXTURE and TEXTURE_LIT paths
            if (tokens.size() >= 5) {
                obj.texture = CSLModelsConvPackagePath(tokens[4], lnNr, true);
                if (tokens.size() >= 6)
                    obj.text_lit = CSLModelsConvPackagePath(tokens[5], lnNr, true);
            } // TEXTURE available
        } // Package name valid
    } // at least 3 params
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 3);
}

/// Process an VERT_OFFSET line of an `xsb_aircraft.txt` file
void AcTxtLine_VERT_OFFSET (CSLModel& csl,
                            std::vector<std::string>& tokens,
                            int lnNr)
{
    if (tokens.size() >= 2) {
        csl.vertOfs = std::stof(tokens[1]);
        csl.bVertOfsReadFromFile = false;   // don't need to read OBJ file
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
}

/// Process an OFFSET line of a PilotEdge `xsb_aircraft.txt` file,
/// actually its 3rd parameter only (we still don't know what the first 2 are)
/// @see http://forums.pilotedge.net/viewtopic.php?f=12&t=7236#p47925
void AcTxtLine_OFFSET (CSLModel& csl,
                       std::vector<std::string>& tokens,
                       int lnNr)
{
    if (tokens.size() >= 4) {
        csl.vertOfs = std::stof(tokens[3]);
        csl.bVertOfsReadFromFile = false;   // don't need to read OBJ file
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 3);
}

/// Process an ICAO, AIRLINE, LIVERY, or MATCHES line of an `xsb_aircraft.txt` file
void AcTxtLine_MATCHES (CSLModel& csl,
                        std::vector<std::string>& tokens,
                        int lnNr)
{
    // at least one parameter, the ICAO type code, is expected
    if (tokens.size() >= 2) {
        // Add match criteria to the CSL model
        CSLModel::MatchCritTy mc;
        if (tokens.size() >= 3 && tokens[2] != "-")     // if given: airline
            mc.setAirline(tokens[2]);
        if (tokens.size() >= 4 && tokens[3] != "-")     // if given: livery
            mc.livery = tokens[3];
        // Set/Add all match criteria
        csl.AddMatchCriteria(tokens[1], mc, lnNr);
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
}


/// @brief Compiles a string from a map of string->int like "(A320 x 5, A388 x 12)"
/// @return Total count (17 in the example above)
int StrCntString (const mapStrIntTy& m, std::string& s)
{
    // do nothing if map is empty
    if (m.empty()) {
        s.clear();
        return 0;
    }
     
    // put together the string
    int n = 0;
    s.reserve(m.size() * 10);       // a reasonable guess about the string size, probably a bit large
    s = "(";
    char buf[100];
    for (const auto& p: m) {
        if (p.second > 1)
            snprintf(buf, sizeof(buf), "%d x %s, ",
                     p.second, p.first.c_str());
        else
            snprintf(buf, sizeof(buf), "%s, ",
                     p.first.c_str());
        s += buf;
        n += p.second;
    }
    s.pop_back();                   // remove the final ", "
    s.pop_back();
    s += ')';
    return n;
}

/// Process one `xsb_aircraft.txt` file for importing OBJ8 models
const char* CSLModelsProcessAcFile (const std::string& path)
{
    // for a good but concise message about ignored elements we keep this list
    std::map<std::string, int> ignoredCmd;
    // for a good but concise message about read a/c we keep this map, keyed by ICAO type designator
    std::map<std::string, int> acRead;
    // for a good but concise message about ignored OBJECTs and AIRCRAFT we keep this list
    std::map<std::string, int> ignoredObj;

    // The package's name
    std::string exportName = "?";
    
    // Here we collect the next CSL model
    CSLModel csl;
    
    // Open the xsb_aircraft.txt file
    const std::string xsbName (path + XPLMGetDirectorySeparator()[0] + XSB_AIRCRAFT_TXT);
    std::ifstream fAc (TOPOSIX(xsbName));
    if (!fAc || !fAc.is_open())
        return WARN_NO_XSBACTXT_FOUND;
    
    // read the file line by line
    LOG_MSG(logDEBUG, DEBUG_XSBACTXT_READ, StripXPSysDir(xsbName).c_str());
    for (int lnNr = 1; fAc; ++lnNr)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fAc, ln);
        trim(ln);
        
        // skip over empty lines or lines starting a comment
        if (ln.empty() || ln[0] == '#')
            continue;
        
        // Break up the line into individual parameters and work on its commands
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        if (tokens.empty()) continue;
        
        // DEPENDENCY: We warn if we don't find the package but try anyway
        if (tokens[0] == "DEPENDENCY")
            AcTxtLine_DEPENDENCY(tokens, lnNr);
        
        // EXPORT_NAME: Already processed, but needed for model id
        else if (tokens[0] == "EXPORT_NAME") {
            exportName = ln.substr(12);
            trim(exportName);
        }

        // OBJECT or AIRCRAFT aren't supported any longer, but as they are supposed start
        // another aircraft we need to make sure to save the one defined previously,
        // and we use the chance to issue some warnings into the log
        else if (tokens[0] == "OBJECT" || tokens[0] == "AIRCRAFT") {
            AcTxtLine_OBJECT_AIRCRAFT(csl, ln, lnNr);
            if (tokens.size() >= 2)
                ignoredObj[tokens[1]]++;
            else
                ignoredObj["<unknown>"]++;
        }

        // OBJ8_AIRCRAFT: Start a new aircraft specification
        else if (tokens[0] == "OBJ8_AIRCRAFT") {
            if (csl.IsValid()) acRead[csl.GetIcaoType()]++;
            AcTxtLine_OBJ8_AIRCRAFT(csl, ln, path, exportName, lnNr);
            continue;                   // don't run into the "ignored" counter later
        }
        
        // -- All following commands only make sense if a model has previously
        //    been started with OBJ8_AIRCRAFT, which sets the csl's id ---
        if (csl.GetId().empty())
            continue;                   // skip the line if no model defined
        
        // OBJ8: Define the object file to load
        if (tokens[0] == "OBJ8")
            AcTxtLine_OBJ8(csl, tokens, lnNr);

        // VERT_OFFSET: Defines the vertical offset of the model, so the wheels correctly touch the ground
        else if (tokens[0] == "VERT_OFFSET")
            AcTxtLine_VERT_OFFSET(csl, tokens, lnNr);
        
        // OFFSET: 3rd parameter defines the vertical offset of the model, so the wheels correctly touch the ground
        else if (tokens[0] == "OFFSET")
            AcTxtLine_OFFSET(csl, tokens, lnNr);
        
        // The matching parameters will be processed the same way...
        else if (tokens[0] == "ICAO" ||
                 tokens[0] == "AIRLINE" ||
                 tokens[0] == "LIVERY" ||
                 tokens[0] == "MATCHES")
            AcTxtLine_MATCHES(csl, tokens, lnNr);

        // else...we just ignore it but count the commands for a proper warning later
        else
            ignoredCmd[tokens[0]]++;
    }
    
    // Close the xsb_aircraft.txt file
    fAc.close();
    
    // Don't forget to also save the last object
    if (csl.IsValid()) {
        acRead[csl.GetIcaoType()]++;
        CSLModelsAdd(csl);
    }
    
    // Log a message about the a/c we've read
    if (!acRead.empty()) {
        std::string acList;
        int totAcRead = StrCntString(acRead, acList);
        LOG_MSG(logINFO, INFO_XSBACTXT_DONE, totAcRead, acList.c_str(),
                StripXPSysDir(xsbName).c_str());
    }
    
    // Log a message about the a/c we've ignored
    if (!ignoredObj.empty()) {
        std::string acList;
        int totAcRead = StrCntString(ignoredObj, acList);
        LOG_MSG(logWARN, WARN_XSBACTXT_IGNORED, totAcRead, acList.c_str(),
                StripXPSysDir(xsbName).c_str());
    }
    
    // If there were ignored commands we list them once now
    if (!ignoredCmd.empty() && glob.bLogMdlMatch) {
        std::string msg(WARN_IGNORED_COMMANDS);
        char buf[25];
        for (const auto& i: ignoredCmd) {
            snprintf(buf, sizeof(buf), "%s (%dx), ",
                     i.first.c_str(), i.second);
            msg += buf;
        }
        msg.pop_back();                 // remove the last trailing ", "
        msg.pop_back();
        LOG_MSG(logWARN, "%s", msg.c_str());
    }

    // Success
    return "";
}


//
// MARK: Global Functions
//


// Initialization
void CSLModelsInit ()
{
    // Create the flight loop callback (unscheduled) if not yet there
    if (!gGarbageCollectionID) {
        XPLMCreateFlightLoop_t cfl = {
            sizeof(XPLMCreateFlightLoop_t),                 // size
            xplm_FlightLoop_Phase_AfterFlightModel,         // phase
            CSLModel::GarbageCollection,                    // callback function
            nullptr                                         // refcon
        };
        gGarbageCollectionID = XPLMCreateFlightLoop(&cfl);
    }
    
    // Schedule the flight loop callback to be called next flight loop cycle
    XPLMScheduleFlightLoop(gGarbageCollectionID, GARBAGE_COLLECTION_PERIOD, 1);
}


// Grace cleanup
void CSLModelsCleanup ()
{
    // stop the garbage collection
    if (gGarbageCollectionID) {
        XPLMDestroyFlightLoop(gGarbageCollectionID);
        gGarbageCollectionID = nullptr;
    }
    
    // Clear out all model objects, will in turn unload all X-Plane objects
    glob.mapCSLModels.clear();
    // Clear out all packages
    glob.mapCSLPkgs.clear();
}


// Read the CSL Models found in the given path and below
const char* CSLModelsLoad (const std::string& _path,
                           int _maxDepth)
{
    // First we identify all package, so that (theoretically)
    // package dependencies to other packages can be resolved.
    // (This might rarely be used as OBJ8 only consists of one file,
    //  but the original xsb_aircraft.txt syntax requires it.)
    std::vector<std::string> paths;
    const char* res = CSLModelsFindPkgs(_path, paths, _maxDepth);
    
    // Now we can process each folder and read in the CSL models there
    for (const std::string& p: paths)
    {
        const char* r = CSLModelsProcessAcFile(p);
        if (r[0]) {                     // error?
            res = r;                    // keep it as function result (but continue with next path anyway)
            LOG_MSG(logWARN, "%s", res);// also report it to the log
        }
    }
    
    // How many models do we now have in total?
    LOG_MSG(logINFO, INFO_TOTAL_NUM_MODELS, (unsigned long)glob.mapCSLModels.size())
    
    // return the final result
    return res;
}


// Find a model by name
CSLModel* CSLModelById (const std::string& _cslId,
                        mapCSLModelTy::iterator* _pOutIter)
{
    // try finding the model by name
    mapCSLModelTy::iterator iter =
    std::find_if(glob.mapCSLModels.begin(),
                 glob.mapCSLModels.end(),
                 [_cslId](const mapCSLModelTy::value_type& csl)
                 { return csl.second.GetId() == _cslId; });
    
    // If requested, also return the iterator itself
    if (_pOutIter)
        *_pOutIter = iter;
    
    // not found, or invalid?
    if (iter == glob.mapCSLModels.end() || iter->second.IsObjInvalid())
        return nullptr;
    
    // Success
    return &iter->second;
}


// Find a model by key
CSLModel* CSLModelByKey(const std::string& _cslKey,
                        mapCSLModelTy::iterator* _pOutIter)
{
    // find the CSL model by it's map key
    mapCSLModelTy::iterator iter = glob.mapCSLModels.find(_cslKey);

    // If requested, also return the iterator itself
    if (_pOutIter)
        *_pOutIter = iter;

    // Return a pointer to the object
    if (iter == glob.mapCSLModels.end())
        return nullptr;
    else
        return &(iter->second);
}


// Find a model by package name hash and short id
/// @note This is directly used by XPMP2-Remote client with a potentially limited short id string
CSLModel* CSLModelByPkgShortId (std::uint16_t _pkgHash,
                                const std::string& _shortId)
{
    // try finding the model by shortId, also verify pckage hash
    mapCSLModelTy::iterator iBest = glob.mapCSLModels.end();
    for (mapCSLModelTy::iterator iter = glob.mapCSLModels.begin();
         iter != glob.mapCSLModels.end();
         ++iter)
    {
        // short id matches...as far as known
        if (std::strncmp(iter->second.GetShortId().c_str(), _shortId.c_str(), _shortId.length()) == 0) {
            if (iter->second.pkgHash == _pkgHash) {     // perfect match!
                iBest = iter;
                break;
            }
            else if (iBest == glob.mapCSLModels.end() &&// remember the first model for which at least the short id matches
                     !iter->second.IsObjInvalid())
                iBest = iter;                           // but keep on searching
        }
    }
    
    // not found, or invalid?
    if (iBest == glob.mapCSLModels.end() || iBest->second.IsObjInvalid())
        return nullptr;
    
    // Success
    return &iBest->second;
}

//
// MARK: Matching
//

/// Returns any random value in the range `[cslLower; cslUpper)`, or `upper` if there is no value in that range
template <class IteratorT>
IteratorT iterRnd (IteratorT lower, IteratorT upper)
{
    const long dist = (long)std::distance(lower, upper);
    // Does the range (cslUpper excluded!) not contain anything?
    if (dist <= 0)
        return upper;
    // Does the "range" only contain exactly one element? Then shortcut the search
    if (dist == 1)
        return lower;
    // Contains more than one, so make a random choice
    const float rnd = float(std::rand()) / float(RAND_MAX); // should be less than 1.0
    const long adv = long(rnd * dist);                      // therefor, should be less than `dist`
    if (adv < dist)                                         // but let's make sure
        std::advance(lower, long(adv));
    else
        lower = std::prev(upper);
    return lower;
}

/// @brief      Tries finding a match using both aircraft and Doc8643 attributes
/// @details    Each attribute is represented by a bit in a bit mask.
///             Lower priority attributes are represented by low value bits,
///             and vice versa high prio match criteria by high value bits.
///             The bit is 0 if the attribute matches and 1 if not.
///             The resulting numeric value of the bitmask is considered
///             the match quality: The lower the number the better the quality.
bool CSLFindMatch (const std::string& _type,
                   const std::string& _airline,
                   const std::string& _livery,
                   bool bIgnoreNoMatch,
                   int& quality,
                   CSLModel* &pModel)
{
    // How many parameters will we compare?
    constexpr unsigned DOC8643_MATCH_PARAMS = 12;
    constexpr unsigned DOC8643_MATCH_WORST_QUAL = 2 << DOC8643_MATCH_PARAMS;
    
    // if there aren't any models we won't find any either
    if (glob.mapCSLModels.empty()) {
        quality += DOC8643_MATCH_WORST_QUAL;
        return false;
    }

    // The Doc8643 definition for the wanted aircraft type
    const Doc8643& doc8643 = Doc8643Get(_type);
    const bool bDocEmpty = doc8643.empty();
    
    // The related group depends on the ICAO aircraft type and can be zero
    // (zero = not part of any related-group)
    const int related = RelatedGet(REL_TXT_DESIGNATOR, _type);
    const int relOp   = RelatedGet(REL_TXT_OP, _airline);
    
    LOG_MATCHING(logINFO, DEBUG_MATCH_INPUT,
                 _type.c_str(),
                 doc8643.wtc, doc8643.classification, related,
                 _airline.c_str(), relOp,
                 _livery.c_str());
    
    // A string copy makes comparisons easier later on
    const std::string wtc = doc8643.wtc;
    
    // We can do a full scan of the complete set of all models
    // and save models that match per pass.
    // The folloing multimap stores potential models, with matching pass as the key
    mmapCSLModelPTy mm;
    unsigned long bestMatchYet = DOC8643_MATCH_WORST_QUAL;

    // Which models to test?
    mapCSLModelTy::iterator mStart = glob.mapCSLModels.begin();
    mapCSLModelTy::iterator mEnd   = glob.mapCSLModels.end();

    // However, most matches are done with ICAO aircraft type given,
    // which implies a "related" group.
    // We can narrow down the set of models to scan if we find one with
    // matching "related" group.
    if (related > 0) {
        std::string key = CSLModelGetKeyStr(related, "", "");
        mStart = glob.mapCSLModels.lower_bound(key);
        key = CSLModelGetKeyStr(related, "~~~~", "~~~~");
        mEnd = glob.mapCSLModels.upper_bound(key);
        
        // No models found matching the related group?
        if (mStart == mEnd) {
            // well, then search all models
            mStart = glob.mapCSLModels.begin();
            mEnd   = glob.mapCSLModels.end();
        }
    }

    // Now scan all CSL models in the defined range
    for (mapCSLModelTy::iterator mIter = mStart;
         mIter != mEnd;
         ++mIter)
    {
        CSLModel& mdl = mIter->second;

        // Now we calculate match quality for each possible match criteria
        for (const CSLModel::MatchCritTy& mc: mdl.vecMatchCrit)
        {
            // Consider each of the following
            // comparisions to represent a bit in the final quality,
            // with lowest priority (livery) in the lowest bit and
            // highest (has rotor) in the highest  (most significant) bit.
            // Bit is zero if matches, non-zero if not => lowest number is best quality
            std::bitset<DOC8643_MATCH_PARAMS> matchQual;
            // Lower part matches on very detailed parameters
            matchQual.set(0, _livery.empty()    || mc.livery         != _livery);
            matchQual.set(1, _airline.empty()   || mc.getAirline()   != _airline);
            matchQual.set(2, relOp == 0         || mc.getRelOp()     != relOp);
            matchQual.set(3, _type.empty()      || mdl.GetIcaoType() != _type);
            matchQual.set(4,                    // this matches if airline _and_ related group match (so we value a matching livery in a "related" model higher than an exact model with improper livery)
                          _airline.empty() || related == 0 ||
                          mc.getAirline() != _airline || mdl.GetRelatedGrp()  != related);
            matchQual.set(5,                    // this matches if related operators group _and_ related group match (so we value a matching group livery in a "related" model higher than an exact model with improper livery)
                          relOp == 0       || related == 0 ||
                          mc.getRelOp()   != relOp    || mdl.GetRelatedGrp()  != related);
            matchQual.set(6, related == 0       || mdl.GetRelatedGrp()  != related);
            // Upper part matches on generic "size/type of aircraft" parameters,
            // which are expected to match anyway if the above (like group/ICAO type) match
            matchQual.set(7, bDocEmpty          || mdl.GetClassEngType()!= doc8643.GetClassEngType());
            matchQual.set(8, bDocEmpty          || mdl.GetClassNumEng() != doc8643.GetClassNumEng());
            matchQual.set(9, bDocEmpty          || mdl.GetWTC()         != wtc);
            matchQual.set(10,bDocEmpty          || mdl.GetClassType()   != doc8643.GetClassType());
            matchQual.set(11,bDocEmpty          || mdl.HasRotor()       != doc8643.HasRotor());
            
            // If we are to ignore the doc8643 matches (in case of no doc8643 found)
            // then we completely ignore models which don't match at all
            if (!bIgnoreNoMatch || !matchQual.all())
            {
                // Add the model into the map (if we do not already
                // have better-category matches: We don't need to store
                // quality 32 any longer if we know we already found a quality 1 match)
                if (matchQual.to_ulong() <= bestMatchYet) {
                    bestMatchYet = matchQual.to_ulong();
                    mm.emplace(bestMatchYet,
                               std::make_pair<CSLModel*,const CSLModel::MatchCritTy*>(&mdl,&mc));
                }
            }
        }
    }
    
    // Potentially, we didn't find anything if we were to ignore some matches
    if (bIgnoreNoMatch && mm.empty()) {
        quality += DOC8643_MATCH_WORST_QUAL;
        return false;
    }
    
    // So: We _must_ have found something
    LOG_ASSERT(bestMatchYet < DOC8643_MATCH_WORST_QUAL);
    quality += bestMatchYet;
    quality++;                  // ...because bestMatchYet is zero-based
    
    // Of those relevant (having the best possible match quality)
    // we return any more or less randomly chosen model out of that list of possible models
    auto pairIter = mm.equal_range(bestMatchYet);
    const auto& selected = iterRnd(pairIter.first, pairIter.second)->second;
    pModel = selected.first;
    
    LOG_MATCHING(logINFO, DEBUG_MATCH_FOUND,
                 pModel->GetIcaoType().c_str(),
                 pModel->GetWTC(),
                 pModel->GetDoc8643().classification,
                 pModel->GetRelatedGrp(),
                 selected.second->getAirline().c_str(),
                 selected.second->getRelOp(),
                 selected.second->livery.c_str(),
                 quality,
                 pModel->GetModelName().c_str());

    return true;
}

/// @details    Matching happens usually in just one pass by calling
///             CSLFindMatch().\n
///             The only exception is if the passed-in aircraft type is _not_ an official
///             ICAO type (which is what doc8643-matching bases on).
///             In that case, the first pass is done without doc8643-matching.
///             If that pass found no actual match,
///             then there is a second pass based on the default ICAO type.
int CSLModelMatching (const std::string& _type,
                      const std::string& _airline,
                      const std::string& _livery,
                      CSLModel* &pModel)
{
    // the number of matches applied, ie. the higher the worse
    int quality = 0;
    
    // Let's start...
    pModel = nullptr;
    
    // ...and let's stop right away if there is _absolutely no model_
    // (otherwise we will return one, no matter of how bad the matching quality is)
    if (glob.mapCSLModels.empty()) {
        LOG_MSG(logERR, ERR_MATCH_NO_MODELS);
        return -1;
    }
    
    // Loop is for trying the given type, plus occasionally also the default type.
    // If no type is given at all we use the default type:
    for (std::string type = _type.empty() ? glob.defaultICAO : _type;;)
    {
        if (CSLFindMatch(type, _airline, _livery,
                         // First pass not using Doc8643 matching?
                         type != glob.defaultICAO && !Doc8643IsTypeValid(type),
                         quality, pModel))
            return quality;
        
        // Can we do another loop, now with the default ICAO?
        if (type == glob.defaultICAO)
            break;                          // no, already identical (or just done)
        
        // yes, try with the default ICAO once again
        type = glob.defaultICAO;
    } // outer for loop
    
    // Actually...we must not get here. CSLFindMatch will return any model
    // if `bIgnoreNoMatch` is `false`. And it is `false` latest in the
    // second round.

    // ...as a last resort we just use _any random_ model
    pModel = &(iterRnd(glob.mapCSLModels.begin(),
                       glob.mapCSLModels.end())->second);
    LOG_MATCHING(logWARN, DEBUG_MATCH_NOTFOUND,
                 pModel->GetIcaoType().c_str(),
                 pModel->GetIcaoAirline().c_str(),
                 pModel->GetLivery().c_str(),
                 pModel->GetModelName().c_str());
    return quality+1;
}

}       // namespace XPMP2

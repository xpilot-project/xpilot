/// @file       CSLCopy.cpp
/// @brief      Copy .obj file just before loading, to replace dataRefs and texture
/// @details    These routines make a copy of an `.obj` file just before it
///             gets loaded by X-Plane. This way we can replace dataRefs and
///             textures.
///             1. `xsb_aircraft.txt` formats vary by now out in the wild.
///                For example, X-CSL/X-IvAp used to defined texture on the
///                `OBJ8` line. X-Plane, especially for instancing,
///                cannot replace textures on the fly. The solution is to
///                copy the `.obj` file, replacing the up to two lines defining
///                the texture to use, and then only have it loaded by X-Plane.
///             2. CSL-Models might not use correct dataRefs as supported by XPMP2.
///                For example are the widely used Bluebell models known to
///                have translated only a limited set of dataRefs from their
///                World Traffic origin into the libxplanemp world.
///                XPMP2 now supports a lot more dataRefs. Replacing
///                dataRef names in those models unlocks a number of feature
///                like rotating props/rotors, turning wheels, or reversers.
///
/// @see        https://twinfan.github.io/XPMP2/XSBAircraftFormat.html
///             for the `xsb_aircraft.txt` format definition as supported
///             by XPMP2.
///
/// @see        https://twinfan.github.io/XPMP2/CSLdataRefs.html
///             for a list of animation dataRefs supported by XPMP2.
///             And beyond that you can add even more using the
///             XPMPAddModelDataRef() function.
///
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

#define DEBUG_CPY_STARTING          "Creating model copy '%s' from '%s'|%s|%s"
#define INFO_CPY_SUCCEED            "Created model copy '%s' successfully"
#define ERR_CPY_FAILED              "Copying to '%s' failed, falling back to '%s'"
#define ERR_CPY_OBJ_NOT_FOUND       "Copying %s: CSLObj not found in CSLModel '%s'!"
#define ERR_CPY_MDL_NOT_FOUND       "Copying %s: CSLModel '%s' not found!"
#define ERR_CPY_THRDNOTRUN          "%s: Status OLS_COPYING but thread's not running??? Reverting to pathOrig"

/// Future keeping the state of the separate thread (we only want to trigger one at a time)
static std::future<bool> gFutCpy;
/// The key of the CSL model object that the current thread is running for
static std::string gThreadCSLkey;
/// The path of the CSL object that the current thread is running for
static std::string gThreadPath;

//
// MARK: Separate Thread functionality
//


// Perform the copy-on-load functionality, replacing dataRefs and textures, expected to be called via std::async
bool CSLObj::CopyAndReplace ()
{
    // This is a thread main function, set thread's name and try to catch all exceptions
    SET_THREAD_NAME("XPMP2_Cpy");
    
    // copy for a) faster access and b) to be sure it doesn't change while processing
    const bool bDoDR    = glob.bObjReplDataRefs;    // replace dataRefs?
    bool bDoTexture     = glob.bObjReplTextures && (!texture.empty() || !text_lit.empty()); // replace textures?
    
    bool doneTexture = false;               // did we replace TEXTURE already?
    bool doneTextureLit = false;            // did we replace TEXTURE_LIT already?
    
    bool bRet = false;
    try {
        // open input and output files
        std::ifstream fIn (pathOrig);
        if (!fIn) {
            LOG_MSG(logERR, "Couldn't open input/original file: %s", pathOrig.c_str());
            return false;
        }
        std::ofstream fOut (path, std::ios_base::out | std::ios_base::trunc);
        if (!fOut) {
            LOG_MSG(logERR, "Couldn't open output file for (over)writing: %s", path.c_str());
            return false;
        }
        
        // Process each line
        int lnNr = 0;
        while (fIn.good() && fOut.good() && !fIn.eof()) {
            // (modified) output written already?
            bool bOutWritten = false;
            
            // Read a line
            std::string ln;
            safeGetline(fIn, ln);
            ++lnNr;
            
            // After line 3 (the header) we insert a comment
            if (lnNr == 4)
                fOut << "# Created by " << glob.logAcronym << "/XPMP2 based on " << StripXPSysDir(pathOrig) << '\n';
            
            // Process TEXTURE
            if (bDoTexture && ln[0] == 'T' &&       // quick test
                ln.find("TEXTURE") == 0)            // full validation, line must _start_ with text TEXTURE
            {
                // separate by whitespace
                const std::vector<std::string> tok = str_tokenize(ln, " \t");
                if (tok.size() == 2) {
                    // Process TEXTURE, possibly replace the valie of one is given
                    if (tok[0] == "TEXTURE") {
                        if (!texture.empty()) {
                            fOut << "TEXTURE " << texture << '\n';
                            bOutWritten = true;
                        }
                        doneTexture = true;
                    } else if (tok[0] == "TEXTURE_LIT") {
                        if (!text_lit.empty()) {
                            fOut << "TEXTURE_LIT " << text_lit << '\n';
                            bOutWritten = true;
                        }
                        doneTextureLit = true;
                    }
                }
                
                // once we found both lines we no longer need to test for TEXTURE
                if (doneTexture && doneTextureLit)
                    bDoTexture = false;
            }
            
            // Process dataRef?
            if (!bOutWritten && bDoDR &&
                ln.find('/') != std::string::npos)  // quick test: any slash in line? (because any dataRef has a slash, and it is a very rare character otherwise, so a really good quick first indication)
            {
                // now we need to seriously test for any of the to-be-replaced dataRefs
                for (const Obj8DataRefs& drVal: glob.listObj8DataRefs)
                {
                    // search for the value to be replaved
                    const std::string::size_type p = ln.find(drVal.s);
                    if (p != std::string::npos) {
                        // found, replace it with the replacement
                        ln.replace(p, drVal.s.size(), drVal.r);
                        break;              // we do only one replacement
                    }
                }
            }
            
            // if not already written do so now
            if (!bOutWritten)
                fOut << ln << '\n';
        }
        
        // If we haven't reach EOF we probably had a problem
        if (!fIn.eof()) {
            LOG_MSG(logERR, "Didn't reach EOF of input file, unknown error");
            bRet = false;
        } else
            bRet = true;
        
        // properly close the files
        fOut.close();
        fIn.close();
    }
    catch(const std::exception& e) {
        LOG_MSG(logERR, "Exception: %s", e.what());
        bRet = false;
    }
    catch (...) {
        LOG_MSG(logERR, "Unknown exception");
        bRet = false;
    }
    
    // done
    return bRet;
}

//
// MARK: Main Thread Calls
//

// Update with the result of the copy operation
void CSLObj::SetCopyResult (bool bResult)
{
    if (bResult) {                      // success
        LOG_MSG(logINFO, INFO_CPY_SUCCEED, StripXPSysDir(path).c_str());
        pathOrig.clear();               // need no copy any longer
    } else {
        // Copying failed!
        LOG_MSG(logERR, ERR_CPY_FAILED, StripXPSysDir(path).c_str(), StripXPSysDir(pathOrig).c_str());
        path = std::move(pathOrig);     // fall back to original
        xpObjState = OLS_UNAVAIL;
    }
}

// Update _another_ CSLObj with the result of the copy operation
void CSLObj::SetOtherObjCopyResult (bool bResult)
{
    // Look for the CSLObj using the stored CSLId and path
    mapCSLModelTy::iterator cslIter;
    CSLModel* pCsl = CSLModelByKey(gThreadCSLkey);
    if (pCsl) {
        // find the object by path
        listCSLObjTy::iterator iter = std::find_if(pCsl->listObj.begin(),
                                                   pCsl->listObj.end(),
                                                   [](const CSLObj& o)
                                                   { return o.path == gThreadPath; });
        if (iter != pCsl->listObj.end())
            // Did find the CSL object!
            iter->SetCopyResult(bResult);
        else {
            LOG_MSG(logERR, ERR_CPY_OBJ_NOT_FOUND,
                    StripXPSysDir(gThreadPath).c_str(),
                    gThreadCSLkey.c_str());
        }
    } else {
        LOG_MSG(logERR, ERR_CPY_MDL_NOT_FOUND,
                StripXPSysDir(gThreadPath).c_str(),
                gThreadCSLkey.c_str());
    }
}

// Trigger a separate thread to copy the .obj file if needed
bool CSLObj::TriggerCopyAndReplace ()
{
    // Independend of who's calling we _always_ collect the state of the
    // copying thread in case it is now available, just to make sure it
    // get's processed, no matter of the initiating CSLObj object still exists and asks:
    const bool bFutValid = gFutCpy.valid();
    if (bFutValid &&
        gFutCpy.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        // we collect the result here and update the affected CSLObj
        SetOtherObjCopyResult(gFutCpy.get());   // by this call gFutCpy becomes invalid
        gThreadCSLkey.clear();
        gThreadPath.clear();
    }
    
    // Decide actions for _this_ based on status
    switch (GetObjState()) {
        case OLS_INVALID: return false;         // don't do, don't continue loading either
        case OLS_UNAVAIL:                       // possible...
            if (!NeedsObjCopy()) return true;   // ...but not needed, so don't do, just go ahead loading
            
            // Before we copy again we do a last check if the file by now already exists
            // This is very possible for .obj files which are shared across models of different livery (like fans, engines, glass elements...)
            if (ExistsFile(path)) {
                // It does exist, so no new copy is needed, just load it
                pathOrig.clear();
                return true;
            }
            
            // A thread's still running, so we'll need to wait
            if (bFutValid) return false;

            // Start a new thread to copy my .obj file
            gThreadCSLkey = cslKey;
            gThreadPath  = path;
            LOG_MSG(logDEBUG, DEBUG_CPY_STARTING,
                    StripXPSysDir(path).c_str(),
                    StripXPSysDir(pathOrig).c_str(),
                    texture.c_str(), text_lit.c_str())
            gFutCpy = std::async(std::launch::async, &CSLObj::CopyAndReplace, this);
            xpObjState = OLS_COPYING;
            return false;                       // need to wait for the copy operation to finish
            
        case OLS_COPYING:                       // Am or was copying for this object?
            if (!NeedsObjCopy()) return true;   // Result already collected, so go ahead and load
            if (bFutValid) return false;        // A thread's still running, so we'll need to wait
            // We shouldn't get here! No thread's running but we still need a copy???
            LOG_MSG(logERR, ERR_CPY_THRDNOTRUN,
                    path.c_str());
            // emergency procedure: revert back to original and load that
            path = std::move(pathOrig);     // fall back to original
            xpObjState = OLS_UNAVAIL;
            return true;
            
            // doesn't make sense calling us in these states
        case OLS_LOADING:
        case OLS_AVAILABLE:
            return true;
    }
    return true;
}

}       // namespace XPMP2

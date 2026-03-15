/// @file       CSLModels.h
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

#ifndef _CSLModels_h_
#define _CSLModels_h_

namespace XPMP2 {

/// Map of CSLPackages: Maps an id to the base path (path ends on forward slash)
typedef std::map<std::string,std::string> mapCSLPackageTy;

/// State of the X-Plane object: Is it being loaded or available?
enum ObjLoadStateTy {
    OLS_INVALID = -1,       ///< loading once failed -> invalid!
    OLS_UNAVAIL = 0,        ///< Not yet tried loading the CSL object
    OLS_COPYING,            ///< creating a new `.obj` file copy, replacing dataRefs and textures
    OLS_LOADING,            ///< async load underway
    OLS_AVAILABLE,          ///< X-Plane object available in `xpObj`
};

/// One `.obj` file of a CSL model (of which it can have multiple)
class CSLObj
{
public:
    std::string cslKey;     ///< key of the CSL model this belongs to
    std::string path;       ///< full path to the (potentially copied) `.obj` file
    std::string pathOrig;   ///< full path to the original `.obj` file IF there is the need to create a copy upon load
    std::string texture;    ///< texture file if defined, to be used in the TEXTURE line of the .obj file
    std::string text_lit;   ///< texture_lit file if defined, to be used in the TEXTURE_LIT line of the .obj file

protected:
    /// The X-Plane object reference to the loaded model as requested with XPLMLoadObjectAsync
    XPLMObjectRef       xpObj = NULL;
    /// State of the X-Plane object: Is it being loaded or available?
    ObjLoadStateTy xpObjState = OLS_UNAVAIL;
    
public:
    /// Constructor doesn't do much
    CSLObj (const std::string& _cslKey,
            const std::string& _path) : cslKey(_cslKey), path(_path) {}
    /// Generate standard move constructor
    CSLObj (CSLObj&& o) = default;
    CSLObj& operator = (CSLObj&& o) = default;
    /// Destructor makes sure the XP object is removed, too
    ~CSLObj ();
    
    /// State of the X-Plane object: Is it being loaded or available?
    ObjLoadStateTy GetObjState () const         { return xpObjState; }
    /// Is invalid?
    bool IsInvalid () const                     { return xpObjState == OLS_INVALID; }

    /// Determine which file to load and if we need a copied .obj file
    void DetermineWhichObjToLoad ();

    /// Read the obj file to calculate its vertical offset
    float FetchVertOfsFromObjFile () const;
    
    /// @brief Load and return the underlying X-Plane objects.
    /// @note Can return NULL while async load is underway!
    XPLMObjectRef GetAndLoadObj ();
    /// Starts loading the XP object
    void Load ();
    /// Free up the object
    void Unload ();
    
    /// Will this object require copying the `.obj` file upon load?
    bool NeedsObjCopy () const { return !pathOrig.empty(); }

protected:
    
    /// @brief Trigger a separate thread to copy the .obj file if needed
    /// @return true when copying has finished, false if the loading sequence needs to wait here (for the copy operation to start or finish)
    bool TriggerCopyAndReplace ();
    /// Update with the result of the copy operation
    void SetCopyResult (bool bResult);
    /// Update _another_ CSLObj with the result of the copy operation
    static void SetOtherObjCopyResult (bool bResult);
    
    /// Perform the copy-on-load functionality, replacing dataRefs and textures, expected to be called via std::async
    bool CopyAndReplace ();
    
    /// callback function called when loading is done
    static void XPObjLoadedCB (XPLMObjectRef inObject,
                               void *        inRefcon);
    /// Marks the CSL Model invalid in case object loading once failed
    void Invalidate ();
};

/// List of objects
typedef std::list<CSLObj> listCSLObjTy;

typedef std::pair<std::string,std::string> pairOfStrTy;

/// Represents a CSL model as it is saved on disk
class CSLModel
{
public:
    /// Combines match-relevant fields (beside ICAO a/c type)
    struct MatchCritTy {
    protected:
        /// ICAO Airline code this model represents: `xsb_aircraft.txt::AIRLINE`
        std::string         icaoAirline;
        int                 relOp = 0;      ///< 'related' group of the airline, if it exists, otherwise `0`
    public:
        /// Livery code this model represents: `xsb_aircraft.txt::LIVERY`
        std::string         livery;
        
        /// Sets airline (operator) and looks up its 'related' group
        void setAirline (const std::string& _icaoOp)
        { relOp = RelatedGet(REL_TXT_OP, icaoAirline = _icaoOp); }
        
        const std::string& getAirline () const { return icaoAirline; }  ///< ICAO Airline code / operator
        int getRelOp () const { return relOp; }                         ///< 'Related' group of operator, if any, otherwise `0`
        
        /// @brief Decide which criteria is better and keep that
        /// @return Did we cover o in some way? (false: needs to be treated separately)
        bool merge (const MatchCritTy& o);
    };
    /// Vector of match-relevant fields
    typedef std::vector<MatchCritTy> MatchCritVecTy;
public:
    /// short id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    std::string         shortId;
    /// full id: package name / shortId, expected to be unique
    std::string         cslId;
    /// name, formed by last part of path plus id
    std::string         modelName;
    /// simple hash of package name, for easy matching in networked setup
    std::uint16_t       pkgHash = 0;
    /// further match-relevant fields like airline and livery can be a list
    MatchCritVecTy      vecMatchCrit;
    /// list of objects representing this model
    listCSLObjTy        listObj;
    /// Vertical offset to be applied [m]
    float               vertOfs = 3.0f;
    /// Shall we try reading vertOfs from the OBJ8 file if we need this a/c?
    bool                bVertOfsReadFromFile = true;
    
    /// Path to the xsb_aircraft.txt file from where this model is loaded
    std::string         xsbAircraftPath;
    /// Line number in the xsb_aircraft.txt file where the model definition starts
    int                 xsbAircraftLn = 0;

protected:
    /// ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    std::string         icaoType;
    /// Proper Doc8643 entry for this model
    const Doc8643*      doc8643 = nullptr;
    /// "related" group for this model (a group of alike plane models), or 0
    int                 related = 0;
    /// Reference counter: Number of Aircraft actively using this model
    unsigned            refCnt = 0;
    /// Time point when refCnt reached 0 (used in garbage collection, in terms of XP's total running time)
    float               refZeroTs = 0.0f;
    /// future for asynchronously reading vertOfs
    std::future<float>  futVertOfs;
    
public:
    /// Constructor
    CSLModel () {}
    /// Generate standard move constructor
    CSLModel (CSLModel&& o) = default;
    CSLModel& operator = (CSLModel&& o) = default;

    /// Destructor frees resources
    virtual ~CSLModel ();
    
    /// @brief Set the a/c type model and add other match criteria
    /// @details Also fills `doc8643` and `related`.
    ///          Keeps most significant match criteria only
    ///          (if "DLH/-" and "DLH/D-ABCD" are defined, then only
    ///           "DLH/D-ABCD" is kept as that covers the "DLH/-" case, too)
    void AddMatchCriteria (const std::string& _type,
                           const MatchCritTy& _matchCrit,
                           int lnNr);
    /// Puts together the model name string from a path component and `shortId`
    void CompModelName ();
    
    /// Minimum requirement for using this object is: id, type, path
    bool IsValid () const { return !cslId.empty() && !icaoType.empty() && !listObj.empty(); }
    
    const std::string& GetShortId () const      { return shortId; }     ///< short id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    const std::string& GetId () const           { return cslId; }       ///< full id: package name / shortId, expected to be unique
    const std::string& GetModelName () const    { return modelName; }   ///< name, formed by last part of path plus id (human readable, but not guaranteed to be unique)
    const std::string& GetIcaoType () const     { return icaoType; }    ///< ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    const std::string& GetIcaoAirline () const  { return vecMatchCrit.at(0).getAirline(); }///< ICAO Airline code this model represents: `xsb_aircraft.txt::AIRLINE`
    const std::string& GetLivery () const       { return vecMatchCrit.at(0).livery; }      ///< Livery code this model represents: `xsb_aircraft.txt::LIVERY`
    int GetRelatedGrp () const                  { return related; }     ///< "related" group for this model (a group of alike plane models), or 0
    std::string GetKeyString () const;          ///< compiles the string used as key in the CSL model map

    // Data from Doc8643
    const Doc8643& GetDoc8643 () const          { return *doc8643; }    ///< Classification (like "L2P" or "L4J") and WTC (like "H" or "L/M")
    const char* GetWTC () const                 { return doc8643->wtc; }///< Wake turbulence category
    char GetClassType () const                  { return doc8643->GetClassType(); }
    char GetClassNumEng () const                { return doc8643->GetClassNumEng(); }
    int  GetNumEngines() const                  { return doc8643->GetNumEngines(); }
    char GetClassEngType () const               { return doc8643->GetClassEngType(); }
    bool HasRotor () const                      { return doc8643->HasRotor(); }

    /// Vertical Offset to be applied to aircraft model
    float GetVertOfs () const                   { return vertOfs; }
        
    /// (Minimum) )State of the X-Plane objects: Is it being loaded or available?
    ObjLoadStateTy GetObjState () const;
    /// (Minimum) )State of the X-Plane object: Is it invalid?
    bool IsObjInvalid () const                  { return GetObjState() == OLS_INVALID; }
    
    /// @brief Try get ALL object handles, only returns anything if it is the complete list
    /// @details This starts async loading of all objects.
    std::list<XPLMObjectRef> GetAllObjRefs ();
    
    /// Increase the reference counter for Aircraft usage
    void IncRefCnt () { ++refCnt; }
    /// Decrease the reference counter for Aircraft usage
    void DecRefCnt ();
    /// Current reference counter
    unsigned GetRefCnt () const { return refCnt; }
    
    /// Unload all objects which haven't been used for a while
    static float GarbageCollection (float  inElapsedSinceLastCall,
                                    float  inElapsedTimeSinceLastFlightLoop,
                                    int    inCounter,
                                    void * inRefcon);

protected:
    /// Unload all objects
    void Unload ();
    /// Read the obj files to fill CSLModel::vertOfs
    float FetchVertOfsFromObjFile () const;
};

/// Map of CSLModels (owning the object), ordered by related group / type
typedef std::map<std::string,CSLModel> mapCSLModelTy;

/// Multimap of references to CSLModels and match criteria for matching purposes
typedef std::multimap<unsigned long,std::pair<CSLModel*,const CSLModel::MatchCritTy*> > mmapCSLModelPTy;

//
// MARK: Global Functions
//

/// Initialization
void CSLModelsInit ();

/// Grace cleanup
void CSLModelsCleanup ();

/// @brief Read the CSL Models found in the given path and below
/// @param _path Path to a folder, which will be searched hierarchically for `xsb_aircraft.txt` files
/// @param _maxDepth Search shall go how many folders deep at max?
/// @return An empty string on success, otherwise a human-readable error message
const char* CSLModelsLoad (const std::string& _path,
                           int _maxDepth = 5);

/// @brief Find a model by unique id
/// @param _cslId The model's unique id to search for (package name/short id)
/// @param[out] _pOutIter Optional pointer to an iterator variable, receiving the iterator position of the found model
CSLModel* CSLModelById (const std::string& _cslId,
                        mapCSLModelTy::iterator* _pOutIter = nullptr);

/// @brief Find a model by (even more) unique key
/// @param _cslKey The model's unique key into the map
/// @param[out] _pOutIter Optional pointer to an iterator variable, receiving the iterator position of the found model
CSLModel* CSLModelByKey(const std::string& _cslKey,
                        mapCSLModelTy::iterator* _pOutIter = nullptr);

/// @brief Find a matching model
/// @param _type ICAO aircraft type like "A319"
/// @param _airline ICAO airline code like "DLH"
/// @param _livery Any specific livery code, in LiveTraffic e.g. the tail number
/// @param[out] pModel Receives the pointer to the matching CSL model, or NULL if nothing found
/// @return The number of passes needed to find a match, the lower the better the quality,
///         negative is error.
int CSLModelMatching (const std::string& _type,
                      const std::string& _airline,
                      const std::string& _livery,
                      CSLModel* &pModel);

}       // namespace XPMP2

#endif

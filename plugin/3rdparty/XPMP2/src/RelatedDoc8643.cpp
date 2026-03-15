/// @file       RelatedDoc8643.cpp
/// @brief      Reading of supporting text files:
///             - `related.txt` for creating groups of similar looking aircraft types;
///             - `relOp.txt` for creating groups of similar looking operator (liveries)
///             - `Doc8643.txt`, the official list of ICAO aircraft type codes;
///             - `Obj8DataRefs.txt`, a mapping list for replacing dataRefs in `.obj` files.
/// @details    A related group is declared simply by a line of ICAO a/c type codes read from the file.
///             Internally, the group is just identified by its line number in `related.txt`.
///             So the group "44" might be "A306 A30B A310", the Airbus A300 series.
///             Similarly, a group of operators is typically a group of mother/subsidiary
///             companies using the same or very similar liveries.
/// @details    Doc8643 is a list of information maintained by the ICAO
///             to list all registered aircraft types. Each type designator can appear multiple times
///             in the dataset for slightly differing models, but the classification und the WTC
///             will be the same in all those listing.\n
///             XPMP2 is only interested in type designator, classification, and WTC.
/// @author     Birger Hoppe
/// @copyright  (c) 2020-2024 Birger Hoppe
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

#define DEBUG_READ_RELATED      "Reading from '%s'"
#define ERR_RELATED_NOT_FOUND   "Could not open the file for reading"
#define WARN_DUP_RELATED_ENTRY  "%s: Duplicate entry for '%s' in line %d"

#define DEBUG_READ_DOC8643      "doc8643.txt: Reading from '%s'"
#define ERR_DOC8643_NOT_FOUND   "doc8643.txt: Could not open the file for reading"
#define ERR_DOC8643_READ_ERR    "doc8643.txt: Line %d did not match expectations: %s"

#define DEBUG_READ_OBJ8DR       "Obj8DataRefs.txt: Trying to read from '%s'"
#define ERR_OBJ8DR_NOT_FOUND    "Obj8DataRefs.txt: Could not open the file for reading"

#define ERR_CFG_LINE_READ       "Error '%s' while reading line %d of %s"
#define ERR_CFG_FILE_TOOMANY    "Too many errors while trying to read file"

/// Maximum number of warnings during file read before bailing
constexpr int ERR_CFG_FILE_MAXWARN = 5;
/// Maximum length of OS error message
constexpr size_t SERR_LEN = 255;

//
// MARK: related.txt
//

// Read the `related.txt` file, full path passed in
const char* RelatedLoad (RelTxtTy relType, const std::string& _path)
{
    // No need to read more than once
    mapRelatedTy& mapRel = glob.mapRelated[relType];
    if (!mapRel.empty())
        return "";
    
    // Open the related.txt file
    const std::string pathStripped = StripXPSysDir(_path);
    LOG_MSG(logDEBUG, DEBUG_READ_RELATED, pathStripped.c_str());
    std::ifstream fRelated (_path);
    if (!fRelated || !fRelated.is_open())
        return ERR_RELATED_NOT_FOUND;
    
    // read the file line by line and keep track of the line number as the internal id
    for (int lnNr = 1; fRelated; ++lnNr)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fRelated, ln);
        trim(ln);
        
        // skip over empty lines and over comments starting with a semicolon
        if (ln.empty() || ln[0] == ';')
            continue;
        
        // The remainder are expected to be arrays of ICAO type codes,
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        // which we just take as is and add them into the map
        // (no validation against Doc8643 here for good reasons.
        //  it increases flexibility if we allow to group non-official codes,
        //  e.g. one could group MD81 (non-official but offen mistakenly used)
        //  with MD80 (the officiel code) and both would be found)
        for (const std::string& key: tokens) {
            // We warn about duplicate entries
            if (glob.logLvl <= logWARN) {
                const auto it = mapRel.find(key);
                if (it != mapRel.cend()) {
                    LOG_MSG(logWARN, WARN_DUP_RELATED_ENTRY,
                            pathStripped.c_str(),
                            key.c_str(), lnNr);
                }
            }
            // But we use all entries - may the last one win
            mapRel[key] = lnNr;
        }
    }
    
    // Close the related.txt file
    fRelated.close();
    
    // Success
    return "";
}

// Load all related files
const char* RelatedLoad (const std::string _paths[], size_t _num)
{
    // outer loop: max as many runs as there are related files or paths
    for (size_t relType = 0;
         relType < REL_TXT_NUM && relType < _num;
         ++relType)
    {
        // Perform the loading
        const std::string& path = _paths[relType];
        if (!path.empty()) {
            if (ExistsFile(path)) {
                const char* s = RelatedLoad(RelTxtTy(relType), path);
                if (s && s[0]) {
                    LOG_MSG(logERR, "%s: %s",
                            StripXPSysDir(path).c_str(), s);
                    return s;
                }
            } else {
                LOG_MSG(logWARN, "'%s' does not exist, skipped",
                        StripXPSysDir(path).c_str());
            }
        }
    }
    return "";
}

// Find the related group for an ICAO a/c type, 0 if none
int RelatedGet (RelTxtTy relType, const std::string& _acType)
{
    try { return glob.mapRelated[relType].at(_acType); }
    catch (const std::out_of_range&) { return 0; }
}

//
// MARK: Doc843.txt
//

const Doc8643 DOC8643_EMPTY;    // objet returned if no other matches

// constructor setting all elements
Doc8643::Doc8643 (const std::string& _classification,
                  const std::string& _wtc)
{
    strncpy(classification, _classification.c_str(), sizeof(classification));
    classification[sizeof(classification)-1] = 0;
    strncpy(wtc, _wtc.c_str(), sizeof(wtc));
    wtc[sizeof(wtc)-1] = 0;
}

// Returns the wake category as per XP12's wake system
int Doc8643::GetWakeCat() const
{
    switch (wtc[0])
    {
    case '-':                           // Not assigned, which happens to the first few lines of Doc8643 with light aircraft, so we consider it light
    case 'L': return 0;                 // Light, also catches the "L/M" type, but XP only offers 4 values anyway
    case 'H': return 2;                 // Heavy, like B744
    case 'J': return 3;                 // Super, like A388
    default:
        return 1;                       // default: Medium
    }
}

// reads the Doc8643 file into mapDoc8643
const char* Doc8643Load (const std::string& _path)
{
    // must not read more than once!
    // CSLModels might already refer to these objects if already loaded.
    if (!glob.mapDoc8643.empty())
        return "";
    
    // open the file for reading
    std::ifstream fIn (_path);
    if (!fIn || !fIn.is_open())
        return ERR_DOC8643_NOT_FOUND;
    LOG_MSG(logDEBUG, DEBUG_READ_DOC8643, StripXPSysDir(_path).c_str());

    // regular expression to extract individual values, separated by TABs
    enum { DOC_MANU=1, DOC_MODEL, DOC_TYPE, DOC_CLASS, DOC_WTC, DOC_EXPECTED };
    const std::regex re("^([^\\t]+)\\t"                   // manufacturer
                        "([^\\t]+)\\t"                    // model
                        "([[:alnum:]]{2,4})\\t"           // type designator
                        "(-|[AGHLST][C1-8][EJPRT])\\t"    // classification
                        "(-|[HLMJ]|L/M)");                // wtc

    // loop over lines of the file
    std::string text;
    int errCnt = 0;
    for (int ln=1; fIn && errCnt <= ERR_CFG_FILE_MAXWARN; ln++) {
        // read entire line
        safeGetline(fIn, text);
        if (text.empty())           // skip empty lines silently
            continue;
        
        // apply the regex to extract values
        std::smatch m;
        std::regex_search(text, m, re);
        
        // add to map (if matched)
        if (m.size() == DOC_EXPECTED) {
            glob.mapDoc8643.emplace(m[DOC_TYPE],
                                    Doc8643(m[DOC_CLASS],
                                            m[DOC_WTC]));
        } else if (fIn) {
            // I/O was good, but line didn't match
            LOG_MSG(logWARN, ERR_DOC8643_READ_ERR, ln, text.c_str());
            errCnt++;
        } else if (!fIn && !fIn.eof()) {
            // I/O error
            char sErr[SERR_LEN];
            strerror_s(sErr, sizeof(sErr), errno);
            LOG_MSG(logERR, ERR_CFG_LINE_READ, sErr, ln, _path.c_str());
            errCnt++;
        }
    }
    
    // close file
    fIn.close();
    
    // too many warnings?
    if (errCnt > ERR_CFG_FILE_MAXWARN)
        return ERR_CFG_FILE_TOOMANY;
    
    // looks like success
    return "";
}

// return the matching Doc8643 object from the global map
const Doc8643& Doc8643Get (const std::string& _type)
{
    try { return glob.mapDoc8643.at(_type); }
    catch (const std::out_of_range&) { return DOC8643_EMPTY; }
}

// Is the given aircraft type a valid ICAO type as per Doc8643?
bool Doc8643IsTypeValid (const std::string& _type)
{
    return glob.mapDoc8643.count(_type) > 0;
}

//
// MARK: Obj8DataRefs.txt
//

// Load the content of the provided `Obj8DataRefs.txt` file
const char* Obj8DataRefsLoad (const std::string& _path)
{
    // No need to read more than once
    if (!glob.listObj8DataRefs.empty())
        return "";
    
    // Open the Obj8DataRefs.txt file
    LOG_MSG(logDEBUG, DEBUG_READ_OBJ8DR, StripXPSysDir(_path).c_str());
    std::ifstream fObjDR (_path);
    if (!fObjDR || !fObjDR.is_open())
        return ERR_OBJ8DR_NOT_FOUND;
    
    // read the file line by line and keep track of the line number for error messages
    int errCnt = 0;
    for (int lnNr = 1; fObjDR && errCnt <= ERR_CFG_FILE_MAXWARN; ++lnNr)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fObjDR, ln);
        trim(ln);
        
        // skip over empty lines and over comments starting with a semicolon
        if (ln.empty() || ln[0] == ';')
            continue;
        
        // The remainder are expected to be just two strings
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        if (tokens.size() != 2) {
            LOG_MSG(logERR, ERR_CFG_LINE_READ, "Expecting 2 entries", lnNr, StripXPSysDir(_path).c_str());
            ++errCnt;
            continue;
        }
        
        // ...of which the first one must include a forward-slash,
        // because that is our quick-check if a .obj line is to be analyzed any further at all
        if (tokens[0].find('/') == std::string::npos) {
            LOG_MSG(logERR, ERR_CFG_LINE_READ, "Left-hand side must include forward slash", lnNr, StripXPSysDir(_path).c_str());
            ++errCnt;
            continue;
        }

        // Just store without any further interpretation
        glob.listObj8DataRefs.emplace_back(std::move(tokens[0]),
                                           std::move(tokens[1]));        
    }
    
    // Close the related.txt file
    fObjDR.close();
    
    // too many warnings?
    if (errCnt > ERR_CFG_FILE_MAXWARN)
        return ERR_CFG_FILE_TOOMANY;
    
    // Success
    return "";
}

}   // namespace XPMP2

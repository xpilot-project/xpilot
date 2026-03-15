/// @file       RelatedDoc8643.h
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

#ifndef _RelatedDoc8643_h_
#define _RelatedDoc8643_h_

namespace XPMP2 {

//
// MARK: related.txt and relOp.txt
//

enum RelTxtTy : size_t {
    REL_TXT_DESIGNATOR = 0,         ///< related file for plane type (designator), `related.txt`
    REL_TXT_OP,                     ///< related file for operators, `relOp.txt`
    REL_TXT_NUM                     ///< always last: number of "related" files
};

/// Map of group membership: ICAO a/c type maps to line in related.txt
typedef std::map<std::string, int> mapRelatedTy;

/// @brief Read the `related.txt` file, full path passed in
/// @return Empty string on success, otherwise error message
const char* RelatedLoad (RelTxtTy relType, const std::string& _path);

/// @brief Load all related files
/// @note Non-existing files are skipped with warning in log, but don't return an error
const char* RelatedLoad (const std::string _paths[], size_t _num);

/// Find the related group for a given key, `0` if none
int RelatedGet (RelTxtTy relType, const std::string& _key);

//
// MARK: Doc843.txt
//

/// @brief Represents a line in the Doc8643.txt file, of which we use only classification and WTC
/// @details Each line has Example lines:
///     `(ANY)    Glider    GLID    -    -`
///     `AAMSA    A-9 Quail    A9    L1P    L`
///     `AIRBUS    A-380-800    A388    L4J    H`
///     `CESSNA    172    C172    L1P    L`
///     `CESSNA    172 Skyhawk    C172    L1P    L`
///     `FAIRCHILD (1)    C-26 Metro    SW4    L2T    L/M`
///     `SOLAR IMPULSE    1    SOL1    L4E    L`
///     `SOLOY    Bell 47    B47T    H1T    L`
struct Doc8643 {
public:
    char classification[4]  = {0,0,0,0};
    char wtc[4]             = {0,0,0,0};
public:
    Doc8643 () {}
    Doc8643 (const std::string& _classification,
             const std::string& _wtc);
    
    /// Is empty, doesn't contain anything?
    bool empty () const { return wtc[0] == 0; }
    
    char GetClassType () const      { return classification[0]; }
    char GetClassNumEng () const    { return classification[1]; }
    int  GetNumEngines () const     { return ('0' <= classification[1] && classification[1] <= '9') ? classification[1] - '0' : 0; }
    char GetClassEngType () const   { return classification[2]; }
    bool HasRotor () const          { return (classification[0] == 'H' ||
                                              classification[0] == 'G'); }

    /// @brief Returns the wake category as per XP12's wake system
    /// @see https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/
    int GetWakeCat() const;
};

/// Map of Doc8643 information, key is the (icao) type code
typedef std::map<std::string, Doc8643> mapDoc8643Ty;

/// Load the content of the provided `Doc8643.txt` file
const char* Doc8643Load (const std::string& _path);

/// @brief Return a reference to the matching Doc8643 object.
/// @return If no match can be found a reference to a standard empty object is returned.
const Doc8643& Doc8643Get (const std::string& _type);

/// Is the given aircraft type a valid ICAO type as per Doc8643?
bool Doc8643IsTypeValid (const std::string& _type);

//
// MARK: Obj8DataRefs.txt
//

/// A pair of strings, first one to search for, second one to replace it with
struct Obj8DataRefs {
    std::string     s;      ///< search the `.obj` file for this string
    std::string     r;      ///< if found replace it with this string
    
    /// Constructor just loads the strings
    Obj8DataRefs (std::string&& _s, std::string&& _r) :
    s (std::move(_s)), r (std::move(_r)) {}
};

/// a list of Obj8DataRefs definitions
typedef std::list<Obj8DataRefs> listObj8DataRefsTy;

/// Load the content of the provided `Obj8DataRefs.txt` file
const char* Obj8DataRefsLoad (const std::string& _path);

}

#endif

/// @file       Utilities.h
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

#ifndef _Utilities_h_
#define _Utilities_h_

namespace XPMP2 {

//
// MARK: General texts
//

#define ERR_ASSERT              "ASSERT FAILED: %s"
#define ERR_EXCEPTION           "EXCEPTION CAUGHT: %s"

// Required supplemental files
constexpr const char* RSRC_RELATED      = "related.txt";
constexpr const char* RSRC_REL_OP       = "relOp.txt";
constexpr const char* RSRC_DOC8643      = "Doc8643.txt";
constexpr const char* RSRC_MAP_ICONS    = "MapIcons.png";
constexpr const char* RSRC_OBJ8DATAREFS = "Obj8DataRefs.txt";

//
// MARK: Default configuration callbacks
//

int     PrefsFuncIntDefault     (const char *, const char *, int _default);

//
// MARK: File access helpers
//

#if IBM
#define PATH_DELIM_STD '\\'
#else
#define PATH_DELIM_STD '/'
#endif

/// Does a file path exist?
bool ExistsFile (const std::string& filename);

/// Is path a directory?
bool IsDir (const std::string& path);

/// @brief Create directory if it does not exist
/// @return Does directory (now) exist?
bool CreateDir(const std::string& path);

/// @brief Copy file if source is newer or destination missing
/// @return Does the destination file (now) exist?
bool CopyFileIfNewer(const std::string& source, const std::string& destDir);

/// List of files in a directory (wrapper around XPLMGetDirectoryContents)
std::list<std::string> GetDirContents (const std::string& path);

/// Read a line from a text file, no matter if ending on CRLF or LF
std::istream& safeGetline(std::istream& is, std::string& t);

/// Returns XP's system directory, including a trailing slash
const std::string& GetXPSystemPath ();

/// If a path starts with X-Plane's system directory it is stripped
std::string StripXPSysDir (const std::string& path);

/// Removes everything after the last dot, the dot including
void RemoveExtension (std::string& path);

//
// MARK: String helpers
//

#define WHITESPACE              " \t\f\v\r\n"

/// change a std::string to uppercase
std::string& str_tolower(std::string& s);

/// @brief trimming of string (from right)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& rtrim(std::string& s, const char* t = WHITESPACE)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

/// @brief trimming of string (from left)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& ltrim(std::string& s, const char* t = WHITESPACE)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

/// @brief trimming of string (from both ends)
/// @see https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline std::string& trim(std::string& s, const char* t = WHITESPACE)
{
    return ltrim(rtrim(s, t), t);
}

/// @brief Returns everything left of any of terminators
inline std::string leftOf(const std::string& s, const std::string& terminators)
{
    return s.substr(0, std::min(s.find_first_of(terminators), s.size()));
}

/// separates string into tokens
std::vector<std::string> str_tokenize (const std::string s,
                                       const std::string tokens,
                                       bool bSkipEmpty = true);

//
// MARK: Math helpers
//

/// Pi
constexpr double PI = 3.1415926535897932384626433832795028841971693993751;

/// Epsilon, a small number
constexpr float EPSILON_F = 0.00001f;

/// Are these two float near-equal? (to avoid trying something like a == b)
inline bool fequal (float a, float b)
{ return ((a - EPSILON_F) < b) && ((a + EPSILON_F) > b); }


/// Convert radians to degrees, normalized to [0..360)
template <class T>
inline T rad2deg (const T _rad)
{ return (_rad < T(0) ? T(360) : T(0)) + _rad * T(180) / T(PI); }

/// Convert degree to radians
template <class T>
inline T deg2rad (const T _deg)
{ return _deg * T(PI) / T(180); }

/// Square
template <class T>
inline T sqr (const T a) { return a*a; }

/// Pythagorean distance between two points in a 3-D world
template <class T>
inline T dist (const T x1, const T y1, const T z1,
               const T x2, const T y2, const T z2)
{
    return std::hypot(x1-x2, y1-y2, z1-z2);
}

/// atan2 converted to degrees: the angle between (0|0) and the given point
inline float atan2deg (float x, float y)
{ return rad2deg(atan2(x,y)); }

/// @brief Angle of line from point (x1|z1) to point (x2|z2)
/// @note Points are given in XP's local coordinates with x -> east and z -> south(!),
///       but as in atan2 angles grow counter-clockwise, but we expect clockwise,
///       these 2 effects (z south, counter-clockwise angles) neutralize.
/// @note atan2 returns 0° when pointing along the x axis, ie. east,
///       when we would expect a result of 90°.
///       Hence, we need to add 90°=PI/2 to the result.
inline float angleLocCoord (float x1, float z1, float x2, float z2)
{ return rad2deg(atan2(z2-z1,x2-x1) + float(PI/2.0)); }

/// (Shortest) difference between 2 angles: How much to turn to go from h1 to h2?
float headDiff (float head1, float head2);

/// Normalize a heading value to [0..360), works for both float and double values
template <class numT>
numT headNormalize (numT _head)
{
    if (_head < numT(0))
        _head += std::ceil(_head/-numT(360)) * numT(360);
    else if (_head >= numT(360))
        _head -= std::floor(_head/numT(360)) * numT(360);
    return _head;
}

/// @brief Convert heading/pitch to normalized x/y/z vector
/// @note Given all the trigonometric functions this is probably expensive,
///       so use with care and avoid in flight loop callback when unnecessary.
/// @returns a `valarray` with 3 values, x, y, and z
std::valarray<float> HeadPitch2Vec (const float head, const float pitch);

/// Convert heading/pitch/roll to unit and normal vector, ie. returns 6 values, first 3 like HeadPitch2Vec, second three the normal vector (pointing "up")
std::valarray<float> HeadPitchRoll2Normal(const float head, const float pitch, const float roll);

//
// MARK: Misc
//

/// @brief Update cached values during a flight loop callback in XP's main thread to have them when called from a non-main thread
/// @returns the value of GetMiscNetwTime()
float UpdateCachedValuesGetNetwTime ();

/// Get synched network time from X-Plane (sim/network/misc/network_time_sec) as used in Log.txt
float GetMiscNetwTime ();

/// @brief Return the network time as a string like used in the XP's Log.txt
/// @param _time If given convert that time, otherwise convert XPMP2::GetMiscNetwTime()
std::string GetMiscNetwTimeStr (float _time = NAN);

/// Text string for current graphics driver in use
const char* GetGraphicsDriverTxt ();

/// X-Plane in a Pause state?
bool IsPaused ();

/// Is current X-Plane view an external view (outside a cockpit)?
bool IsViewExternal ();

/// @brief Convenience function to check on something at most every x seconds
/// @param _lastCheck Provide a float which holds the time of last check (init with `0.0f`)
/// @param _interval [seconds] How often to perform the check?
/// @param _now Current time, possibly from a call to GetTotalRunningTime()
/// @return `true` if more than `_interval` time has passed since `_lastCheck`
bool CheckEverySoOften (float& _lastCheck, float _interval, float _now);

/// @brief Convenience function to check on something at most every x seconds
/// @param _lastCheck Provide a float which holds the time of last check (init with `0.0f`)
/// @param _interval [seconds] How often to perform the check?
/// @return `true` if more than `_interval` time has passed since `_lastCheck`
inline bool CheckEverySoOften (float& _lastCheck, float _interval)
{ return CheckEverySoOften(_lastCheck, _interval, GetMiscNetwTime()); }

//
// MARK: Logging Support
//

/// @brief To apply printf-style warnings to our functions.
/// @see Taken from imgui.h's definition of IM_FMTARGS
#if defined(__clang__) || defined(__GNUC__)
#define XPMP2_FMTARGS(FMT)  __attribute__((format(printf, FMT, FMT+1)))
#else
#define XPMP2_FMTARGS(FMT)
#endif

/// Logging level
enum logLevelTy {
    logDEBUG = 0,       ///< Debug, highest level of detail
    logINFO,            ///< regular info messages
    logWARN,            ///< warnings, i.e. unexpected, but uncritical events, maybe leading to unwanted display, but still: display of aircraft
    logERR,             ///< errors mean, aircraft can potentially not be displayed
    logFATAL,           ///< fatal is shortly before a crash
    logMSG              ///< will always be output, no matter what has been configured, cannot be suppressed
};

/// Returns ptr to static buffer filled with formatted log string
const char* LogGetString ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, va_list args );
             
/// Log Text to log file
void LogMsg ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, ... ) XPMP2_FMTARGS(5);

//
// MARK: Logging macros
//

/// @brief Log a message if lvl is greater or equal currently defined log level
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MSG(lvl,...)  {                                         \
    if (lvl >= XPMP2::glob.logLvl)                                  \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Log a message about matching if logging of model matching is enabled
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MATCHING(lvl,...)  {                                    \
    if (XPMP2::glob.bLogMdlMatch && lvl >= glob.logLvl)             \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Throws an exception using XPMP2Error
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define THROW_ERROR(...)                                            \
throw XPMP2Error(__FILE__, __LINE__, __func__, __VA_ARGS__);

/// @brief Throw in an assert-style (logging takes place in XPMP2Error constructor)
/// @note This conditional check _always_ takes place, independend of any build or logging settings!
#define LOG_ASSERT(cond)                                            \
    if (!(cond)) {                                                  \
        THROW_ERROR(ERR_ASSERT,#cond);                              \
    }

/// @brief Standard catch clauses for dealing with aircraft: logs message, sets aircraft invalid
#define CATCH_AC(ac)                                                \
    catch (const std::exception& e) {                               \
        LOG_MSG(logFATAL, ERR_EXCEPTION, e.what());                 \
        (ac).SetInvalid();                                          \
    }                                                               \
    catch (...) {                                                   \
        LOG_MSG(logFATAL, ERR_EXCEPTION, "<unknown>");              \
        (ac).SetInvalid();                                          \
    }


//
// MARK: Compiler differences
//

#if APL == 1 || LIN == 1
// not quite the same but close enough for our purposes
inline void strncpy_s(char * dest, size_t destsz, const char * src, size_t count)
{
    strncpy(dest, src, std::min(destsz,count)); dest[destsz - 1] = 0;
}

// these simulate the VC++ version, not the C standard versions!
inline struct tm *gmtime_s(struct tm * result, const time_t * time)
{ return gmtime_r(time, result); }
inline struct tm *localtime_s(struct tm * result, const time_t * time)
{ return localtime_r(time, result); }

#endif

/// Simpler access to strncpy_s if dest is a char array (not a pointer!)
#define STRCPY_S(dest,src) strncpy_s(dest,sizeof(dest),src,sizeof(dest)-1)
#define STRCPY_ATMOST(dest,src) strncpy_s(dest,sizeof(dest),strAtMost(src,sizeof(dest)-1).c_str(),sizeof(dest)-1)

// XCode/Linux don't provide the _s functions, not even with __STDC_WANT_LIB_EXT1__ 1
#if APL
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ return strerror_r(errnum, buf, bufsz); }
#elif LIN
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ strerror_r(errnum, buf, bufsz); return 0; }
#endif

// In case of Mac we need to prepare for HFS-to-Posix path conversion
#if APL
/// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path
std::string TOPOSIX (const std::string& p);
/// Checks how XPLM_USE_NATIVE_PATHS is set (recommended to use), and if not set converts the path from POSIX to HFS
std::string FROMPOSIX (const std::string& p);
#else
/// On Lin/Win there is no need for a conversion, but we do treat `p` now as `std::string`
inline std::string TOPOSIX (const std::string& p) { return p; }
/// On Lin/Win there is no need for a conversion, but we do treat `p` now as `std::string`
inline std::string FROMPOSIX (const std::string& p) { return p; }
#endif

// MARK: Thread names
#ifdef DEBUG
// This might not work on older Windows version, which is why we don't publish it in release builds
#if IBM
#define SET_THREAD_NAME(sName) SetThreadDescription(GetCurrentThread(), L##sName)
#elif APL
#define SET_THREAD_NAME(sName) pthread_setname_np(sName)
#elif LIN
#define SET_THREAD_NAME(sName) pthread_setname_np(pthread_self(),sName)
#endif
#else
#define SET_THREAD_NAME(sName)
#endif

}       // namespace XPMP2

#endif

/// @file       Sound.h
/// @brief      Sound for aircraft, based on XPLMSound
///
/// @author     Birger Hoppe
/// @copyright  (c) 2022 Birger Hoppe
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

#pragma once

namespace XPMP2 {

//
// MARK: FMOD Error Handling
//

/// Exception class to pass on error information
class FmodError : public std::runtime_error
{
public:
    FMOD_RESULT fmodRes;        ///< the actual FMOD result code
    std::string sFile;          ///< file path of code file
    int ln;                     ///< line number here in the code
    std::string sFunc;          ///< name of the function the error occurred in
public:
    /// Constructor taking on a descriptive string and the `FMOD_RESULT`
    FmodError (const std::string& _what, FMOD_RESULT _r,
               const std::string& _file,
               int _ln, const std::string& _sFunc) :
    std::runtime_error(leftOf(_what,"( ")),     // takes over the name of the function called, but not all the parameter string
    fmodRes(_r), sFile(_file), ln(_ln), sFunc(_sFunc)
    {}
    
    /// Log myself to Log.txt as an error
    void LogErr () const {
        LogMsg(sFile.c_str(), ln, sFunc.c_str(), logERR,
#if INCLUDE_FMOD_SOUND + 0 >= 1
               "FMOD Error %d - '%s' in %s", fmodRes,
               FMOD_ErrorString(fmodRes),
#else
               "FMOD Error %d in %s", fmodRes,
#endif
               what());
    }
};

/// last FMOD result code, needed by the following macro, better don't rely on anywhere else
extern FMOD_RESULT gFmodRes;

/// @brief Log an error if `fmodRes` is not `FMOD_OK`
#define FMOD_TEST(fmodCall)  {                                              \
    if ((gFmodRes = (fmodCall)) != FMOD_OK)                                 \
        throw FmodError(#fmodCall, gFmodRes, __FILE__, __LINE__, __func__); \
}

/// @brief Standard catch clause to handle FmodError exceptions by just logging them
#define FMOD_CATCH catch (const FmodError& e) { e.LogErr(); }

/// Just log an error without raising an exception
#define FMOD_LOG(fmodCall)  {                                                                \
    if ((XPMP2::gFmodRes = (fmodCall)) != FMOD_OK)                                           \
        XPMP2::FmodError(#fmodCall, XPMP2::gFmodRes, __FILE__, __LINE__, __func__).LogErr(); \
}

//
// MARK: Sound Files
//

class SoundSystem;

/// @brief Convert heading/pitch to normalized x/y/z vector
/// @note Given all the trigonometric functions this is probably expensive,
///       so use with care and avoid in flight loop callback when unnecessary.
FMOD_VECTOR FmodHeadPitch2Vec (const float head, const float pitch);

/// @brief Convert heading/pitch to normal x/y/z vector
/// @note Given all the trigonometric functions this is probably expensive,
///       so use with care and avoid in flight loop callback when unnecessary.
void FmodHeadPitchRoll2Normal(const float head, const float pitch, const float roll,
                              FMOD_VECTOR& vecDir, FMOD_VECTOR& vecNorm);

/// @brief Normalize Pitch/Heading
/// @details Heading: [0; 360), Pitch: [-90; 90]
void FmodNormalizeHeadingPitch(float& head, float& pitch);

/// @brief Represents a sound file
/// @see For 3D cones also see https://www.fmod.com/docs/2.01/api/core-api-channelcontrol.html#channelcontrol_set3dconesettings
class SoundFile {
public:
    SoundSystem* pSndSys = nullptr; ///< Sound system for which this object was created
    std::string filePath;           ///< File path to sound file
    bool bLoop = true;              ///< sound to be played in a loop?
    
    // 3D Cone information, to be applied to the channel later when playing the sound
    float coneDir = NAN;            ///< Which direction relative to plane's heading does the cone point to? (180 would be typical for jet engines)
    float conePitch = NAN;          ///< Which pitch does the cone point to (up/down)? (0 would be typical, ie. level with the plane)
    float coneInAngle = NAN;        ///< Inside cone angle. This is the angle spread within which the sound is unattenuated.
    float coneOutAngle = NAN;       ///< Outside cone angle. This is the angle spread outside of which the sound is attenuated to its SoundFile::coneOutVol.
    float coneOutVol = NAN;         ///< Cone outside volume.
    
public:
    /// @brief Construct a sound object from a file name and have it loaded asynchonously
    /// @throws FmodError in case of errors during `CreateSound`
    SoundFile (SoundSystem* _pSndSys,
               const std::string& _filePath, bool _bLoop,
               float _coneDir, float _conePitch,
               float _coneInAngle, float _coneOutAngle, float _coneOutVol);
    
    /// Make sure destructor is virtual
    virtual ~SoundFile () {}
    
    /// Copying is not permitted
    SoundFile (const SoundFile& o) = delete;
    
    /// Has full cone information?
    bool hasConeInfo () const
    {
        return
            !std::isnan(coneDir) &&
            !std::isnan(conePitch) &&
            !std::isnan(coneInAngle) &&
            !std::isnan(coneOutAngle) &&
            !std::isnan(coneOutVol);
    }
    
    /// Ready to play?
    virtual bool isReady () = 0;
};

/// Smart pointer to a SoundFile object
typedef std::unique_ptr<SoundFile> SoundFilePtr;
/// Map of all sounds, indexed by a sound name (type)
typedef std::map<std::string,SoundFilePtr> mapSoundPtrTy;

/// Represents a WAV file, read into memory, played by XP's Sound API
class SoundWAV : public SoundFile {
public:
    int16_t*    pBuf = nullptr;             ///< audio buffer of PCM16 data
    uint32_t    bufferSize = 0;             ///< buffer size in bytes
    int         freqHz = 0;                 ///< sound frequency in Hz
    int         numChannels = 0;            ///< number of channels in the sound sample
    
public:
    /// Constructor, will load immediately and throw an exception if loading is unsuccessful
    SoundWAV (SoundSystem* _pSndSys,
              const std::string& _filePath, bool _bLoop,
              float _coneDir, float _conePitch,
              float _coneInAngle, float _coneOutAngle, float _coneOutVol);
    
    /// Destructor
    ~SoundWAV() override;
    
    /// Ready to play? As we load right away in the constructor we're always ready
    bool isReady () override { return true; }
    
protected:
    /// Reads a WAV file into a PCM16 memory buffer, throws exceptions in case of errors
    void WavRead ();
};

//
// MARK: Sound System
//

/// A sound object being played, called channel
class SoundChannel {
public:
    FMOD_CHANNEL*   pChn    = nullptr;      ///< the actual sound channel
    SoundFile*      pSnd    = nullptr;      ///< the sound file the sound was created from
    float           vol     = 1.0f;         ///< volume (if not muted)
    bool            bMuted  = false;        ///< currently muted?
    int             nPauseCountdown = 2;    ///< frame till unpause this sound (starts paused to avoid crackling)
public:
    /// Default Constructor creates an invalid object
    SoundChannel () {}
    /// Constructor
    SoundChannel (FMOD_CHANNEL* c, SoundFile* s, float v) : pChn(c), pSnd(s), vol(v) {}
    /// Has valid pointers? (Doesn't say if actual objects are valid any longer)
    operator bool () const { return pChn && pSnd; }
    /// Need to unpause now? (returns `true` only once when countdown reaches zero)
    bool ShallUnpause () { return nPauseCountdown > 0 ? (--nPauseCountdown == 0) : false;  }
};

/// Base class for sound systems, practically empty
class SoundSystem {
protected:
    /// Map of all sounds, indexed by a sound name (type)
    mapSoundPtrTy mapSounds;
    /// Keeps track of currently playing sounds, their key serving as id
    std::map<uint64_t,SoundChannel> mapChn;
private:
    /// Next sound channel id
    uint64_t uNxtId = 0;
    /// Cache to avoid re-lookup of repeatedly same sound id
    uint64_t cacheSndId = 0;
    SoundChannel* pCacheChn = nullptr;
    
public:
    /// Construtor
    SoundSystem() {}
    /// Destructor
    virtual ~SoundSystem() {}
    
    /// Loads a sound file so it becomes ready to play
    virtual bool LoadSoundFile (const std::string& _sndName,
                                const std::string& _filePath, bool _bLoop,
                                float _coneDir, float _conePitch,
                                float _coneInAngle, float _coneOutAngle, float _coneOutVol) = 0;
    /// Enumerate all loaded sounds
    const char* EnumerateSounds (const char* prevName, const char** ppFilePath);

    /// Play a new sound, returns an id for that sound
    virtual uint64_t Play (const std::string& sndName, float vol, const Aircraft& ac) = 0;
    /// Unpause a sound, which got started in a paused state to avoid crackling
    virtual void Unpause (uint64_t sndId) = 0;
    /// Stop the sound
    virtual void Stop (uint64_t sndId) = 0;
    /// Update sound's position and orientation
    virtual void SetPosOrientation (uint64_t sndId, const Aircraft& ac, bool bDoOrientation) = 0;
    /// Set sound's volume
    virtual void SetVolume (uint64_t sndId, float vol) = 0;
    /// Mute the sound (temporarily)
    virtual void SetMute (uint64_t sndId, bool bMute) = 0;
    
    /// Any updates to be done at the end of the frame
    virtual void Update () = 0;

    /// Set Master Volume, effectively a multiplicator to SetVolume()
    virtual void SetMasterVolume (float volMaster) = 0;
    /// Mute all sounds (temporarily)
    virtual void SetAllMute (bool bMute) = 0;
    
    /// Return possible audio devices, returns if `i` was valid and `devName` filled
    virtual bool GetAudioDeviceName (int i, std::string& devName) const = 0;
    /// Set a specific audio device as the output device, returns if it was found
    virtual bool SetAudioDevice (int i) = 0;
    /// Get currently active audio device index
    virtual int GetActiveAudioDevice () const = 0;
    
    /// Is the sound id available?
    virtual bool IsValid (uint64_t sndId);

protected:
    /// Add one more channel, returning the newly created id
    std::pair<uint64_t,SoundChannel*> AddChn (SoundFile* pSnd, float vol, FMOD_CHANNEL* pChn = nullptr);
    /// Return the SoundChannel object for a given id, or `nullptr` if not found
    SoundChannel* GetChn (uint64_t sndId);
    /// Remove a channel from out tracking
    void RemoveChn (uint64_t sndId);
};

/// The sound system in use
extern SoundSystem* gpSndSys;


/// @brief Encapsulates the XP12 Sound System
/// @details Looks for availability of the new XP12.04 sound functions (see XPLMSound.h),
///          and use those for playing sounds. Only PCM16 sounds from memory can be played
///          this way, which means that we need to read `WAV` files and load their PCM data to memory.
class SoundSystemXP : public SoundSystem {
protected:
    float   volMaster = 1.0f;       ///< Master volume, effectively a multiplicator for individual volume
    bool    bAllMuted = false;      ///< All sounds (temporarily) muted?
public:
    /// Construtor, throws exception if XP sound system unavailable (prior to XP12.04)
    SoundSystemXP();
    /// Destructor
    ~SoundSystemXP() override {}
    
    /// @brief Loads a sound file so it becomes ready to play
    /// @note In this sound system, we can only deal with `WAV` files as we have to read the ourselves
    bool LoadSoundFile (const std::string& _sndName,
                        const std::string& _filePath, bool _bLoop,
                        float _coneDir, float _conePitch,
                        float _coneInAngle, float _coneOutAngle, float _coneOutVol) override;

    /// Play a new sound, returns an id for that sound
    uint64_t Play (const std::string& sndName, float vol, const Aircraft& ac) override;
    /// Unpause a sound, which got started in a paused state to avoid crackling
    void Unpause (uint64_t sndId) override;
    /// Stop the sound
    void Stop (uint64_t sndId) override;
    /// Update sound's position and orientation
    void SetPosOrientation (uint64_t sndId, const Aircraft& ac, bool bDoOrientation) override;
    /// Set sound's volume
    void SetVolume (uint64_t sndId, float vol) override;
    /// Mute the sound (temporarily)
    void SetMute (uint64_t sndId, bool bMute) override;
    
    /// Any updates to be done at the end of the frame (not for XP system)
    void Update () override {}
    
    /// Set Master Volume, effectively a multiplicator to SetVolume()
    void SetMasterVolume (float volMaster) override;
    /// Mute all sounds (temporarily)
    void SetAllMute (bool bMute) override;

    /// Return possible audio devices: For X-Plane device, we don't interfere and only return "X-Plane"
    bool GetAudioDeviceName (int i, std::string& devName) const override;
    /// Set a specific audio device as the output device: For X-Plane, we only support "X-Plane" and don't actually interfere with X-Plane's output control
    bool SetAudioDevice (int i) override;
    /// Get currently active audio device index
    int GetActiveAudioDevice () const override;

protected:
    /// Callback required by XPLMPlayPCMOnBus
    static void PlayCallback (void*         inRefcon,
                              FMOD_RESULT   status);
    /// Update an individual channel's volume
    void ChnSetVol (const SoundChannel& chn);
    /// Update all channels' volume
    void AllChnSetVol ();
};

//
// MARK: Global Functions
//

/// Initialize the sound module and load the sounds
void SoundInit ();

/// Prepare for this frame's updates, which are about to start
void SoundUpdatesBegin();

/// Tell FMOD that all updates are done
void SoundUpdatesDone ();

/// Graceful shoutdown
void SoundCleanup ();

}

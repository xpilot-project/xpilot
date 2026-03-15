/// @file       Sound.h
/// @brief      Sound for aircraft, based on the FMOD library
///
/// @note       If built with `INCLUDE_FMOD_SOUND=1` then
///             Audio Engine is FMOD Core API by Firelight Technologies Pty Ltd.
///             Understand FMOD [licensing](https://www.fmod.com/licensing) and
///             [attribution requirements](https://www.fmod.com/attribution) first!
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

// Only included if specified. Understand FMOD licensing and attribution first!
#if INCLUDE_FMOD_SOUND + 0 >= 1

namespace XPMP2 {

class SoundSystemFMOD;

/// Represents a sound file to be passed on to FMOD to be played
class SoundFMOD : public SoundFile {
protected:
    FMOD_SOUND* pSound = nullptr;   ///< FMOD sound object
    bool bLoaded = false;           ///< cached version of the sound's `openstate`
    
public:
    /// Constructor creates the FMOD sound object and initiates loading of the file
    SoundFMOD (SoundSystemFMOD* _pSndSys,
               const std::string& _filePath, bool _bLoop,
               float _coneDir, float _conePitch,
               float _coneInAngle, float _coneOutAngle, float _coneOutVol);
    /// Destructor removes the sound object
    ~SoundFMOD() override;
    
    /// Finished loading, ready to play?
    bool isReady () override;
    
    /// Return the FMOD sound
    FMOD_SOUND* GetSnd() const { return pSound; }
    /// Return the FMOD sound system (a type cast of `pSndSys`)
    SoundSystemFMOD* GetFmodSys() const;
};


/// @brief Encapsulates direct access to the FMOD Sound System
class SoundSystemFMOD : public SoundSystem {
protected:
    static SoundSystemFMOD* me;                         ///< static pointer to myself
    FMOD_SYSTEM*            pFmodSystem = nullptr;      ///< FMOD system
    unsigned int            fmodVer = 0;                ///< FMOD version
    FMOD_CHANNELGROUP*      pChnGrp = nullptr;          ///< Our channel group that we place everything under
    bool                    bLowPass = false;           ///< Low Pass filter currently active?
public:
    /// Construtor, throws exception if FMOD sound system cannot be initialized
    SoundSystemFMOD();
    /// Destructor
    ~SoundSystemFMOD() override;
    
    /// @brief Loads a sound file so it becomes ready to play
    /// @note In this sound system, we can only deal with `WAV` files as we have to read the ourselves
    bool LoadSoundFile (const std::string& _sndName,
                        const std::string& _filePath, bool _bLoop,
                        float _coneDir, float _conePitch,
                        float _coneInAngle, float _coneOutAngle, float _coneOutVol) override;

    /// Use pre-v2 FMOD version structures?
    bool UsePreV2Fmod() { return fmodVer < 0x00020000; }
    /// Return the FMOD sound system
    FMOD_SYSTEM* GetSys() const { return pFmodSystem; }
    /// Return the FMOD version
    unsigned int GetVer() const { return fmodVer; }

    /// @brief Enables/disables FMOD logging
    /// @note FMOD Logging only works if linked to the `L` versions of the FMOD library
    void SoundLogEnable (bool bEnable = true);

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
    
    /// Any updates to be done at the end of the frame
    void Update () override;
    
    /// Set Master Volume on our channel group
    void SetMasterVolume (float volMaster) override;
    /// Mute all sounds (temporarily)
    void SetAllMute (bool bMute) override;
    
    /// Return list of possible audio devices
    bool GetAudioDeviceName (int i, std::string& devName) const override;
    /// Set a specific audio device as the output device, returns if it was found
    bool SetAudioDevice (int i) override;
    /// Get currently active audio device index
    int GetActiveAudioDevice () const override;

    /// Is the sound id available?
    bool IsValid (uint64_t sndId) override;
    
protected:
    /// Set low pass gain on all active sounds
    void SetLowPassGain (bool bOn);

    /// @brief Callback function for when sound ends, allows for internal bookkeeping
    static FMOD_RESULT F_CALLBACK ChnCB(FMOD_CHANNELCONTROL *channelcontrol,
                                        FMOD_CHANNELCONTROL_TYPE controltype,
                                        FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                                        void *commanddata1,
                                        void *commanddata2);
    
    /// @brief Callback function called by FMOD for logging purposes
    /// @note FMOD warns this can be called from any thread,
    ///       so strictly speaking we must not call `XPLMDebugString()`,
    ///       but so far it didn't hurt, and it should be debug builds only anyway.
    static FMOD_RESULT F_CALLBACK SoundLogCB(FMOD_DEBUG_FLAGS flags,
                                             const char * file,
                                             int line,
                                             const char * func,
                                             const char *message);

    /// @brief Helper functon to set FMOD settings
    /// @details Implemented as template function so it works with both
    ///          `FMOD_ADVANCEDSETTINGS` and `FMOD_10830_ADVANCEDSETTINGS`
    ///          for backwards compatibility with v1.8 of FMOD as used in XP11.
    template<class T_ADVSET>
    void SoundSetFmodSettings(T_ADVSET& advSet);
    
    /// Read SoundChannel from FMOD Channel's user data
    static uint64_t GetSndId (FMOD_CHANNEL* pFmodChn);
};

}

#endif // INCLUDE_FMOD_SOUND

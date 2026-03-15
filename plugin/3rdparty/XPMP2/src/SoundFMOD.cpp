/// @file       Sound.cpp
/// @brief      Sound for aircraft, based on the FMOD library
///
/// @note       If built with `INCLUDE_FMOD_SOUND=1` then
///             Audio Engine is FMOD Core API by Firelight Technologies Pty Ltd.
///             Understand FMOD [licensing](https://www.fmod.com/licensing) and
///             [attribution requirements](https://www.fmod.com/attribution) first!
///
/// @see        FMOD Core API Guide
///             https://fmod.com/docs/2.02/api/core-guide.html
/// @details    FMOD's C interface is used exclusively (and not the C++ ABI).
///             This is to ease handling of dynamic libaries between versions
///             (XP11 is using FMOD 1.x while XP12 has upgraded to 2.x)
///             and allows compiling with MingW.
/// @details    Some functionality looks like immitating FMOD's SoundGroup,
///             but 3D positioning was unstable when used via SoundGroup,
///             and the cone functionality did not work at all,
///             so this file handles all sound channels individually.
/// @note       If  linking to the logging version of the FMOD API library
///             (the one ending in `L`) and specifying a
///             config item `log_level` of `0 = Debug`
///             (see ::XPMPIntPrefsFuncTy) while initializing sound,
///             ie. typically during the first call to XPMPMultiplayerInit(),
///             then FMOD logging output is added to X-Plane's `Log.txt`.
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

#include "XPMP2.h"

// Only if we want Sound Support
#if INCLUDE_FMOD_SOUND + 0 >= 1

namespace XPMP2 {

//
// MARK: Global Variables and Types
//

constexpr int FMOD_NUM_VIRT_CHANNELS = 1000;        ///< Number of virtual channels during initialization
constexpr float FMOD_3D_MAX_DIST     = 10000.0f;    ///< Value used for 3D max distance, which doesn't have much of a function for the inverse roll-off model used here
constexpr float FMOD_LOW_PASS_GAIN   = 0.2f;        ///< Gain used when activating Low Pass filter

//
// MARK: Sound Files
//

// Constructor creates the FMOD sound object and initiates loading of the file
SoundFMOD::SoundFMOD (SoundSystemFMOD* _pSndSys,
                      const std::string& _filePath, bool _bLoop,
                      float _coneDir, float _conePitch,
                      float _coneInAngle, float _coneOutAngle, float _coneOutVol) :
SoundFile(_pSndSys, _filePath, _bLoop, _coneDir, _conePitch, _coneInAngle, _coneOutAngle, _coneOutVol)
{
    FMOD_TEST(FMOD_System_CreateSound(_pSndSys->GetSys(), filePath.c_str(),
                                      (bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) |
                                      FMOD_3D | FMOD_3D_WORLDRELATIVE | FMOD_3D_INVERSEROLLOFF |
                                      FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING,
                                      nullptr, &pSound));
    LOG_ASSERT(pSound);
}

// Destructor removes the sound object
SoundFMOD::~SoundFMOD()
{
    if (pSound)
        FMOD_Sound_Release(pSound);
    pSound = nullptr;
    bLoaded = false;
}

// Has loading the sound sample finished?
bool SoundFMOD::isReady ()
{
    // Cached values
    if (!pSound) return false;
    if (bLoaded) return true;
    
    // Otherwise query FMOD
    FMOD_OPENSTATE state = FMOD_OPENSTATE_LOADING;
    if ((FMOD_Sound_GetOpenState(pSound, &state, nullptr, nullptr, nullptr) == FMOD_OK) &&
        (state == FMOD_OPENSTATE_READY))
    {
        return bLoaded = true;
    }
    return false;
}

// Return the FMOD sound system
SoundSystemFMOD* SoundFMOD::GetFmodSys() const
{
    return dynamic_cast<SoundSystemFMOD*>(pSndSys);
}


//
// MARK: Sound System
//

// static pointer to myself
SoundSystemFMOD* SoundSystemFMOD::me = nullptr;

// Construtor, throws exception if XP sound system unavailable (prior to XP12.04)
SoundSystemFMOD::SoundSystemFMOD()
{
    /// FMOD 1.08.30 version of FMOD_ADVANCEDSETTINGS
    struct FMOD_10830_ADVANCEDSETTINGS
    {
        int                 cbSize;                     /* [w]   Size of this structure.  Use sizeof(FMOD_ADVANCEDSETTINGS)  NOTE: This must be set before calling System::getAdvancedSettings or System::setAdvancedSettings! */
        int                 maxMPEGCodecs;              /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  MPEG   codecs consume 22,216 bytes per instance and this number will determine how many MPEG   channels can be played simultaneously. Default = 32. */
        int                 maxADPCMCodecs;             /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  ADPCM  codecs consume  2,480 bytes per instance and this number will determine how many ADPCM  channels can be played simultaneously. Default = 32. */
        int                 maxXMACodecs;               /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  XMA    codecs consume  6,263 bytes per instance and this number will determine how many XMA    channels can be played simultaneously. Default = 32. */
        int                 maxVorbisCodecs;            /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  Vorbis codecs consume 16,512 bytes per instance and this number will determine how many Vorbis channels can be played simultaneously. Default = 32. */
        int                 maxAT9Codecs;               /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  AT9    codecs consume 20,664 bytes per instance and this number will determine how many AT9    channels can be played simultaneously. Default = 32. */
        int                 maxFADPCMCodecs;            /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_CREATECOMPRESSEDSAMPLE only.  FADPCM codecs consume  2,232 bytes per instance and this number will determine how many FADPCM channels can be played simultaneously. Default = 32. */
        int                 maxPCMCodecs;               /* [r/w] Optional. Specify 0 to ignore. For use with PS3 only.                          PCM    codecs consume  2,536 bytes per instance and this number will determine how many streams and PCM voices can be played simultaneously. Default = 32. */
        int                 ASIONumChannels;            /* [r/w] Optional. Specify 0 to ignore. Number of channels available on the ASIO device. */
        char              **ASIOChannelList;            /* [r/w] Optional. Specify 0 to ignore. Pointer to an array of strings (number of entries defined by ASIONumChannels) with ASIO channel names. */
        FMOD_SPEAKER       *ASIOSpeakerList;            /* [r/w] Optional. Specify 0 to ignore. Pointer to a list of speakers that the ASIO channels map to.  This can be called after System::init to remap ASIO output. */
        float               HRTFMinAngle;               /* [r/w] Optional.                      For use with FMOD_INIT_HRTF_LOWPASS.  The angle range (0-360) of a 3D sound in relation to the listener, at which the HRTF function begins to have an effect. 0 = in front of the listener. 180 = from 90 degrees to the left of the listener to 90 degrees to the right. 360 = behind the listener. Default = 180.0. */
        float               HRTFMaxAngle;               /* [r/w] Optional.                      For use with FMOD_INIT_HRTF_LOWPASS.  The angle range (0-360) of a 3D sound in relation to the listener, at which the HRTF function has maximum effect. 0 = front of the listener. 180 = from 90 degrees to the left of the listener to 90 degrees to the right. 360 = behind the listener. Default = 360.0. */
        float               HRTFFreq;                   /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_HRTF_LOWPASS.  The cutoff frequency of the HRTF's lowpass filter function when at maximum effect. (i.e. at HRTFMaxAngle).  Default = 4000.0. */
        float               vol0virtualvol;             /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_VOL0_BECOMES_VIRTUAL.  If this flag is used, and the volume is below this, then the sound will become virtual.  Use this value to raise the threshold to a different point where a sound goes virtual. */
        unsigned int        defaultDecodeBufferSize;    /* [r/w] Optional. Specify 0 to ignore. For streams. This determines the default size of the double buffer (in milliseconds) that a stream uses.  Default = 400ms */
        unsigned short      profilePort;                /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_PROFILE_ENABLE.  Specify the port to listen on for connections by the profiler application. */
        unsigned int        geometryMaxFadeTime;        /* [r/w] Optional. Specify 0 to ignore. The maximum time in miliseconds it takes for a channel to fade to the new level when its occlusion changes. */
        float               distanceFilterCenterFreq;   /* [r/w] Optional. Specify 0 to ignore. For use with FMOD_INIT_DISTANCE_FILTERING.  The default center frequency in Hz for the distance filtering effect. Default = 1500.0. */
        int                 reverb3Dinstance;           /* [r/w] Optional. Specify 0 to ignore. Out of 0 to 3, 3d reverb spheres will create a phyical reverb unit on this instance slot.  See FMOD_REVERB_PROPERTIES. */
        int                 DSPBufferPoolSize;          /* [r/w] Optional. Specify 0 to ignore. Number of buffers in DSP buffer pool.  Each buffer will be DSPBlockSize * sizeof(float) * SpeakerModeChannelCount.  ie 7.1 @ 1024 DSP block size = 8 * 1024 * 4 = 32kb.  Default = 8. */
        unsigned int        stackSizeStream;            /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD Stream thread in bytes.  Useful for custom codecs that use excess stack.  Default 49,152 (48kb) */
        unsigned int        stackSizeNonBlocking;       /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD_NONBLOCKING loading thread.  Useful for custom codecs that use excess stack.  Default 65,536 (64kb) */
        unsigned int        stackSizeMixer;             /* [r/w] Optional. Specify 0 to ignore. Specify the stack size for the FMOD mixer thread.  Useful for custom dsps that use excess stack.  Default 49,152 (48kb) */
        FMOD_DSP_RESAMPLER  resamplerMethod;            /* [r/w] Optional. Specify 0 to ignore. Resampling method used with fmod's software mixer.  See FMOD_DSP_RESAMPLER for details on methods. */
        unsigned int        commandQueueSize;           /* [r/w] Optional. Specify 0 to ignore. Specify the command queue size for thread safe processing.  Default 2048 (2kb) */
        unsigned int        randomSeed;                 /* [r/w] Optional. Specify 0 to ignore. Seed value that FMOD will use to initialize its internal random number generators. */
    };
    
    // Set pointer to myself
    me = this;
    
    // Enable FMOD logging
    if (glob.logLvl == logDEBUG)
        SoundLogEnable(true);
    
    // Create FMOD system and first of all determine its version,
    // which depends a bit if run under XP11 or XP12 and the OS we are on.
    // There are subtle difference in settings.
    FMOD_TEST(FMOD_System_Create(&pFmodSystem, FMOD_VERSION));
    FMOD_TEST(FMOD_System_GetVersion(pFmodSystem, &fmodVer));
    LOG_MSG(logINFO, "Initializing FMOD version %u.%u.%u",
            fmodVer >> 16, (fmodVer & 0x0000ff00) >> 8,
            fmodVer & 0x000000ff);
    FMOD_TEST(FMOD_System_Init(pFmodSystem, FMOD_NUM_VIRT_CHANNELS,
                               FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_CHANNEL_LOWPASS | FMOD_INIT_VOL0_BECOMES_VIRTUAL, nullptr));
    
    // Set advanced settings, which are version-dependend
    if (UsePreV2Fmod()) {
        FMOD_10830_ADVANCEDSETTINGS oldAdvSet;
        SoundSetFmodSettings(oldAdvSet);
    } else {
        FMOD_ADVANCEDSETTINGS advSet;
        SoundSetFmodSettings(advSet);
    }
    
    // The channel group we place all sounds under is the master channel group of this just created system
    FMOD_TEST(FMOD_System_GetMasterChannelGroup(pFmodSystem, &pChnGrp));
}

// Destructor releases the FMOD sound system
SoundSystemFMOD::~SoundSystemFMOD()
{
    if (pFmodSystem)
        FMOD_System_Release(pFmodSystem);
    SoundLogEnable(false);                  // Disable FMOD logging
    me = nullptr;
    pFmodSystem = nullptr;
    fmodVer = 0;
}

// Callback function called by FMOD for logging purposes
FMOD_RESULT F_CALLBACK SoundSystemFMOD::SoundLogCB(FMOD_DEBUG_FLAGS flags,
                                                   const char * file,
                                                   int line,
                                                   const char * func,
                                                   const char *message)
{
    // Basically convert it to our standard log output
    LogMsg(file, line, func,
           flags & FMOD_DEBUG_LEVEL_ERROR   ? logERR  :
           flags & FMOD_DEBUG_LEVEL_WARNING ? logWARN : logINFO,
           "FMOD_LOG: %s", message);
    return FMOD_OK;
}

// Enables/disables FMOD logging
void SoundSystemFMOD::SoundLogEnable (bool bEnable)
{
    gFmodRes = FMOD_Debug_Initialize(bEnable ? FMOD_DEBUG_LEVEL_LOG : FMOD_DEBUG_LEVEL_NONE,
                                     FMOD_DEBUG_MODE_CALLBACK, SoundLogCB, nullptr);
    if (gFmodRes == FMOD_OK) {
        LOG_MSG(logDEBUG, "FMOD logging has been %s.", bEnable ? "enabled" : "disabled");
    }
    // FMOD_ERR_UNSUPPORTED just means: not linked to fmodL, ie. "normal"
    else if (gFmodRes != FMOD_ERR_UNSUPPORTED)
    {
        FmodError("FMOD_Debug_Initialize", gFmodRes, __FILE__, __LINE__, __func__);
    }
}


// Loads a sound file so it becomes ready to play
bool SoundSystemFMOD::LoadSoundFile (const std::string& _sndName,
                                     const std::string& _filePath, bool _bLoop,
                                     float _coneDir, float _conePitch,
                                     float _coneInAngle,
                                     float _coneOutAngle, float _coneOutVol)
{
    try {
        // Create the sound file object, loading the file into memory
        SoundFilePtr p = std::make_unique<SoundFMOD>(this, _filePath, _bLoop,
                                                     _coneDir, _conePitch,
                                                     _coneInAngle,
                                                     _coneOutAngle, _coneOutVol);
        // Have the sound file object managed by our map of sounds
        mapSounds[_sndName] = std::move(p);
        LOG_MSG(logDEBUG, "Added%ssound '%s' from file '%s'",
                _bLoop ? " looping " : " ", _sndName.c_str(), _filePath.c_str());
        return true;
    }
    catch (const std::runtime_error& e) {
        LOG_MSG(logERR, "Could not load sound '%s' from file '%s': %s",
                _sndName.c_str(), _filePath.c_str(), e.what());
    }
    catch (...) {
        LOG_MSG(logERR, "Could not load sound '%s' from file '%s'",
                _sndName.c_str(), _filePath.c_str());
    }
    return false;
}



// Callback function for when sound ends, allows for internal bookkeeping
FMOD_RESULT F_CALLBACK SoundSystemFMOD::ChnCB(FMOD_CHANNELCONTROL *channelcontrol,
                                              FMOD_CHANNELCONTROL_TYPE controltype,
                                              FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                                              void *, void *)
{
    // We are only interested in "end of channel" events
    if (controltype != FMOD_CHANNELCONTROL_CHANNEL ||
        callbacktype != FMOD_CHANNELCONTROL_CALLBACK_END)
        return FMOD_OK;
    if (!me) return FMOD_ERR_INTERNAL;
    
    // The sound has stop, remove it from the tracking
    FMOD_CHANNEL* pFmodChn = (FMOD_CHANNEL*)channelcontrol;
    uint64_t sndId = GetSndId(pFmodChn);
    SoundChannel* pChn = me->GetChn(sndId);
    if (pChn) pChn->pChn = nullptr;                 // Sound has stopped already, no need to stop again during destruction
    me->RemoveChn(sndId);
    return FMOD_OK;
}


// Play a new sound, returns an id for that sound
uint64_t SoundSystemFMOD::Play (const std::string& sndName, float vol, const Aircraft& ac)
{
    FMOD_CHANNEL* pFmodChn = nullptr;
    uint64_t sndId = 0;
    
    // Find the sound a check if it is ready
    try {
        // find the sound, may throw if not found, return if sound isn't ready yet
        SoundFilePtr& pSnd = mapSounds.at(sndName);
        if (!pSnd->isReady()) throw std::runtime_error("Sound not yet ready");
        SoundFMOD* pSndFmod = dynamic_cast<SoundFMOD*>(pSnd.get());
        if (!pSndFmod) throw std::runtime_error("Sound has not been loaded for FMOD system");
        
        // Start playing the sound, but in a paused state to avoid crackling
        FMOD_TEST(FMOD_System_PlaySound(pFmodSystem, pSndFmod->GetSnd(), pChnGrp, true, &pFmodChn));
        if (!pFmodChn) throw std::runtime_error("FMOD_System_PlaySound returned NULL channel");
        
        // We must keep track of the sounds we produce so we can clean up after us
        SoundChannel* pChn = nullptr;
        std::tie(sndId, pChn) = AddChn(pSndFmod, vol, pFmodChn);
        if (!pChn) throw std::runtime_error("pChn is NULL");
        FMOD_TEST(FMOD_Channel_SetUserData(pFmodChn, (void*)sndId));// save internal sound id for later bookkeeping
        FMOD_TEST(FMOD_Channel_SetCallback(pFmodChn, ChnCB));       // set callback, this is essential for internal cleanup!
        
        // Set a few more parameters to the sound
        FMOD_LOG(FMOD_Channel_Set3DMinMaxDistance(pFmodChn, (float)ac.sndMinDist, FMOD_3D_MAX_DIST));
        if (pSndFmod->hasConeInfo()) {
            FMOD_LOG(FMOD_Channel_Set3DConeSettings(pFmodChn,
                                                    pSndFmod->coneInAngle,
                                                    pSndFmod->coneOutAngle,
                                                    pSndFmod->coneOutVol));
        }
        SetPosOrientation(sndId, ac, true);
        FMOD_LOG(FMOD_Channel_SetVolume(pFmodChn, vol));
        FMOD_LOG(FMOD_Channel_SetMute(pFmodChn, ac.SoundIsMuted()));
        if (bLowPass) {
            FMOD_LOG(FMOD_Channel_SetLowPassGain(pFmodChn, FMOD_LOW_PASS_GAIN));
        }
        
        // Success
        return sndId;
    }
    FMOD_CATCH
    catch (const std::runtime_error& e) {
        LOG_MSG(logERR, "Could not play sound '%s': %s",
                sndName.c_str(), e.what());
    }
    
    // So there was an error...we better remove the sound
    if (sndId)
        RemoveChn(sndId);
    
    return 0;
}

// Unpause a sound, which got started in a paused state to avoid crackling
void SoundSystemFMOD::Unpause (uint64_t sndId)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn || !pChn->pChn) return;
    
    if (pChn->ShallUnpause()) {
        FMOD_LOG(FMOD_Channel_SetPaused(pChn->pChn, false));
    }
}

// Stop the sound
void SoundSystemFMOD::Stop (uint64_t sndId)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn) return;
    
    // Stop the channel, will trigger callback, which will remove the sound from tracking
    if (pChn->pChn) {
        FMOD_LOG(FMOD_Channel_Stop(pChn->pChn));
    } else {
        RemoveChn(sndId);
    }
}

// Update sound's position and orientation
void SoundSystemFMOD::SetPosOrientation (uint64_t sndId, const Aircraft& ac, bool bDoOrientation)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn || !pChn->pChn || !pChn->pSnd ) return;
    
    // Aircraft position and velocity
    FMOD_VECTOR fmodPos {                           // Copy of the aircraft's 3D location
        ac.drawInfo.x,
        ac.drawInfo.y,
        ac.drawInfo.z,
    };
    FMOD_VECTOR fmodVel { ac.v_x, ac.v_y, ac.v_z }; // Copy of the aircraft's relative speed
    FMOD_LOG(FMOD_Channel_Set3DAttributes(pChn->pChn, &fmodPos, &fmodVel));
    
    // Do the (computationally more expensive) orientation stuff?
    if (!bDoOrientation ||
        !pChn->pSnd->hasConeInfo()) return;
    
    // Calculate current cone orientation based on standard orientation and aircraft orientation
    const float coneDirRad = deg2rad(pChn->pSnd->coneDir);
    FMOD_VECTOR coneVec = FmodHeadPitch2Vec(ac.GetHeading() + pChn->pSnd->coneDir,
                                            std::cos(coneDirRad)*ac.GetPitch() - std::sin(coneDirRad)*ac.GetRoll() + pChn->pSnd->conePitch);
    
    // Set cone info and orientation
    FMOD_LOG(FMOD_Channel_Set3DConeOrientation(pChn->pChn, &coneVec));
}

// Set sound's volume
void SoundSystemFMOD::SetVolume (uint64_t sndId, float vol)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn || !pChn->pChn) return;
    
    pChn->vol = vol;                        // Save its volume
    FMOD_LOG(FMOD_Channel_SetVolume(pChn->pChn, vol));
}

// Mute the sound (temporarily)
void SoundSystemFMOD::SetMute (uint64_t sndId, bool bMute)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn || !pChn->pChn) return;
    
    pChn->bMuted = bMute;                   // Save its mute status
    FMOD_LOG(FMOD_Channel_SetMute(pChn->pChn, bMute));
}

// Any updates to be done at the end of the frame (not for XP system)
void SoundSystemFMOD::Update ()
{
    // Update the listener position and orientation
    const FMOD_VECTOR posCam   = {
        glob.posCamera.x,
        glob.posCamera.y,
        glob.posCamera.z
    };
    const FMOD_VECTOR velocity  = { glob.vCam_x, glob.vCam_y, glob.vCam_z };
    
    // The forward direction takes heading, pitch, and roll into account
    FmodNormalizeHeadingPitch(glob.posCamera.heading, glob.posCamera.pitch);
    FMOD_VECTOR normForw;
    FMOD_VECTOR normUpw;
    FmodHeadPitchRoll2Normal(glob.posCamera.heading, glob.posCamera.pitch, glob.posCamera.roll, normForw, normUpw);
    
    // FMOD_ERR_INVALID_VECTOR used to be a problem, but should be no longer
    // Still, just in case it comes back, we log more details
    gFmodRes = FMOD_System_Set3DListenerAttributes(pFmodSystem, 0, &posCam, &velocity, &normForw, &normUpw);
    if (gFmodRes != FMOD_OK) {
        if (gFmodRes != FMOD_ERR_INVALID_VECTOR)
            FmodError("FMOD_System_Set3DListenerAttributes", gFmodRes, __FILE__, __LINE__, __func__).LogErr();
        else
        {
            static float lastInvVecErrMsgTS = -500.0f;
            FmodError("FMOD_System_Set3DListenerAttributes", gFmodRes, __FILE__, __LINE__, __func__).LogErr();
            if (GetMiscNetwTime() >= lastInvVecErrMsgTS + 300.0f) {
                lastInvVecErrMsgTS = GetMiscNetwTime();
                LOG_MSG(logERR, "Please report the following details as a reply to https://bit.ly/LTSound36");
                LOG_MSG(logERR, "Camera   roll=%.3f, heading=%.3f, pitch=%.3f",
                        glob.posCamera.roll, glob.posCamera.pitch, glob.posCamera.heading);
                LOG_MSG(logERR, "normForw x=%.6f, y=%.6f, z=%.6f", normForw.x, normForw.y, normForw.z);
                LOG_MSG(logERR, "normUpw  x=%.6f, y=%.6f, z=%.6f", normUpw.x, normUpw.y, normUpw.z);
            }
        }
    }
    
    // Set low pass gain in case we're in an internal view
    SetLowPassGain(!IsViewExternal());
    
    // Tell FMOD we're done
    FMOD_LOG(FMOD_System_Update(pFmodSystem));
}

// Set Master Volume on our channel group
void SoundSystemFMOD::SetMasterVolume (float volMaster)
{
    FMOD_LOG(FMOD_ChannelGroup_SetVolume(pChnGrp, volMaster));
}

// Mute all sounds (temporarily)
void SoundSystemFMOD::SetAllMute (bool bMute)
{
    FMOD_LOG(FMOD_ChannelGroup_SetMute(pChnGrp, bMute));
}

// Return list of possible audio devices
bool SoundSystemFMOD::GetAudioDeviceName (int i, std::string& devName) const
{
    // Get number of drivers first
    int numDevices = 0;
    if (pFmodSystem && (FMOD_System_GetNumDrivers(pFmodSystem, &numDevices) == FMOD_OK))
    {
        // Then the individual name
        if (i < numDevices)
        {
            char name[256];
            if (FMOD_System_GetDriverInfo(pFmodSystem, i, name, sizeof(name), nullptr, nullptr, nullptr, nullptr) == FMOD_OK)
            {
                devName = name;
                return true;
            }
        }
    }
    return false;
}

// Set a specific audio device as the output device, returns if it was found
bool SoundSystemFMOD::SetAudioDevice (int i)
{
    // Get number of drivers first
    int numDevices = 0;
    if (pFmodSystem && (FMOD_System_GetNumDrivers(pFmodSystem, &numDevices) == FMOD_OK))
    {
        if (i < numDevices) {
            FMOD_LOG(FMOD_System_SetDriver(pFmodSystem, i));
            return true;
        }
    }
    return false;
}

// Get currently active audio device index
int SoundSystemFMOD::GetActiveAudioDevice () const
{
    int activeDev = 0;
    if (pFmodSystem) {
        FMOD_LOG(FMOD_System_GetDriver(pFmodSystem, &activeDev));
    }
    return activeDev;
}


// Set low pass gain on all active sounds
void SoundSystemFMOD::SetLowPassGain (bool bOn)
{
    // no change?
    if (bLowPass == bOn) return;
    bLowPass = bOn;
    
    // Setting LowPassGain on channel group-level didn't do the trick,
    // so we iterate all sounds we know and set it individually
    for (auto& p: mapChn) {
        SoundChannel& chn = p.second;
        if (chn.pChn) {
            FMOD_LOG(FMOD_Channel_SetLowPassGain(chn.pChn, bOn ? FMOD_LOW_PASS_GAIN : 1.0f));
        }
    }
}

// Is the sound id available?
bool SoundSystemFMOD::IsValid (uint64_t sndId)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn) return false;

    FMOD_BOOL bPlaying = false;
    FMOD_LOG(FMOD_Channel_IsPlaying(pChn->pChn, &bPlaying));
    return bPlaying;
}

// Helper functon to set FMOD settings
template<class T_ADVSET>
void SoundSystemFMOD::SoundSetFmodSettings(T_ADVSET& advSet)
{
    memset(&advSet, 0, sizeof(advSet));
    advSet.cbSize = sizeof(advSet);
    advSet.vol0virtualvol = 0.01f;     // set some low volume for fading to 0
    FMOD_TEST(FMOD_System_SetAdvancedSettings(pFmodSystem, (FMOD_ADVANCEDSETTINGS*)&advSet));
}

// Read SoundChannel from FMOD Channel's user data
uint64_t SoundSystemFMOD::GetSndId (FMOD_CHANNEL* pFmodChn)
{
    void* pVoid = nullptr;
    FMOD_LOG(FMOD_Channel_GetUserData(pFmodChn, &pVoid));
    return (uint64_t)pVoid;
}

} // namespace XPMP2

#endif // INCLUDE_FMOD_SOUND

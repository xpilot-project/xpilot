/// @file       Sound.cpp
/// @brief      Sound for aircraft
///
/// @details    As of X-Plane 12.04, X-Plane's SDK offers an interface into
///             X-Plane's FMOD system. If available these functions are loaded
///             dynamically (to stay compatible to XP11).
///             If not available then either FMOD can be used directly if
///             built including FMOD (see SoundFMOD.cpp) or no sound is available.
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

#include "XPMP2.h"

namespace XPMP2 {

FMOD_VECTOR FmodHeadPitch2Vec (const float head, const float pitch);

//
// MARK: Global Variables and Types
//

/// The sound system in use
SoundSystem* gpSndSys = nullptr;

/// last FMOD result code, needed by the following macro, better don't rely on anywhere else
FMOD_RESULT gFmodRes = FMOD_OK;

constexpr float FMOD_3D_MAX_DIST    = 10000.0f;     ///< Value used for 3D max distance, which doesn't have much of a function for the inverse roll-off model used here
constexpr int EXP_COMP_SKIP_CYCLES  = 10;           ///< In how many cycles to skip expensive computations?

/// Definition of how sound is handled based on dataRef values (type)
struct SoundDefTy {
    float (Aircraft::* pVal)() const = nullptr;     ///< Function return the value to observe, typically a dataRef value
    bool bSndLoop = true;                           ///< Sound to be played in a loop while value is large than `valMin`? Otherwise a single sound upon detection of a change of value
    float valMin = NAN;                             ///< Sound to be played if `*pVal > valMin`
    float valMax = NAN;                             ///< Only used to control volume, which raises for `*pVal` between `valMin` and `valMax`
};

/// Definition of how sound is handled based on dataRef values
static SoundDefTy gaSoundDef[Aircraft::SND_NUM_EVENTS] = {
    { &Aircraft::GetThrustRatio,        true, 0.0f,  1.0f },    // SND_ENG
    { &Aircraft::GetThrustReversRatio,  true, 0.0f,  1.0f },    // SND_REVERSE_THRUST
    { &Aircraft::GetTireRotRpm,         true, 60.0f, 1000.0f }, // SND_TIRE
    { &Aircraft::GetGearRatio,          false, NAN, NAN },      // SND_GEAR
    { &Aircraft::GetFlapRatio,          false, NAN, NAN },      // SND_FLAPS
};

//
// MARK: X-Plane 12 SDK 400 functions to be loaded dynaimcally
//

// In SDK400, these are defined in XPLMSound.h:
typedef XPLM_API FMOD_CHANNEL* (f_XPLMPlayPCMOnBus)(
                                void *               audioBuffer,
                                uint32_t             bufferSize,
                                FMOD_SOUND_FORMAT    soundFormat,
                                int                  freqHz,
                                int                  numChannels,
                                int                  loop,
                                XPLMAudioBus         audioType,
                                XPLMPCMComplete_f    inCallback,
                                void *               inRefcon);
typedef XPLM_API FMOD_RESULT (f_XPLMStopAudio)(
                                FMOD_CHANNEL*        fmod_channel);
typedef XPLM_API FMOD_RESULT (f_XPLMSetAudioPosition)(
                                FMOD_CHANNEL*        fmod_channel,
                                FMOD_VECTOR*         position,
                                FMOD_VECTOR*         velocity);
typedef XPLM_API FMOD_RESULT (f_XPLMSetAudioFadeDistance)(
                                FMOD_CHANNEL*        fmod_channel,
                                float                min_fade_distance,
                                float                max_fade_distance);
typedef XPLM_API FMOD_RESULT (f_XPLMSetAudioVolume)(
                                FMOD_CHANNEL*        fmod_channel,
                                float                source_volume);
typedef XPLM_API FMOD_RESULT (f_XPLMSetAudioCone)(
                                FMOD_CHANNEL*        fmod_channel,
                                float                inside_angle,
                                float                outside_angle,
                                float                outside_volume,
                                FMOD_VECTOR*         orientation);

f_XPLMPlayPCMOnBus*         gpXPLMPlayPCMOnBus          = nullptr;
f_XPLMStopAudio*            gpXPLMStopAudio             = nullptr;
f_XPLMSetAudioPosition*     gpXPLMSetAudioPosition      = nullptr;
f_XPLMSetAudioFadeDistance* gpXPLMSetAudioFadeDistance  = nullptr;
f_XPLMSetAudioVolume*       gpXPLMSetAudioVolume        = nullptr;
f_XPLMSetAudioCone*         gpXPLMSetAudioCone          = nullptr;

/// Are XP12 sound functions available?
inline bool XPSoundIsAvail () { return gpXPLMSetAudioCone != nullptr; }

/// Tries to find all the new XP12 sound functions, available as of XP12.04
bool XPSoundLoadFctPtr ()
{
#define LOAD_XP_FUNC(FCT)                               \
    gp ## FCT = (f_ ## FCT*) XPLMFindSymbol (#FCT);     \
    if (!gp ## FCT) return false;
    
    LOAD_XP_FUNC(XPLMPlayPCMOnBus);
    LOAD_XP_FUNC(XPLMStopAudio);
    LOAD_XP_FUNC(XPLMSetAudioPosition);
    LOAD_XP_FUNC(XPLMSetAudioFadeDistance);
    LOAD_XP_FUNC(XPLMSetAudioVolume);
    LOAD_XP_FUNC(XPLMSetAudioCone);         // do XPLMSetAudioCone last, this is used in `XPSoundIsAvail`
    LOG_MSG(logINFO, "X-Plane's FMOD system available");
    return true;
}

//
// MARK: Sound Files
//

SoundFile::SoundFile (SoundSystem* _pSndSys,
                      const std::string& _filePath, bool _bLoop,
                      float _coneDir, float _conePitch,
                      float _coneInAngle, float _coneOutAngle, float _coneOutVol) :
    pSndSys(_pSndSys), filePath(_filePath), bLoop(_bLoop),
    coneDir(_coneDir), conePitch(_conePitch),
    coneInAngle(_coneInAngle), coneOutAngle(_coneOutAngle), coneOutVol(_coneOutVol)
{}
    
// Constructor, will load immediately and throw an exception if loading is unsuccessful
SoundWAV::SoundWAV (SoundSystem* _pSndSys,
                    const std::string& _filePath, bool _bLoop,
                    float _coneDir, float _conePitch,
                    float _coneInAngle, float _coneOutAngle, float _coneOutVol) :
SoundFile(_pSndSys, _filePath, _bLoop, _coneDir, _conePitch,
          _coneInAngle, _coneOutAngle, _coneOutVol)
{
    // Directly try to read the WAV file, will throw exceptions if it doesn't work
    WavRead();
}

// Destructor frees up the memory of the PCM16 data
SoundWAV::~SoundWAV()
{
    if (pBuf) {
        free(pBuf);
        pBuf = nullptr;
    }
}

// Reads a WAV file into a PCM16 memory buffer, throws exceptions in case of errors
void SoundWAV::WavRead ()
{
    // Header of a WAV file
    struct WavHeaderTy {
        uint32_t ChunkID;           // "RIFF"
        uint32_t ChunkSize;
        uint32_t Format;            // "WAVE"
    } wavHdr;
    
    // Structure of the 'fmt' chunk
    struct WavFmtTy {
        uint16_t AudioFormat;
        uint16_t NumChannels;
        uint32_t SampleRate;
        uint32_t ByteRate;
        uint16_t BlockAlign;
        uint16_t BitsPerSample;
    } wavFmt;
    
    // Beginning of a chunk
    struct WavChunkTy {
        uint32_t id;
        uint32_t size;
    } wavChnk;
    
    // Encapsulate the file just to properly close on any exit
    struct FileTy {
        FILE* f = nullptr;
        ~FileTy () {
            if (f) fclose(f);
        }
    } f;
    
    // Open the file in binary mode for reading
    f.f = fopen(filePath.c_str(), "rb");
    if (!f.f) throw std::runtime_error(std::strerror(errno));
    
    // Verify Header signatures
    if (fread(&wavHdr, sizeof(wavHdr), 1, f.f) != 1)
        throw std::runtime_error("Couldn't read WAV header");
    // Should be a RIFF/WAV file
    if (wavHdr.ChunkID       != htonl(0x52494646) ||    // "RIFF"
        wavHdr.Format        != htonl(0x57415645))      // "WAVE"
        throw std::runtime_error("File does not appear to have WAV format");
    
    // Skip over any non-fmt chunks to reach the 'fmt' chunk
    while(1) {
        if (fread(&wavChnk, sizeof(wavChnk), 1, f.f) != 1)
            throw std::runtime_error("Couldn't find 'fmt' chunk");
        if (wavChnk.id == htonl(0x666d7420))            // "fmt "
            break;
        if (fseek(f.f, long(wavChnk.size), SEEK_CUR) != 0)
            throw std::runtime_error("Couldn't skip over non-fmt chunk, fseek failed");
    }
    
    // read the 'fmt' chunk
    if (fread(&wavFmt, sizeof(wavFmt), 1, f.f) != 1)
        throw std::runtime_error("Couldn't read 'fmt' chunk");
    // If the "fmt" chunk wasn't the expected 16 bytes we need to skip over the additional ones
    if (wavChnk.size > 16) {
        if (fseek(f.f, long(wavChnk.size - 16), SEEK_CUR) != 0)
            throw std::runtime_error("Couldn't skip over additional 'fmt' data, fseek failed");
    }
    
    // Save some header attributes
    freqHz      = (int)wavFmt.SampleRate;
    numChannels = (int)wavFmt.NumChannels;
    
    // Loop over all following chunks, skipping everything that is not "data"
    int numDataChunks = 0;
    while (!feof(f.f) && !ferror(f.f))
    {
        if (fread(&wavChnk, sizeof(wavChnk), 1, f.f) != 1) {
            // Just end-of-file after we read data? That's perfectly OK!
            if (feof(f.f) && numDataChunks)
                return;
            // Otherwise...not good
            throw std::runtime_error("Couldn't read subsequent chunk header");
        }
        
        // Skip over non-data chunks
        if (wavChnk.id != htonl(0x64617461)) {          // "data"
            if (fseek(f.f, long(wavChnk.size), SEEK_CUR) != 0)
                throw std::runtime_error("Couldn't skip over non-data chunk, fseek failed");
            continue;
        }
        
        // -- Process data chunk --

        // How many samples will we have? (across all channels)
        const uint32_t numSamples = wavChnk.size / (wavFmt.BitsPerSample/8);
        // Add to already existing buffer or create a new one?
        int16_t* pWritePos = nullptr;
        if (pBuf) {
            const uint32_t oldNumSamples = bufferSize / sizeof(int16_t);
            bufferSize += numSamples * sizeof(int16_t);
            pBuf = (int16_t*)realloc(pBuf, bufferSize);
            pWritePos = pBuf + oldNumSamples;
        }
        else {
            // Create a buffer of the required size
            bufferSize = numSamples * sizeof(int16_t);
            pWritePos = pBuf = (int16_t*)malloc(bufferSize);
        }
        if (!pBuf || !pWritePos) throw std::runtime_error("Could not allocate memory for PCM16 data");
        
        // If the WAV file is originally in PCM16 data, then read directly to the target buffer
        if (wavFmt.AudioFormat == 1 && wavFmt.BitsPerSample == 16) {
            size_t numRead = fread(pWritePos, sizeof(int16_t), numSamples, f.f);
            if (numRead != numSamples)
                throw std::runtime_error("Less PCM16 samples than expected");
        }

        // Convert PCM8 data (seem to be unsigned data with 0x80 = 0)
        else if (wavFmt.AudioFormat == 1 && wavFmt.BitsPerSample == 8) {
            uint8_t iVal;
            for (uint32_t i = 0; i < numSamples; ++i) {
                if (fread(&iVal, sizeof(iVal), 1, f.f) != 1)
                    throw std::runtime_error("Less PCM8 samples than expected");
                pWritePos[i] = (int16_t(iVal) - 0x0080) * 0x0100;
            }
        }

        // Convert IEEE32 (float)
        else if (wavFmt.AudioFormat == 3 && wavFmt.BitsPerSample == 32) {
            float fVal;
            static_assert(sizeof(float) == 4, "'float' isn't 32bits");
            for (uint32_t i = 0; i < numSamples; ++i) {
                if (fread(&fVal, sizeof(fVal), 1, f.f) != 1)
                    throw std::runtime_error("Less IEEE32 samples than expected");
                pWritePos[i] = (int16_t)std::lroundf(fVal * INT16_MAX);
            }
        }

        // Convert IEEE64 (double)
        else if (wavFmt.AudioFormat == 3 && wavFmt.BitsPerSample == 64) {
            double dVal;
            static_assert(sizeof(double) == 8, "'double' isn't 64bits");
            for (uint32_t i = 0; i < numSamples; ++i) {
                if (fread(&dVal, sizeof(dVal), 1, f.f) != 1)
                    throw std::runtime_error("Less IEEE64 samples than expected");
                pWritePos[i] = (int16_t)std::lround(dVal * INT16_MAX);
            }
        }

        // Unknown
        else {
            LOG_MSG(logERR, "Unknown WAV sample format: Audio Format = %u, Bits per Sample = %u, in %s",
                    wavFmt.AudioFormat, wavFmt.BitsPerSample, filePath.c_str());
            throw std::runtime_error("Unknown WAV sample format");
        }
        ++numDataChunks;                        // loaded (one more) 'data' chunk successfully
    }
    
    // if we get here then we didn't read data!
    throw std::runtime_error("Unknown error occured before reading WAV data");
}


/// @brief Load fixed set of X-Plane-internal sounds
/// @returns Number of loaded sounds
int SoundLoadXPSounds ()
{
    int n = 0;
#define ADD_SND(s,f,l) if (!XPMPSoundAdd(s,f,l)[0]) ++n;
#define ADD_CONE(s,f,l,cd,cp,cia,coa,cov) if (!XPMPSoundAdd(s,f,l,cd,cp,cia,coa,cov)[0]) ++n;
    // Engine sounds
    ADD_SND(XP_SOUND_ELECTRIC,          "Resources/sounds/engine/ENGINE_ELECTRIC_out.wav",          true);
    // The two jet sounds define a sound cone: 180 degrees heading (ie. backwards), no pitch, with 30 degree inner angle (full sound), and 60 degree outer angle (reduced sound), with sound reduction to 50% outside the outer angle:
    ADD_CONE(XP_SOUND_HIBYPASSJET,      "Resources/sounds/engine/ENGINE_HI_BYPASS_JET_out.wav",     true, 180.0f, 0.0f, 30.0f, 60.0f, 0.5f);
    ADD_CONE(XP_SOUND_LOBYPASSJET,      "Resources/sounds/engine/ENGINE_LO_BYPASS_JET_out.wav",     true, 180.0f, 0.0f, 30.0f, 60.0f, 0.5f);
    ADD_SND(XP_SOUND_TURBOPROP,         "Resources/sounds/engine/ENGINE_TURBOPROP_out.wav",         true);
    ADD_SND(XP_SOUND_PROP_AIRPLANE,     "Resources/sounds/engine/PROPELLER_OF_AIRPLANE_out.wav",    true);
    ADD_SND(XP_SOUND_PROP_HELI,         "Resources/sounds/engine/PROPELLER_OF_HELO_out.wav",        true);
    ADD_SND(XP_SOUND_REVERSE_THRUST,    "Resources/sounds/engine/REVERSE_THRUST_out.wav",           true);
    
    // Rolling on the ground
    ADD_SND(XP_SOUND_ROLL_RUNWAY,       "Resources/sounds/contact/roll_runway.wav",                 true);
    
    // One-time sounds
    ADD_SND(XP_SOUND_FLAP,              "Resources/sounds/systems/flap.wav",                        false);
    ADD_SND(XP_SOUND_GEAR,              "Resources/sounds/systems/gear.wav",                        false);
    return n;
}

//
// MARK: Sound System
//

// Enumerate all loaded sounds
const char* SoundSystem::EnumerateSounds (const char* prevName, const char** ppFilePath)
{
    if (ppFilePath) *ppFilePath = nullptr;

    // No sounds available at all?
    if (mapSounds.empty())
        return nullptr;
    
    auto sndIter = mapSounds.end();
    // Return first sound?
    if (!prevName || !prevName[0])
        sndIter = mapSounds.begin();
    else {
        // Try finding the given `prevName` sound, then return next
        sndIter = mapSounds.find(prevName);
        if (sndIter != mapSounds.end())
            sndIter++;
    }
    
    // Not found anything anymore?
    if (sndIter == mapSounds.end()) {
        return nullptr;
    } else {
        // Return the found element
        if (ppFilePath && sndIter->second)
            *ppFilePath = sndIter->second->filePath.c_str();
        return sndIter->first.c_str();
    }
}

// Is the sound id available?
bool SoundSystem::IsValid (uint64_t sndId)
{
    return mapChn.find(sndId) != mapChn.end();
}

// Add one more channel, returning the newly created id
std::pair<uint64_t,SoundChannel*> SoundSystem::AddChn (SoundFile* pSnd, float vol, FMOD_CHANNEL* pChn)
{
    ++uNxtId;
    auto i = mapChn.emplace(std::piecewise_construct,
                            std::forward_as_tuple(uNxtId),
                            std::forward_as_tuple(pChn, pSnd, vol));
    return std::make_pair(uNxtId, &i.first->second);
}

// Return the SoundChannel object for a given id, or `nullptr` if not found
SoundChannel* SoundSystem::GetChn (uint64_t sndId)
{
    // check cached value first
    if (cacheSndId == sndId && pCacheChn)
        return pCacheChn;
    // search in map of channels
    auto i = mapChn.find(sndId);
    if (i != mapChn.end()) {
        cacheSndId = sndId;                 // fill cache
        return pCacheChn = &i->second;
    }
    else
        return nullptr;
}

// Remove a channel from out tracking
void SoundSystem::RemoveChn (uint64_t sndId)
{
    if (cacheSndId == sndId)                // clear cache
        pCacheChn = nullptr;
    mapChn.erase(sndId);
}





// Constructor, throws exception if XP sound system unavailable (prior to XP12.04)
SoundSystemXP::SoundSystemXP()
{
    // find pointers to XP12's sound functions
    if (!XPSoundLoadFctPtr())
        throw std::runtime_error("X-Plane Sound functions unavailable (not XP12.04 or higher?)");
}

// Loads a sound file so it becomes ready to play
bool SoundSystemXP::LoadSoundFile (const std::string& _sndName,
                                   const std::string& _filePath, bool _bLoop,
                                   float _coneDir, float _conePitch,
                                   float _coneInAngle,
                                   float _coneOutAngle, float _coneOutVol)
{
    try {
        // Create the sound file object, loading the file into memory
        SoundFilePtr p = std::make_unique<SoundWAV>(this, _filePath, _bLoop,
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


// Play a new sound
uint64_t SoundSystemXP::Play (const std::string& sndName, float vol, const Aircraft& ac)
{
    // Safety checks
    if (!XPSoundIsAvail()) return 0;
    
    try {
        // find the sound, may throw if not found, return if sound isn't ready yet
        SoundFilePtr& pSnd = mapSounds.at(sndName);
        if (!pSnd->isReady()) throw std::runtime_error("Sound not yet ready");
        SoundWAV* pSndWav = dynamic_cast<SoundWAV*>(pSnd.get());
        if (!pSndWav) throw std::runtime_error("Sound does not have WAV format");

        // We must keep track of the sounds we produce so we can clean up after us
        uint64_t sndId = 0;
        SoundChannel* pChn = nullptr;
        std::tie(sndId, pChn) = AddChn(pSndWav, vol);
        if (!pChn) throw std::runtime_error("pChn is NULL");
        
        // Actually play the sound
        pChn->pChn = gpXPLMPlayPCMOnBus(pSndWav->pBuf, pSndWav->bufferSize,
                                        FMOD_SOUND_FORMAT_PCM16,
                                        pSndWav->freqHz, pSndWav->numChannels,
                                        pSndWav->bLoop,
                                        xplm_AudioExteriorEnvironment,
                                        PlayCallback, (void*)sndId);
        if (!pChn->pChn) throw std::runtime_error("XPLMPlayPCMOnBus return NULL");
#if INCLUDE_FMOD_SOUND + 0 >= 1
        // if we have FMOD available then start in a paused state to avoid crackling
        FMOD_LOG(FMOD_Channel_SetPaused(pChn->pChn, true));
#else
        pChn->nPauseCountdown = 0;
#endif

        // Set a few more parameters to the sound
        FMOD_LOG(gpXPLMSetAudioFadeDistance(pChn->pChn, (float)ac.sndMinDist, FMOD_3D_MAX_DIST));
        SetPosOrientation(sndId, ac, true);
        ChnSetVol(*pChn);
        
        return sndId;
    }
    catch (const std::out_of_range&) {
        LOG_MSG(logERR, "Sound '%s' not found, cannot play",
                sndName.c_str());
    }
    catch (const std::runtime_error& e) {
        LOG_MSG(logERR, "Could not play sound '%s': %s",
                sndName.c_str(), e.what());
    }
    return 0;
}


// Callback required by XPLMPlayPCMOnBus
void SoundSystemXP::PlayCallback (void*         inRefcon,
                                  FMOD_RESULT   status)
{
    // Sanity checks
    SoundSystemXP* me = dynamic_cast<SoundSystemXP*>(gpSndSys);
    if (!me) {
        LOG_MSG(logERR, "No XP Sound system in use!");
        return;
    }
    
    // Log any issue there might have been
    uint64_t sndId = uint64_t(inRefcon);
    if (status != FMOD_OK) {
        const SoundChannel* pChn = me->GetChn(sndId);
        LOG_MSG(logERR, "XPLMPlayPCMOnBus for sound %lu/'%s' caused FMOD error %d",
                (unsigned long)sndId,
                pChn && pChn->pSnd ? pChn->pSnd->filePath.c_str() : "<NULL>",
                status);
    }
    
    // Playback has ended, remove the sound channel entry
    me->RemoveChn(sndId);
}

// Unpause a sound, which got started in a paused state to avoid crackling
/// @note Only available if built with FMOD library
void SoundSystemXP::Unpause ([[maybe_unused]] uint64_t sndId)
{
#if INCLUDE_FMOD_SOUND + 0 >= 1
    SoundChannel* pChn = GetChn(sndId);
    if (!pChn || !pChn->pChn) return;

    if (pChn->ShallUnpause()) {
        FMOD_LOG(FMOD_Channel_SetPaused(pChn->pChn, false));
    }
#endif
}
    

// Stop the sound
void SoundSystemXP::Stop (uint64_t sndId)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!gpXPLMStopAudio || !pChn || !pChn->pChn) return;
    if (!pChn) return;

    // Stop the channel (callback will be called and will remove tracking)
    if (gpXPLMStopAudio && pChn->pChn) {
        FMOD_LOG(gpXPLMStopAudio(pChn->pChn))
    } else {
        RemoveChn(sndId);
    }
}

// Update sound's position
void SoundSystemXP::SetPosOrientation (uint64_t sndId, const Aircraft& ac, bool bDoOrientation)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!XPSoundIsAvail() || !pChn || !pChn->pChn || !pChn->pSnd ) return;
    
    // Aircraft position and velocity
    FMOD_VECTOR fmodPos {                           // Copy of the aircraft's 3D location
        ac.drawInfo.x,
        ac.drawInfo.y,
        ac.drawInfo.z,
    };
    FMOD_VECTOR fmodVel { ac.v_x, ac.v_y, ac.v_z }; // Copy of the aircraft's relative speed
    FMOD_LOG(gpXPLMSetAudioPosition(pChn->pChn, &fmodPos, &fmodVel));
    
    // Do the (computationally more expensive) orientation stuff?
    if (!bDoOrientation ||
        !pChn->pSnd->hasConeInfo()) return;
    
    // Calculate current cone orientation based on standard orientation and aircraft orientation
    const float coneDirRad = deg2rad(pChn->pSnd->coneDir);
    FMOD_VECTOR coneVec = FmodHeadPitch2Vec(ac.GetHeading() + pChn->pSnd->coneDir,
                                            std::cos(coneDirRad)*ac.GetPitch() - std::sin(coneDirRad)*ac.GetRoll() + pChn->pSnd->conePitch);

    // Validate cone parameters before calling gpXPLMSetAudioCone to prevent FMOD error 29
    if (std::isfinite(pChn->pSnd->coneInAngle) && 
        std::isfinite(pChn->pSnd->coneOutAngle) && 
        std::isfinite(pChn->pSnd->coneOutVol) &&
        pChn->pSnd->coneInAngle >= 0.0f && 
        pChn->pSnd->coneOutAngle >= 0.0f &&
        pChn->pSnd->coneInAngle <= 360.0f && 
        pChn->pSnd->coneOutAngle <= 360.0f &&
        pChn->pSnd->coneOutVol >= 0.0f && 
        pChn->pSnd->coneOutVol <= 1.0f &&
        std::isfinite(coneVec.x) && 
        std::isfinite(coneVec.y) && 
        std::isfinite(coneVec.z)) {
        // Set cone info and orientation
        FMOD_LOG(gpXPLMSetAudioCone(pChn->pChn,
                                    pChn->pSnd->coneInAngle,
                                    pChn->pSnd->coneOutAngle,
                                    pChn->pSnd->coneOutVol,
                                    &coneVec));
    } else {
        // Log the invalid cone parameters to help debug the issue
        //LOG_MSG(logWARN, "Skipping audio cone setting due to invalid parameters: "
        //        "InAngle=%.2f, OutAngle=%.2f, OutVol=%.2f, Vec=(%.2f,%.2f,%.2f)",
         //       pChn->pSnd->coneInAngle, pChn->pSnd->coneOutAngle, pChn->pSnd->coneOutVol,
          //      coneVec.x, coneVec.y, coneVec.z);
    }
}

// Set sound's volume
void SoundSystemXP::SetVolume (uint64_t sndId, float vol)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!gpXPLMSetAudioVolume || !pChn || !pChn->pChn) return;
    
    pChn->vol = vol;                        // Save its volume
    ChnSetVol(*pChn);                       // Set the channel's volume
}

// Mute the sound (temporarily)
void SoundSystemXP::SetMute (uint64_t sndId, bool bMute)
{
    SoundChannel* pChn = GetChn(sndId);
    if (!gpXPLMSetAudioVolume || !pChn || !pChn->pChn) return;

    pChn->bMuted = bMute;                   // Save its mute status
    ChnSetVol(*pChn);                       // Set the channel's volume
}

// Set Master Volume, effectively a multiplicator to SetVolume()
void SoundSystemXP::SetMasterVolume (float v)
{
    volMaster = v;                          // save master volume internally
    if (bAllMuted) return;                  // no need to touch volume if currently muted
    AllChnSetVol();                         // Otherwise update all channels
}

// Mute all sounds (temporarily)
void SoundSystemXP::SetAllMute (bool bMute)
{
    bAllMuted = bMute;                      // save the global mute status
    AllChnSetVol();                         // update all channels
}

// Return list of possible audio devices: For X-Plane device, we don't interfere and only return "X-Plane"
bool SoundSystemXP::GetAudioDeviceName (int i, std::string& devName) const
{
    if (i == 0) {
        devName = "X-Plane";
        return true;
    }
    return false;
}

/// Set a specific audio device as the output device: For X-Plane, we only support "X-Plane" (0) and don't actually interfere with X-Plane's output control
bool SoundSystemXP::SetAudioDevice (int i)
{
    return i == 0;
}

// Get currently active audio device index
int SoundSystemXP::GetActiveAudioDevice () const
{
    return 0;
}


// Update an individual channel's volume
void SoundSystemXP::ChnSetVol (const SoundChannel& chn)
{
    if (!chn.pChn) return;
    FMOD_LOG(gpXPLMSetAudioVolume(chn.pChn,
                                  chn.bMuted || bAllMuted ? 0.0f : (chn.vol * volMaster)));
}

// Update all channels' volume
void SoundSystemXP::AllChnSetVol ()
{
    for (const auto& i: mapChn)
        ChnSetVol(i.second);
}


//
// MARK: Local Functions
//

/// Return a text for the values of Aircraft::SoundEventsTy
const char* SoundEventTxt (Aircraft::SoundEventsTy e)
{
    switch (e) {
        case Aircraft::SND_ENG:             return "Engine";
        case Aircraft::SND_REVERSE_THRUST:  return "Reverse Thrust";
        case Aircraft::SND_TIRE:            return "Tires";
        case Aircraft::SND_GEAR:            return "Gear";
        case Aircraft::SND_FLAPS:           return "Flaps";
        default:
            return "<unknown>";
    }
}

// Convert heading/pitch to normalized x/y/z vector
FMOD_VECTOR FmodHeadPitch2Vec (const float head, const float pitch)
{
    const std::valarray<float> v = HeadPitch2Vec(head, pitch);
    assert(v.size() == 3);
    return FMOD_VECTOR { v[0], v[1], v[2] };
}

// Convert heading/pitch to normal x/y/z vector
void FmodHeadPitchRoll2Normal(const float head, const float pitch, const float roll,
                              FMOD_VECTOR& vecDir, FMOD_VECTOR& vecNorm)
{
    const std::valarray<float> v = HeadPitchRoll2Normal(head, pitch, roll);
    assert(v.size() == 6);
    vecDir.x  = v[0];
    vecDir.y  = v[1];
    vecDir.z  = v[2];
    vecNorm.x = v[3];
    vecNorm.y = v[4];
    vecNorm.z = v[5];
}

// Normalize Pitch/Heading
void FmodNormalizeHeadingPitch(float& head, float& pitch)
{
    // pitch "over the top"
    if (pitch > 90.0f) {
        pitch = 180.0f - pitch;
        head += 180.0f;
    }
    if (pitch < -90.0f) {
        pitch = -180.0f - pitch;
        head += 180.0f;
    }

    // normalize heading
    while (head >= 360.0f) head -= 360.0f;
    while (head < 0.0f) head += 360.0f;
}

//
// MARK: Public Aircraft member functions
//

// Play a sound; a looping sound plays until explicitely stopped
uint64_t Aircraft::SoundPlay (const std::string& sndName, float vol)
{
    if (!gpSndSys) return 0;
    uint64_t sndId = gpSndSys->Play(sndName, vol, *this);
    if (!sndId) {
        LOG_MSG(logDEBUG, "Aircraft %08X (%s): Sound '%s' couldn't be played!",
                modeS_id, GetFlightId().c_str(), sndName.c_str());
        return 0;
    }
    chnList.push_back(sndId);               // save to list of sounds the aircraft is making
    if (bChnMuted)                          // if aircraft currently muted then mute this new sound, too
        gpSndSys->SetMute(sndId, true);
    LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Sound '%s' playing%s as id %lu",
                 modeS_id, GetFlightId().c_str(), sndName.c_str(),
                 bChnMuted ? " (muted)" : "", (unsigned long)sndId);
    return sndId;
}

// Stop a continuously playing sound
void Aircraft::SoundStop (uint64_t sndId)
{
    if (!gpSndSys) return;
    gpSndSys->Stop(sndId);
    chnList.remove(sndId);
    LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Sound id %lu stopped",
                 modeS_id, GetFlightId().c_str(), (unsigned long)sndId);
}

// Sets the sound's volume
void Aircraft::SoundVolume (uint64_t sndId, float vol)
{
    if (!gpSndSys) return;
    gpSndSys->SetVolume(sndId, vol);
}

// Mute/Unmute all sounds of the airplane temporarily
void Aircraft::SoundMuteAll (bool bMute)
{
    if (!gpSndSys) return;
    for (uint64_t sndId: chnList)               // mute all channels of the aircraft
        gpSndSys->SetMute(sndId, bMute);
    bChnMuted = bMute;                          // aircraft muted?
    if (!chnList.empty()) {
        LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Sound %s",
                     modeS_id, GetFlightId().c_str(),
                     bMute ? "muted" : "unmuted");
    }
}

// Returns the name of the sound to play per event
std::string Aircraft::SoundGetName (SoundEventsTy sndEvent, float& volAdj) const
{
    // The default is: The more engines, the louder;
    // that not ownly goes for engine sounds, but also the rest will be bigger the bigger the plane
    volAdj = float(pCSLMdl->GetNumEngines());
    // Now go by event type:
    switch (sndEvent) {
        // Engine Sound is based on aircraft classification
        case SND_ENG:
            // Sanity check: need a CSL model to derive details
            if (!pCSLMdl) {
                LOG_MSG(logWARN, "Aircraft %08X (%s): No CSL model info, using default engine sound",
                        modeS_id, GetFlightId().c_str());
                return XP_SOUND_PROP_AIRPLANE;
            }
            // Now check out engine type and return proper sound name
            if (IsGroundVehicle()) {                                // We assume all ground vehicles are electric cars by now...they should be at least ;-)
                return XP_SOUND_ELECTRIC;
            }
            if (pCSLMdl->HasRotor()) {
                volAdj += 1.0f;                                     // Helis are loud anyway, but louder with more engines
                return XP_SOUND_PROP_HELI;
            }
            switch (pCSLMdl->GetClassEngType()) {
                case 'E': return XP_SOUND_ELECTRIC;
                case 'J': return pCSLMdl->GetClassNumEng() == '1' ? XP_SOUND_HIBYPASSJET : XP_SOUND_LOBYPASSJET;
                case 'P': return XP_SOUND_PROP_AIRPLANE;
                case 'T': return XP_SOUND_TURBOPROP;
                default:
                    LOG_MSG(logWARN, "Aircraft %08X (%s): Unknown engine type '%c', using default engine sound",
                            modeS_id, GetFlightId().c_str(), pCSLMdl->GetClassEngType());
                    return XP_SOUND_PROP_AIRPLANE;
            }
            
        // All other sound types have constant sounds assigned
        case SND_REVERSE_THRUST:                        return XP_SOUND_REVERSE_THRUST;
        case SND_TIRE:              volAdj *= 0.50f;    return XP_SOUND_ROLL_RUNWAY;
        case SND_GEAR:              volAdj *= 0.25f;    return XP_SOUND_GEAR;   // that sound is too loud compared to engines, make it more quiet
        case SND_FLAPS:             volAdj *= 0.25f;    return XP_SOUND_FLAP;   // that sound is too loud compared to engines, make it more quiet
            
        default:
            LOG_MSG(logERR, "Aircraft %08X (%s): Unknown Sound Event type %d, no sound name returned",
                    modeS_id, GetFlightId().c_str(), int(sndEvent));
            return "";
    }
}

//
// MARK: Protected Aircraft member functions
//

// Sound-related initializations, called by Create() and ChangeModel()
void Aircraft::SoundSetup ()
{
    // Just to be sure: remove everything
    // In case of a model-rematch this may in fact change the aircraft's sound!
    SoundRemoveAll();
    
    // Find a default "sound size" depending on engine type and number of engines
    if      (pCSLMdl->HasRotor())               sndMinDist = 25;    // Helis are loud, even with a single engine!
    else if (pCSLMdl->GetClassEngType() == 'J') sndMinDist = 20;    // each jet engine, too
    else if (pCSLMdl->GetClassEngType() == 'T') sndMinDist = 15;    // Turboprops are nearly as loud
    else sndMinDist = 10;                                           // everything else falls behind
    sndMinDist *= pCSLMdl->GetNumEngines();
    
    // For gliders there's no engine sound, otherwise there is
    aSndCh[SND_ENG].bAuto = !IsGlider();
}

// Update sound, like position and volume, called once per frame
void Aircraft::SoundUpdate ()
{
    // If we don't want sound we don't get sound
    if (!gpSndSys) return;

    // --- Loop all Sound Definitions ---
    for (SoundEventsTy eSndEvent = SoundEventsTy(0);
         eSndEvent < SND_NUM_EVENTS;
         eSndEvent = SoundEventsTy(eSndEvent + 1))
    {
        // Simpler access to values indexed by the sound event:
        const SoundDefTy    &def        = gaSoundDef[eSndEvent];
        SndChTy             &sndCh      = aSndCh[eSndEvent];

        // Channel is no longer playing?
        if (!gpSndSys->IsValid(sndCh.chnId))
            sndCh.chnId = 0;

        // Automatic Handling of this event?
        if (sndCh.bAuto)
        {
            assert(def.pVal);
            const float fVal = (this->*def.pVal)();     // get the current (dataRef) value
            // --- Looping sound? ---
            if (def.bSndLoop)
            {
                assert(def.valMax > def.valMin);
                
                // Looping sound: Should there be sound?
                if (fVal > def.valMin) {
                    // Set volume based on val (between min and max)
                    float vol = std::clamp<float>((fVal - def.valMin) / (def.valMax - def.valMin), 0.0f, 1.0f);
                    // If there hasn't been a sound triggered do so now
                    if (!sndCh.chnId) {
                        // Get Sound's name and volume adjustment
                        const std::string sndName = SoundGetName(eSndEvent, sndCh.volAdj);
                        if (!sndName.empty()) {
                            vol *= sndCh.volAdj;
                            sndCh.chnId = SoundPlay(sndName, vol);
                        }
                    } else {
                        // Update the volume as it can change any time
                        SoundVolume(sndCh.chnId, vol * sndCh.volAdj);
                    }
                } else {
                    // There should be no sound, remove it if there was one
                    if (sndCh.chnId) {
                        SoundStop(sndCh.chnId);
                        sndCh.chnId = 0;
                    }
                }
            }
            // --- One-time event ---
            else
            {
                // Fresh object, don't even know a 'last value'?
                if (std::isnan(sndCh.lastDRVal)) {
                    sndCh.lastDRVal = fVal;                         // Remember the initial value
                }
                // Should there be sound because the value changed?
                else if (std::fabs(sndCh.lastDRVal - fVal) > 0.01f)
                {
                    sndCh.lastDRVal = fVal;                         // Remember this current value
                    // If there hasn't been a sound triggered do so now
                    if (!sndCh.chnId)
                    {
                        const std::string sndName = SoundGetName(eSndEvent, sndCh.volAdj);
                        if (!sndName.empty())
                            sndCh.chnId = SoundPlay(sndName, sndCh.volAdj);
                    }
                }
                // No more significant value change: Clear pointer to sound once sound has ended
                else if (sndCh.chnId && !gpSndSys->IsValid(sndCh.chnId)) {
                    sndCh.chnId = 0;
                }
            }
        }
        // No automatic handling of this event
        else
        {
            // So if there currently is a channel, remove it
            if (sndCh.chnId) {
                SoundStop(sndCh.chnId);
                sndCh.chnId = 0;
            }
        }
    }
    
    // --- Unpause and Update all channels' 3D position

    // Decide here already if this time we do expensive computations (like sound cone orientation)
    bool bDoExpensiveComp = false;
    if (++skipCounter > EXP_COMP_SKIP_CYCLES) {
        skipCounter = 0;
        bDoExpensiveComp = true;
    }
    
    for (auto iter = chnList.begin(); iter != chnList.end(); )
    {
        // Still valid?
        const uint64_t sndId = *iter;
        if (gpSndSys->IsValid(sndId)) {
            gpSndSys->Unpause(sndId);
            gpSndSys->SetPosOrientation(sndId, *this, bDoExpensiveComp);
            iter++;
        } else {
            // Channels might have become invalid along the way, e.g. if a non-looping sound has ended
            iter = chnList.erase(iter);
        }
    }
}

// Remove all sound, e.g. during destruction
void Aircraft::SoundRemoveAll ()
{
    if (!gpSndSys) return;

    // Log a message only if there actually is anything to stop
    if (!chnList.empty()) {
        LOG_MATCHING(logDEBUG, "Aircraft %08X (%s): Removed all sounds",
                     modeS_id, GetFlightId().c_str());
    }

    // All channels now to be stopped
    for (uint64_t sndId: chnList) gpSndSys->Stop(sndId);
    chnList.clear();
    for (SndChTy &sndChn: aSndCh) sndChn.chnId = 0;
}


//
// MARK: Global Functions
//

// Initialize the sound module and load the sounds
void SoundInit ()
{
    // Don't init twice!
    if (gpSndSys) return;
    
    glob.bSoundAvail = false;
    
#if INCLUDE_FMOD_SOUND + 0 >= 1
    // Try the XP12 system first, if not ruled out by configuration
    if (!glob.bSoundForceFmodInstance) try {
        gpSndSys = new SoundSystemXP();
    }
    catch (const std::runtime_error& e) {
        LOG_MSG(logDEBUG, "Could not attach to the XP sound system: %s", e.what());
    }
    catch (...) {}

    // Otherwise create an FMOD instance
    if (!gpSndSys) try {
        gpSndSys = new SoundSystemFMOD();
    }
    FMOD_CATCH
    catch (const std::runtime_error& e) {
        LOG_MSG(logERR, "Could not attach to/create any sound system: %s", e.what());
    }
    catch(...) {}
#else
    // Try the XP12 system only
    try {
        gpSndSys = new SoundSystemXP();
    }
    catch (const std::runtime_error& e) {
        LOG_MSG(logERR, "Could not attach to the XP sound system: %s", e.what());
    }
    catch(...) {}
#endif

    // Still don't have a sound system now? -> bail
    if (!gpSndSys) return;

    // Some more initializations
    glob.bSoundAvail = true;
    gpSndSys->SetMasterVolume(glob.sndMasterVol);
    SoundLoadXPSounds();
}

// Prepare for this frame's updates, which are about to start
void SoundUpdatesBegin()
{
    // Currently, there's nothing in here
}

// Tell FMOD that all updates are done
void SoundUpdatesDone ()
{
    if (!gpSndSys) return;

    // Mute-on-Pause
    if (glob.bSoundMuteOnPause) {                           // shall act automatically on Pause?
        if (IsPaused()) {                                   // XP is paused?
            if (!glob.bSoundAutoMuted) {                    // ...but not yet muted?
                XPMPSoundMute(true);                        //    -> do mute
                glob.bSoundAutoMuted = true;
            }
        } else {                                            // XP is not paused
            if (glob.bSoundAutoMuted) {                     // ...but sound is auto-muted?
                XPMPSoundMute(false);                       //    -> unmute
                glob.bSoundAutoMuted = false;
            }
        }
    }

    // Have sound system do some last updates
    gpSndSys->Update();
}

// Graceful shoutdown
void SoundCleanup ()
{
    // Remove the sound system
    if (gpSndSys) {
        delete gpSndSys;
        gpSndSys = nullptr;
        LOG_MSG(logDEBUG, "Sound system shut down");
    }
    glob.bSoundAvail = false;
}

} // XPMP2 namespace

//
// MARK: Public functions outside XPMP2 namespace
//

// Enable/Disable Sound
bool XPMPSoundEnable (bool bEnable)
{
    // Any change?
    if (bEnable != XPMP2::glob.bSoundAvail)
    {
        // Enable or disable?
        if (bEnable)
            XPMP2::SoundInit();
        else
            XPMP2::SoundCleanup();
    }
    return XPMPSoundIsEnabled();
}

// Is Sound enabled?
bool XPMPSoundIsEnabled ()
{
    return XPMP2::glob.bSoundAvail;
}

// Set Master Volume
void XPMPSoundSetMasterVolume (float fVol)
{
    XPMP2::glob.sndMasterVol = fVol;
    if (XPMP2::gpSndSys)
        XPMP2::gpSndSys->SetMasterVolume(fVol);
}

// Mute all sounds (temporarily)
void XPMPSoundMute(bool bMute)
{
    if (!XPMP2::gpSndSys) return;
    XPMP2::gpSndSys->SetAllMute(bMute);
    LOG_MSG(XPMP2::logDEBUG, "All sounds %s", bMute ? "muted" : "unmuted");
}

// Add a sound that can later be referenced from an XPMP2::Aircraft
const char* XPMPSoundAdd (const char* sName,
                          const char* filePath,
                          bool bLoop,
                          float coneDir, float conePitch,
                          float coneInAngle, float coneOutAngle,
                          float coneOutVol)
{
    // Test file existence first before bothering
    if (!XPMP2::ExistsFile(filePath)) {
        LOG_MSG(XPMP2::logERR, "Sound file not found: %s", filePath)
        return "Sound File not found";
    }
    // No sound system yet available?
    if (!XPMP2::gpSndSys)
        return "No sound system available";
    // Load the sound and add it to the map
    if (!XPMP2::gpSndSys->LoadSoundFile(sName, filePath, bLoop,
                                       coneDir, conePitch, coneInAngle,
                                       coneOutAngle, coneOutVol))
        return "Failed loading the sound file";
    
    // All good
    return "";
}

// Enumerate all sounds, including the internal ones
const char* XPMPSoundEnumerate (const char* prevName, const char** ppFilePath)
{
    if (XPMP2::gpSndSys) {
        return XPMP2::gpSndSys->EnumerateSounds(prevName, ppFilePath);
    } else {
        if (ppFilePath) *ppFilePath = nullptr;
        return nullptr;
    }
}

// List all possible audio devices if using separate FMOD instance
bool XPMPSoundGetAudioDeviceName(int i, std::string& devName)
{
    if (XPMP2::gpSndSys)
        return XPMP2::gpSndSys->GetAudioDeviceName(i, devName);
    return false;
}

// Set the sound output device if using a separate FMOD instance
bool XPMPSoundSetAudioDeviceName(const std::string& deviceName)
{
    std::string s;
    for (int i = 0; XPMPSoundGetAudioDeviceName(i, s); i++)
        if (s == deviceName)
            return XPMPSoundSetAudioDevice(i);
    return false;
}

// Set the sound output device if using a separate FMOD instance
bool XPMPSoundSetAudioDevice(int i)
{
    if (XPMP2::gpSndSys)
        return XPMP2::gpSndSys->SetAudioDevice(i);
    return false;
}

// Returns the index and name of the active audio device
int XPMPSoundGetActiveAudioDevice (std::string* pDevName)
{
    if (!XPMP2::gpSndSys)
        return 0;

    const int ret = XPMP2::gpSndSys->GetActiveAudioDevice();
    if (pDevName)
        XPMPSoundGetAudioDeviceName(ret, *pDevName);
    return ret;
}

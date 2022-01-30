#include "AudioEngine.h"
#include "Utilities.h"

Implementation::Implementation() :
	mNextChannelId(0)
{
	CAudioEngine::ErrorCheck("Implementation::System_Create", FMOD::System_Create(&mSystem));
	CAudioEngine::ErrorCheck("Implementation::init", mSystem->init(100, FMOD_INIT_NORMAL, 0));
}

Implementation::~Implementation()
{
	CAudioEngine::ErrorCheck("~Implementation", mSystem->release());
}

void Implementation::Update()
{
	vector<ChannelMap::iterator> stoppedChannels;
	for (auto it = mChannels.begin(), itEnd = mChannels.end(); it != itEnd; ++it) 
	{
		bool isPlaying = false;
		it->second->isPlaying(&isPlaying);
		if (!isPlaying)
		{
			stoppedChannels.push_back(it);
		}
	}
	for (auto& it : stoppedChannels)
	{
		LOG_MSG(logDEBUG, "stoppedChannel, erase: %i", it->first);
		mChannels.erase(it);
	}
}

Implementation* sImplementation = nullptr;

void CAudioEngine::Init()
{
	sImplementation = new Implementation;
}

void CAudioEngine::Update()
{
	sImplementation->Update();
	sImplementation->mSystem->update();
}

void CAudioEngine::LoadSound(const string& soundName, const string& soundFilePath, bool bLooping)
{
	if (sImplementation == nullptr)
		return;

	auto foundIt = sImplementation->mSounds.find(soundName);
	if (foundIt != sImplementation->mSounds.end())
		return;

	FMOD_MODE eMode = FMOD_DEFAULT;
	eMode |= FMOD_3D;
	eMode |= FMOD_CREATECOMPRESSEDSAMPLE;
	eMode |= bLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	FMOD::Sound* pSound = nullptr;
	CAudioEngine::ErrorCheck("LoadSound", sImplementation->mSystem->createSound(soundFilePath.c_str(), eMode, nullptr, &pSound));
	if (pSound)
	{
		sImplementation->mSounds[soundName] = pSound;
	}
}

void CAudioEngine::UnloadSound(const string& soundName)
{
	if (sImplementation == nullptr)
		return;

	auto foundIt = sImplementation->mSounds.find(soundName);
	if (foundIt != sImplementation->mSounds.end())
		return;

	CAudioEngine::ErrorCheck("UnloadSound", foundIt->second->release());
	sImplementation->mSounds.erase(foundIt);
}

int CAudioEngine::PlaySounds(const string& soundName, float volumeDb)
{
	if (sImplementation == nullptr)
		return 1;

	int mChannelId = sImplementation->mNextChannelId++;
	auto foundIt = sImplementation->mSounds.find(soundName);
	if (foundIt == sImplementation->mSounds.end()) {
		LOG_MSG(logERROR, "PlaySounds, sound not found");
		return mChannelId;
	}

	FMOD::Channel* pChannel = nullptr;
	CAudioEngine::ErrorCheck("PlaySounds::playSound", sImplementation->mSystem->playSound(foundIt->second, nullptr, true, &pChannel));
	if (pChannel)
		sImplementation->mChannels[mChannelId] = pChannel;

	return mChannelId;
}

void CAudioEngine::SetChannel3dPosition(int channelId, const AudioVector3& _pos, const AudioVector3& _vel)
{
	if (sImplementation == nullptr)
		return;

	auto foundIt = sImplementation->mChannels.find(channelId);
	if (foundIt == sImplementation->mChannels.end()) {
		LOG_MSG(logDEBUG, "SetChannel3dPosition, channel %i not found", channelId);
		return;
	}

	FMOD_VECTOR position = VectorToFmod(_pos);
	FMOD_VECTOR velocity = VectorToFmod(_vel);
	CAudioEngine::ErrorCheck("SetChannel3dPosition", foundIt->second->set3DAttributes(&position, &velocity));
}

void CAudioEngine::SetChannelVolume(int channelId, float volumeDb)
{
	if (sImplementation == nullptr)
		return;

	auto tFoundIt = sImplementation->mChannels.find(channelId);
	if (tFoundIt == sImplementation->mChannels.end())
		return;

	CAudioEngine::ErrorCheck("SetChannelVolume", tFoundIt->second->setVolume(dbToVolume(volumeDb)));
}

void CAudioEngine::SetChannelPaused(int channel, bool paused)
{
	if (sImplementation == nullptr)
		return;

	auto tFoundIt = sImplementation->mChannels.find(channel);
	if (tFoundIt == sImplementation->mChannels.end())
		return;

	CAudioEngine::ErrorCheck("SetChannelPaused", tFoundIt->second->setPaused(paused));
}

void CAudioEngine::StopChannel(int channel)
{
	if (sImplementation == nullptr)
		return;

	auto iter = sImplementation->mChannels.find(channel);
	if (iter == sImplementation->mChannels.end())
		return;

	CAudioEngine::ErrorCheck("RemoveChannel", iter->second->stop());
}

void CAudioEngine::StopAllChannels()
{
	if (sImplementation == nullptr)
		return;

	for (auto it = sImplementation->mChannels.begin(), itEnd = sImplementation->mChannels.end(); it != itEnd; ++it)
	{
		it->second->stop();
	}
}

void CAudioEngine::SetListenerPosition(const AudioVector3& _pos, const AudioVector3& _vel, const AudioVector3& _forward, const AudioVector3& _up)
{
	if (sImplementation == nullptr)
		return;

	FMOD_VECTOR forward = VectorToFmod(_forward);
	FMOD_VECTOR up = VectorToFmod(_up);
	FMOD_VECTOR zero = { 0,0,0 };

	CAudioEngine::ErrorCheck("SetListenerPosition", sImplementation->mSystem->set3DListenerAttributes(0, &zero, &zero, &forward, &up));
}

int CAudioEngine::ErrorCheck(const string& method, FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		LOG_MSG(logERROR, "FMOD Error: %s: %i", method.c_str(), result);
		return 1;
	}
	return 0;
}

void CAudioEngine::Shutdown()
{
	delete sImplementation;
}

FMOD_VECTOR CAudioEngine::VectorToFmod(const AudioVector3& vPosition)
{
	FMOD_VECTOR fVec;
	fVec.x = vPosition.x;
	fVec.y = vPosition.y;
	fVec.z = vPosition.z;
	return fVec;
}

float CAudioEngine::dbToVolume(float dB)
{
	return powf(10.0f, 0.05 * dB);
}

float CAudioEngine::VolumeTodB(float volume)
{
	return 20.0f * log10f(volume);
}
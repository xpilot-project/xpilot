/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2022 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#include "AudioEngine.h"
#include "Utilities.h"

CAudioEngine::CAudioEngine() :
	mNextChannelId(0) {
	CAudioEngine::ErrorCheck("Implementation::System_Create", FMOD::System_Create(&SoundSystem));
	CAudioEngine::ErrorCheck("Implementation::init", SoundSystem->init(300, FMOD_INIT_NORMAL, 0));
}

CAudioEngine::~CAudioEngine() {
	CAudioEngine::ErrorCheck("~Implementation", SoundSystem->release());

	std::lock_guard soundLock(mSoundMapMutex);
	SoundMap.clear();

	std::lock_guard channelLock(mChannelMapMutex);
	ChannelMap.clear();
}

void CAudioEngine::Update() {
	SoundSystem->update();
}

void CAudioEngine::LoadSound(const std::string& soundName, const std::string& soundFilePath, bool bLooping) {
	std::lock_guard soundLock(mSoundMapMutex);
	auto foundIt = SoundMap.find(soundName);
	if (foundIt != SoundMap.end())
		return;

	FMOD_MODE eMode = FMOD_DEFAULT;
	eMode |= FMOD_3D;
	eMode |= FMOD_CREATECOMPRESSEDSAMPLE;
	eMode |= bLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	FMOD::Sound* pSound = nullptr;
	CAudioEngine::ErrorCheck("LoadSound", SoundSystem->createSound(soundFilePath.c_str(), eMode, nullptr, &pSound));
	if (pSound) {
		SoundMap[soundName] = pSound;
	}
}

void CAudioEngine::UnloadSound(const std::string& soundName) {
	std::lock_guard soundLock(mSoundMapMutex);
	auto foundIt = SoundMap.find(soundName);
	if (foundIt != SoundMap.end())
		return;

	CAudioEngine::ErrorCheck("UnloadSound", foundIt->second->release());
	SoundMap.erase(foundIt);
}

int CAudioEngine::CreateSoundChannel(const std::string& soundName, float volumeDb) {
	std::lock_guard soundLock(mSoundMapMutex);
	int mChannelId = mNextChannelId++;
	auto foundIt = SoundMap.find(soundName);
	if (foundIt == SoundMap.end()) {
		return mChannelId;
	}

	FMOD::Channel* pChannel = nullptr;
	CAudioEngine::ErrorCheck("CreateSoundChannel::playSound", SoundSystem->playSound(foundIt->second, nullptr, true, &pChannel));
	if (pChannel) {
		std::lock_guard channelLock(mChannelMapMutex);
		{
			CAudioEngine::ErrorCheck("CreateSoundChannel::set3DMinMaxDistance", pChannel->set3DMinMaxDistance(3.0f, 10000.0f));
			ChannelMap[mChannelId] = pChannel;
		}
	}

	return mChannelId;
}

void CAudioEngine::SetChannel3dPosition(int channelId, const AudioVector3& pos) {
	std::lock_guard channelLock(mChannelMapMutex);
	auto foundIt = ChannelMap.find(channelId);
	if (foundIt == ChannelMap.end()) {
		return;
	}

	FMOD_VECTOR position = VectorToFmod(pos);
	CAudioEngine::ErrorCheck("SetChannel3dPosition", foundIt->second->set3DAttributes(&position, NULL));
}

void CAudioEngine::SetChannelVolume(int channelId, float volume) {
	std::lock_guard channelLock(mChannelMapMutex);
	auto tFoundIt = ChannelMap.find(channelId);
	if (tFoundIt == ChannelMap.end())
		return;

	CAudioEngine::ErrorCheck("SetChannelVolume", tFoundIt->second->setVolume(volume));
}

void CAudioEngine::SetChannelPaused(int channel, bool paused) {
	std::lock_guard channelLock(mChannelMapMutex);
	auto tFoundIt = ChannelMap.find(channel);
	if (tFoundIt == ChannelMap.end())
		return;

	CAudioEngine::ErrorCheck("SetChannelPaused", tFoundIt->second->setPaused(paused));
}

void CAudioEngine::StopChannel(int channel) {
	std::lock_guard channelLock(mChannelMapMutex);
	auto iter = ChannelMap.find(channel);
	if (iter == ChannelMap.end())
		return;

	CAudioEngine::ErrorCheck("RemoveChannel", iter->second->stop());
}

void CAudioEngine::StopAllChannels() {
	std::lock_guard channelLock(mChannelMapMutex);
	for (auto it = ChannelMap.begin(), itEnd = ChannelMap.end(); it != itEnd; ++it) {
		it->second->stop();
	}
}

void CAudioEngine::SetListenerPosition() {
	FMOD_VECTOR zero = { 0.0f,0.0f,0.0f };
	CAudioEngine::ErrorCheck("SetListenerPosition", SoundSystem->set3DListenerAttributes(0, &zero, NULL, NULL, NULL));
}

int CAudioEngine::ErrorCheck(const std::string& method, FMOD_RESULT result) {
	if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE)) {
		LOG_MSG(logERROR, "FMOD Error: %s: %i", method.c_str(), result);
		return 1;
	}
	return 0;
}

FMOD_VECTOR CAudioEngine::VectorToFmod(const AudioVector3& vPosition) {
	FMOD_VECTOR fVec{};
	fVec.x = vPosition.x;
	fVec.y = vPosition.y;
	fVec.z = vPosition.z;
	return fVec;
}

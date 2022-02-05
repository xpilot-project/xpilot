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

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "fmod.hpp"

#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <iostream>

struct AudioVector3 {
	float x;
	float y;
	float z;
};

class CAudioEngine {
public:
	CAudioEngine();
	~CAudioEngine();

	void Update();
	void Shutdown();
	void LoadSound(const std::string& soundName, const std::string& soundFilePath, bool bLooping = true);
	void UnloadSound(const std::string& soundName);
	int CreateSoundChannel(const std::string& soundName, float fVolumedB = 0.0f);
	void SetChannel3dPosition(int nChannelId, const AudioVector3& position, const AudioVector3& velocity);
	void SetChannelVolume(int nChannelId, float fVolumedB);
	void SetChannelPaused(int channel, bool paused);
	void StopChannel(int channel);
	void StopAllChannels();
	void SetListenerPosition(const AudioVector3& vPos, const AudioVector3& velocity, const AudioVector3& forward, const AudioVector3& up);
	FMOD_VECTOR VectorToFmod(const AudioVector3& vPosition);

	FMOD::System* SoundSystem;
	std::map<std::string, FMOD::Sound*> SoundMap;
	std::map<int, FMOD::Channel*> ChannelMap;

	static int ErrorCheck(const std::string& method, FMOD_RESULT result);

private:
	int mNextChannelId;
};

#endif // !AUDIO_ENGINE_H

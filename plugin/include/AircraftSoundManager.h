#ifndef AIRCRAFT_SOUND_MANAGER
#define AIRCRAFT_SOUND_MANAGER

#include <AL/al.h>

#include <map>
#include <string>
#include <vector>
#include <thread>

class AircraftSoundManager
{
public:
	static AircraftSoundManager* get();

	ALuint jetEngine() const { return m_jetEngine; }
	ALuint pistonProp() const { return m_pistonProp; }
	ALuint turboProp() const { return m_turboProp; }
	ALuint helicopter() const { return m_helicopter; }

private:
	AircraftSoundManager();
	~AircraftSoundManager();

	ALuint addSound(const char* filename);

	ALuint m_jetEngine;
	ALuint m_pistonProp;
	ALuint m_turboProp;
	ALuint m_helicopter;

	static AircraftSoundManager* instance;
};

#endif // !AIRCRAFT_SOUND_MANAGER

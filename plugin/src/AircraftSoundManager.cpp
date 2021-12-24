#include "AircraftSoundManager.h"
#include "Utilities.h"
#include <AL/alext.h>
#include <sndfile.h>
#include <iostream>
#include <functional>
#include <climits>

AircraftSoundManager* AircraftSoundManager::instance = nullptr;

AircraftSoundManager::AircraftSoundManager()
{
	std::string jet(GetPluginPath() + "/Resources/Sounds/JetEngine.wav");
	m_jetEngine = addSound(jet.c_str());

	std::string jetStarter(GetPluginPath() + "/Resources/Sounds/JetStarter.wav");
	m_jetStarter = addSound(jetStarter.c_str());

	std::string pistionProp(GetPluginPath() + "/Resources/Sounds/PistonProp.wav");
	m_pistonProp = addSound(pistionProp.c_str());

	std::string pistonStarter(GetPluginPath() + "/Resources/Sounds/PistonStarter.wav");
	m_pistonStarter = addSound(pistonStarter.c_str());

	std::string pathTurboProp(GetPluginPath() + "/Resources/Sounds/TurboProp.wav");
	m_turboProp = addSound(pathTurboProp.c_str());

	std::string turboStarter(GetPluginPath() + "/Resources/Sounds/TurboStarter.wav");
	m_turboStarter = addSound(turboStarter.c_str());

	std::string pathHeli(GetPluginPath() + "/Resources/Sounds/Helicopter.wav");
	m_helicopter = addSound(pathHeli.c_str());
}

AircraftSoundManager::~AircraftSoundManager()
{
	alDeleteBuffers(1, &m_jetEngine);
	alDeleteBuffers(1, &m_jetStarter);
	alDeleteBuffers(1, &m_pistonProp);
	alDeleteBuffers(1, &m_pistonStarter);
	alDeleteBuffers(1, &m_turboProp);
	alDeleteBuffers(1, &m_turboStarter);
	alDeleteBuffers(1, &m_helicopter);
}

AircraftSoundManager* AircraftSoundManager::get()
{
	if (instance == nullptr) {
		instance = new AircraftSoundManager();
	}
	return instance;
}

ALuint AircraftSoundManager::addSound(const char* filename)
{
	ALenum err, format;
	ALuint buffer;
	SNDFILE* sndfile;
	SF_INFO sfinfo;
	short* membuf;
	sf_count_t num_frames;
	ALsizei num_bytes;

	/* Open the audio file and check that it's usable */
	sndfile = sf_open(filename, SFM_READ, &sfinfo);
	if (!sndfile)
	{
		LOG_MSG(logERROR, "Could not open audio in %s: %s", filename, sf_strerror(sndfile));
		return 0;
	}
	if (sfinfo.frames < 1 || sfinfo.frames >(sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels)
	{
		LOG_MSG(logERROR, "Bad sample count in %s (%ld)", filename, sfinfo.frames);
		sf_close(sndfile);
		return 0;
	}

	/* Get the sound format, and figure out the OpenAL format */
	format = AL_NONE;
	if (sfinfo.channels == 1)
		format = AL_FORMAT_MONO16;
	else if (sfinfo.channels == 2)
		format = AL_FORMAT_STEREO16;
	else if (sfinfo.channels == 3)
	{
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			format = AL_FORMAT_BFORMAT2D_16;
	}
	else if (sfinfo.channels == 4)
	{
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			format = AL_FORMAT_BFORMAT3D_16;
	}
	if (!format)
	{
		LOG_MSG(logERROR, "Unsupported channel count: %d", sfinfo.channels);
		sf_close(sndfile);
		return 0;
	}

	/* Decode the whole audio file to a buffer. */
	membuf = static_cast<short*>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));

	num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
	if (num_frames < 1)
	{
		free(membuf);
		sf_close(sndfile);
		LOG_MSG(logERROR, "Failed to read samples in %s (%ld)", filename, num_frames);
		return 0;
	}
	num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

	/* Buffer the audio data into a new buffer object, then free the data and close the file */
	buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

	free(membuf);
	sf_close(sndfile);

	/* Check if an error occured, and clean up if so. */
	err = alGetError();
	if (err != AL_NO_ERROR)
	{
		LOG_MSG(logERROR, "OpenAL Error: %s", alGetString(err));
		if (buffer && alIsBuffer(buffer))
			alDeleteBuffers(1, &buffer);
		return 0;
	}

	return buffer;
}
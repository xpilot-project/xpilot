/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2021 Justin Shannon
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

#include "AircraftManager.h"
#include "NetworkAircraft.h"
#include "Utilities.h"
#include "GeoCalc.hpp"
#include "Quaternion.hpp"
#include "XPilot.h"

namespace xpilot
{
	constexpr long FAST_POSITION_INTERVAL_TOLERANCE = 300;
	constexpr double ERROR_CORRECTION_INTERVAL_FAST = 2.0;
	constexpr double ERROR_CORRECTION_INTERVAL_SLOW = 5.0;
	constexpr float CLOSED_SPACE_VOLUME_SCALAR = 0.5f;

	static double NormalizeDegrees(double value, double lowerBound, double upperBound)
	{
		double range = upperBound - lowerBound;
		if (value < lowerBound)
		{
			return value + range;
		}
		if (value > upperBound)
		{
			return value - range;
		}
		return value;
	}

	static double CalculateNormalizedDelta(double start, double end, double lowerBound, double upperBound)
	{
		double range = upperBound - lowerBound;
		double halfRange = range / 2.0;

		if (abs(end - start) > halfRange)
		{
			end += (end > start ? -range : range);
		}

		return end - start;
	}

	static double Round(double value, int to)
	{
		double places = pow(10.0, to);
		return round(value * places) / places;
	}

	mapPlanesTy mapPlanes;
	mapPlanesTy::iterator mapGetAircraftByIndex(int idx)
	{
		int i = 0;
		for (mapPlanesTy::iterator iter = mapPlanes.begin(); iter != mapPlanes.end(); ++iter)
		{
			if (iter->second)
			{
				if (++i == idx)
				{
					return iter;
				}
			}
		}
		return mapPlanes.end();
	}

	AircraftManager::AircraftManager(XPilot* instance) : 
		mEnv(instance),
		m_soundOn("sim/operation/sound/sound_on", ReadOnly),
		m_simPaused("sim/time/paused", ReadOnly),
		m_masterVolumeRatio("sim/operation/sound/master_volume_ratio", ReadOnly),
		m_engineVolumeRatio("sim/operation/sound/engine_volume_ratio", ReadOnly),
		m_exteriorVolumeRatio("sim/operation/sound/exterior_volume_ratio", ReadOnly),
		m_propVolumeRatio("sim/operation/sound/prop_volume_ratio", ReadOnly),
		m_environmentVolumeRatio("sim/operation/sound/enviro_volume_ratio", ReadOnly),
		m_isViewExternal("sim/graphics/view/view_is_external", ReadOnly),
		m_canopyOpenRatio("sim/operation/sound/users_canopy_open_ratio", ReadOnly),
		m_userDoorOpenRatio("sim/operation/sound/users_door_open_ratio", ReadOnly)
	{
		StartAudio();
		XPLMRegisterFlightLoopCallback(&AircraftManager::UpdateListenerPosition, -1.0f, this);
	}

	AircraftManager::~AircraftManager()
	{
		StopAudio();
		XPLMUnregisterFlightLoopCallback(&AircraftManager::UpdateListenerPosition, nullptr);
	}

	void AircraftManager::HandleAddPlane(const std::string& callsign, const AircraftVisualState& visualState, const std::string& airline, const std::string& typeCode)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt != mapPlanes.end()) return;

		NetworkAircraft* plane = new NetworkAircraft(callsign.c_str(), visualState, typeCode.c_str(), airline.c_str(), "", 0, "");
		mapPlanes.emplace(callsign, std::move(plane));
	}

	void AircraftManager::HandleAircraftConfig(const std::string& callsign, const NetworkAircraftConfig& config)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt == mapPlanes.end()) return;

		NetworkAircraft* plane = planeIt->second.get();
		if (!plane) return;

		if (config.data.fullConfig.has_value() && config.data.fullConfig.value())
		{
			plane->SetVisible(true);
		}

		if (config.data.flapsPct.has_value())
		{
			if (config.data.flapsPct.value() != plane->target_flaps_position)
			{
				plane->target_flaps_position = config.data.flapsPct.value();
			}
		}
		if (config.data.gearDown.has_value())
		{
			if (config.data.gearDown.value() != plane->gear_down)
			{
				plane->gear_down = config.data.gearDown.value();
			}
		}
		if (config.data.spoilersDeployed.has_value())
		{
			if (config.data.spoilersDeployed.value() != plane->spoilers_deployed)
			{
				plane->spoilers_deployed = config.data.spoilersDeployed.value();
			}
		}
		if (config.data.lights.has_value())
		{
			if (config.data.lights.value().strobesOn.has_value())
			{
				if (config.data.lights.value().strobesOn.value() != plane->surfaces.lights.strbLights)
				{
					plane->surfaces.lights.strbLights = config.data.lights.value().strobesOn.value();
				}
			}
			if (config.data.lights.value().taxiOn.has_value())
			{
				if (config.data.lights.value().taxiOn.value() != plane->surfaces.lights.taxiLights)
				{
					plane->surfaces.lights.taxiLights = config.data.lights.value().taxiOn.value();
				}
			}
			if (config.data.lights.value().navOn.has_value())
			{
				if (config.data.lights.value().navOn.value() != plane->surfaces.lights.navLights)
				{
					plane->surfaces.lights.navLights = config.data.lights.value().navOn.value();
				}
			}
			if (config.data.lights.value().landingOn.has_value())
			{
				if (config.data.lights.value().landingOn.value() != plane->surfaces.lights.landLights)
				{
					plane->surfaces.lights.landLights = config.data.lights.value().landingOn.value();
				}
			}
			if (config.data.lights.value().beaconOn.has_value())
			{
				if (config.data.lights.value().beaconOn.value() != plane->surfaces.lights.bcnLights)
				{
					plane->surfaces.lights.bcnLights = config.data.lights.value().beaconOn.value();
				}
			}
		}
		if (config.data.enginesRunning.has_value())
		{
			if (config.data.enginesRunning.value() != plane->engines_running)
			{
				plane->engines_running = config.data.enginesRunning.value();
			}
		}
		if (config.data.reverseThrust.has_value())
		{
			if (config.data.reverseThrust.value() != plane->reverse_thrust)
			{
				plane->reverse_thrust = config.data.reverseThrust.value();
			}
		}
		if (config.data.onGround.has_value())
		{
			if (config.data.onGround.value() != plane->on_ground)
			{
				plane->on_ground = config.data.onGround.value();
			}
		}
	}

	void AircraftManager::HandleRemovePlane(const std::string& callsign)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft) return;

		mapPlanes.erase(callsign);
	}

	void AircraftManager::RemoveAllPlanes()
	{
		mapPlanes.clear();
	}

	void AircraftManager::StartAudio()
	{
		audioDevice = alcOpenDevice(nullptr);
		if (!audioDevice) {
			LOG_MSG(logERROR, "Failed to open default sound device.");
			return;
		}

		audioContext = alcCreateContext(audioDevice, nullptr);
		if (!audioContext) {
			LOG_MSG(logERROR, "Failed to create sound context.");
			return;
		}

		if (!alcMakeContextCurrent(audioContext)) {
			LOG_MSG(logERROR, "Failed to make the audio context current.");
			return;
		}

		ALCint major_version, minor_version;
		const char* al_hw = alcGetString(audioDevice, ALC_DEVICE_SPECIFIER);
		const char* al_ex = alcGetString(audioDevice, ALC_EXTENSIONS);
		alcGetIntegerv(nullptr, ALC_MAJOR_VERSION, sizeof(major_version), &major_version);
		alcGetIntegerv(nullptr, ALC_MINOR_VERSION, sizeof(minor_version), &minor_version);

		LOG_MSG(logDEBUG, "OpenAL version       : %d.%d", major_version, minor_version);
		LOG_MSG(logDEBUG, "OpenAL hardware      : %s", al_hw ? al_hw : "none");
		LOG_MSG(logDEBUG, "OpenAL extensions    : %s", al_ex ? al_ex : "none");

		ALfloat	listenerOri[] = { 0, 0, -1, 0, 1, 0 };
		alListener3f(AL_POSITION, 0, 0, 1.0f);
		alListener3f(AL_VELOCITY, 0, 0, 0);
		alListenerfv(AL_ORIENTATION, listenerOri);
	}

	void AircraftManager::StopAudio()
	{
		if (!alcMakeContextCurrent(nullptr)) {
			LOG_MSG(logINFO, "Failed to clear the active audio context.");
		}

		alcDestroyContext(audioContext);

		if (!alcCloseDevice(audioDevice)) {
			LOG_MSG(logERROR, "Failed to close audio playback device.");
		}
	}

	float AircraftManager::UpdateListenerPosition(float, float, int, void* ref)
	{
		XPLMCameraPosition_t camera;
		XPLMReadCameraPosition(&camera);
		ALfloat	zero[3] = { 0,0,0 };
		ALfloat	listenerOri[] = { (ALfloat)sin(camera.heading * M_PI / 180.0f), 0.0f, (ALfloat)cos(camera.heading * M_PI / 180.0f), 0.0f, -1.0f, 0.0f };

		alListenerfv(AL_VELOCITY, zero);
		alListenerfv(AL_POSITION, zero);
		alListenerfv(AL_ORIENTATION, listenerOri);

		float soundVolume = 1.0f;
		float doorSum = 0;
		bool anyDoorOpen = false;

		auto* instance = static_cast<AircraftManager*>(ref);
		if (instance) {
			for (int i = 0; i < 10; i++) {
				doorSum += instance->m_userDoorOpenRatio[i];
			}

			if (doorSum >= 0.075) {
				anyDoorOpen = true;
			}

			if (instance->m_soundOn && !instance->m_simPaused) {
				if (instance->m_isViewExternal == 0 && instance->m_canopyOpenRatio == 0 && anyDoorOpen == false) {
					// internal view
					soundVolume = instance->m_masterVolumeRatio * instance->m_exteriorVolumeRatio * CLOSED_SPACE_VOLUME_SCALAR;
				}
				else {
					// external view
					soundVolume = instance->m_masterVolumeRatio * instance->m_exteriorVolumeRatio;
				}
			}
			else {
				// sounds disable or sim is paused
				soundVolume = 0.0f;
			}

			alListenerf(AL_GAIN, soundVolume);
		}

		return -1.0f;
	}

	void AircraftManager::HandleSlowPositionUpdate(const std::string& callsign, AircraftVisualState visualState, double speed)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
		{
			return;
		}

		auto now = std::chrono::steady_clock::now();

		// The logic here is that if we have not received a fast position packet recently, then
		// we need to derive positional velocities from the last position that we received (either
		// fast or slow) and the position that we're currently processing. We also snap to the
		// newly-reported rotation rather than trying to derive rotational velocities.
		if (!ReceivingFastPositionUpdates(aircraft))
		{
			auto lastUpdateTimeStamp = (aircraft->last_slow_position_timestamp < aircraft->last_fast_position_timestamp) ? aircraft->last_slow_position_timestamp : aircraft->last_fast_position_timestamp;

			auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTimeStamp).count();

			aircraft->positional_velocity_vector = DerivePositionalVelocityVector(
				aircraft->remote_visual_state,
				visualState,
				intervalMs
			);
			aircraft->rotational_velocity_vector = Vector3::Zero();

			AircraftVisualState newVisualState{};
			newVisualState.Lat = aircraft->predicted_visual_state.Lat;
			newVisualState.Lon = aircraft->predicted_visual_state.Lon;
			newVisualState.Altitude = aircraft->predicted_visual_state.Altitude;
			newVisualState.Pitch = visualState.Pitch;
			newVisualState.Heading = visualState.Heading;
			newVisualState.Bank = visualState.Bank;

			aircraft->predicted_visual_state = newVisualState;
			aircraft->remote_visual_state = visualState;
			aircraft->UpdateErrorVectors(ERROR_CORRECTION_INTERVAL_SLOW);
		}

		aircraft->last_slow_position_timestamp = now;
		aircraft->ground_speed = speed;
	}

	void AircraftManager::HandleFastPositionUpdate(const std::string& callsign, const AircraftVisualState& visualState, Vector3 positionalVector, Vector3 rotationalVector)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
		{
			return;
		}

		aircraft->positional_velocity_vector = positionalVector;
		aircraft->rotational_velocity_vector = rotationalVector;
		aircraft->remote_visual_state = visualState;
		aircraft->last_fast_position_timestamp = std::chrono::steady_clock::now();
		aircraft->fast_positions_received_count++;

		aircraft->UpdateErrorVectors(ERROR_CORRECTION_INTERVAL_FAST);
	}

	NetworkAircraft* AircraftManager::GetAircraft(const std::string& callsign)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt == mapPlanes.end()) return nullptr;
		return planeIt->second.get();
	}

	bool AircraftManager::ReceivingFastPositionUpdates(NetworkAircraft* aircraft)
	{
		const auto now = std::chrono::steady_clock::now();
		const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - aircraft->last_fast_position_timestamp);
		return diff.count() < FAST_POSITION_INTERVAL_TOLERANCE;
	}

	Vector3 AircraftManager::DerivePositionalVelocityVector(AircraftVisualState previousVisualState, AircraftVisualState newVisualState, long intervalMs)
	{
		// We're rounding the lat/lon/alt to the lowest common precision among slow and
		// fast position updates, because sometimes the previous visual state is from a
		// fast position. (The new visual state is always from a slow position.)

		double latDelta = DegreesToMeters(CalculateNormalizedDelta(
			Round(previousVisualState.Lat, 5),
			Round(newVisualState.Lat, 5),
			-90.0,
			90.0
		));

		double lonDelta = DegreesToMeters(CalculateNormalizedDelta(
			Round(previousVisualState.Lon, 5),
			Round(newVisualState.Lon, 5),
			-180.0,
			180.0
		)) * LongitudeScalingFactor(newVisualState.Lat);

		double altDelta = newVisualState.Altitude - previousVisualState.Altitude;

		double intervalSec = intervalMs / 1000.0;

		return Vector3(
			lonDelta / intervalSec,
			altDelta / intervalSec,
			latDelta / intervalSec
		);
	}

	void AircraftManager::HandleChangePlaneModel(const std::string& callsign, const std::string& typeIcao, const std::string& airlineIcao)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft) return;

		aircraft->ChangeModel(typeIcao.c_str(), airlineIcao.c_str(), "");
	}
}
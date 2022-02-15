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

#include "AircraftManager.h"
#include "NetworkAircraft.h"
#include "Utilities.h"
#include "GeoCalc.hpp"
#include "Quaternion.hpp"
#include "XPilot.h"

using namespace std;

namespace xpilot
{
	constexpr long FAST_POSITION_INTERVAL_TOLERANCE = 500;
	constexpr double ERROR_CORRECTION_INTERVAL_FAST = 2.0;
	constexpr double ERROR_CORRECTION_INTERVAL_SLOW = 5.0;

	constexpr float CLOSED_SPACE_VOLUME_SCALAR = 0.10f;
	constexpr float OUTSIDE_SPACE_VOLUME_SCALAR = 0.60f;

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
		FlightModel::InitializeModels();

		m_audioEngine = new CAudioEngine();

		m_audioEngine->LoadSound("JetEngine", GetPluginPath() + "/Resources/Sounds/JetEngine.wav");
		m_audioEngine->LoadSound("PistonProp", GetPluginPath() + "/Resources/Sounds/PistonProp.wav");
		m_audioEngine->LoadSound("TurboProp", GetPluginPath() + "/Resources/Sounds/TurboProp.wav");
		m_audioEngine->LoadSound("Heli", GetPluginPath() + "/Resources/Sounds/Helicopter.wav");

		ThisThreadIsXplane();

		XPLMRegisterFlightLoopCallback(&AircraftManager::UpdateAircraftSounds, -1.0f, this);
	}

	AircraftManager::~AircraftManager()
	{
		XPLMUnregisterFlightLoopCallback(&AircraftManager::UpdateAircraftSounds, nullptr);
	}

	void AircraftManager::HandleAddPlane(const string& callsign, const AircraftVisualState& visualState, const string& airline, const string& typeCode)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt != mapPlanes.end()) return;

		NetworkAircraft* plane = new NetworkAircraft(callsign.c_str(), visualState, typeCode.c_str(), airline.c_str(), "", 0, "");
		mapPlanes.emplace(callsign, move(plane));

		if (plane) {
			string engineSound = "JetEngine";
			switch (plane->EngineClass)
			{
			case EngineClassType::JetEngine:
				engineSound = "JetEngine";
				break;
			case EngineClassType::PistonProp:
				engineSound = "PistonProp";
				break;
			case EngineClassType::TurboProp:
				engineSound = "TurboProp";
				break;
			case EngineClassType::Helicopter:
				engineSound = "Heli";
				break;
			default:
				engineSound = "JetEngine";
				break;
			}
			plane->SoundChannelId = m_audioEngine->CreateSoundChannel(engineSound, 1.0f);
		}
	}

	void AircraftManager::HandleAircraftConfig(const string& callsign, const NetworkAircraftConfig& config)
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
			if (config.data.flapsPct.value() != plane->TargetFlapsPosition)
			{
				plane->TargetFlapsPosition = config.data.flapsPct.value();
			}
		}
		if (config.data.gearDown.has_value())
		{
			if (config.data.gearDown.value() != plane->IsGearDown)
			{
				plane->IsGearDown = config.data.gearDown.value();
			}
		}
		if (config.data.spoilersDeployed.has_value())
		{
			if (config.data.spoilersDeployed.value() != plane->IsSpoilersDeployed)
			{
				plane->IsSpoilersDeployed = config.data.spoilersDeployed.value();
			}
		}
		if (config.data.lights.has_value())
		{
			if (config.data.lights.value().strobesOn.has_value())
			{
				if (config.data.lights.value().strobesOn.value() != plane->Surfaces.lights.strbLights)
				{
					plane->Surfaces.lights.strbLights = config.data.lights.value().strobesOn.value();
				}
			}
			if (config.data.lights.value().taxiOn.has_value())
			{
				if (config.data.lights.value().taxiOn.value() != plane->Surfaces.lights.taxiLights)
				{
					plane->Surfaces.lights.taxiLights = config.data.lights.value().taxiOn.value();
				}
			}
			if (config.data.lights.value().navOn.has_value())
			{
				if (config.data.lights.value().navOn.value() != plane->Surfaces.lights.navLights)
				{
					plane->Surfaces.lights.navLights = config.data.lights.value().navOn.value();
				}
			}
			if (config.data.lights.value().landingOn.has_value())
			{
				if (config.data.lights.value().landingOn.value() != plane->Surfaces.lights.landLights)
				{
					plane->Surfaces.lights.landLights = config.data.lights.value().landingOn.value();
				}
			}
			if (config.data.lights.value().beaconOn.has_value())
			{
				if (config.data.lights.value().beaconOn.value() != plane->Surfaces.lights.bcnLights)
				{
					plane->Surfaces.lights.bcnLights = config.data.lights.value().beaconOn.value();
				}
			}
		}
		if (config.data.enginesRunning.has_value())
		{
			if (config.data.enginesRunning.value() != plane->IsEnginesRunning)
			{
				plane->IsEnginesRunning = config.data.enginesRunning.value();
			}
		}
		if (config.data.enginesReversing.has_value())
		{
			if (config.data.enginesReversing.value() != plane->IsEnginesReversing)
			{
				plane->IsEnginesReversing = config.data.enginesReversing.value();
			}
		}
		if (config.data.onGround.has_value())
		{
			if (config.data.onGround.value() != plane->IsReportedOnGround)
			{
				plane->IsReportedOnGround = config.data.onGround.value();
				if (!plane->IsReportedOnGround) {
					plane->TerrainOffsetFinished = false;
				}
			}
		}
	}

	void AircraftManager::HandleRemovePlane(const string& callsign)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft) return;

		m_audioEngine->StopChannel(aircraft->SoundChannelId);

		mapPlanes.erase(callsign);
	}

	void AircraftManager::RemoveAllPlanes()
	{
		m_audioEngine->StopAllChannels();

		mapPlanes.clear();
	}

	float AircraftManager::UpdateAircraftSounds(float, float inElapsedTimeSinceLastFlightLoop, int, void* ref)
	{
		auto* instance = static_cast<AircraftManager*>(ref);
		
		if (instance)
		{
			if (!instance->IsXplaneThread()) {
				return -1.0f;
			}

			float soundVolume = 1.0f;
			float doorSum = 0;
			bool anyDoorOpen = false;

			for (int i = 0; i < 10; i++) {
				doorSum += instance->m_userDoorOpenRatio[i];
			}

			if (doorSum >= 0.075) {
				anyDoorOpen = true;
			}

			bool ShouldPauseSound = !Config::Instance().getEnableAircraftSounds() || !instance->m_soundOn || instance->m_simPaused;

			if (instance->m_isViewExternal == 0 && instance->m_canopyOpenRatio == 0 && anyDoorOpen == false) {
				// internal view
				soundVolume = Config::Instance().getAircraftSoundVolume() / 100.0f * CLOSED_SPACE_VOLUME_SCALAR;
			}
			else {
				// external view
				soundVolume = Config::Instance().getAircraftSoundVolume() / 100.0f * OUTSIDE_SPACE_VOLUME_SCALAR;
			}

			XPLMCameraPosition_t camera;
			XPLMReadCameraPosition(&camera);

			instance->m_audioEngine->SetListenerPosition();

			for (mapPlanesTy::iterator iter = mapPlanes.begin(); iter != mapPlanes.end(); ++iter)
			{
				int channel = iter->second->SoundChannelId;

				AudioVector3 soundPos{};
				soundPos.x = camera.x - iter->second->drawInfo.x;
				soundPos.y = camera.y - iter->second->drawInfo.y;
				soundPos.z = camera.z - iter->second->drawInfo.z;

				if (soundPos.isNonZero()) {
					instance->m_audioEngine->SetChannel3dPosition(channel, soundPos);
					instance->m_audioEngine->SetChannelPaused(channel, ShouldPauseSound || !iter->second->IsEnginesRunning);
					instance->m_audioEngine->SetChannelVolume(channel, soundVolume);
				}
			}

			instance->m_audioEngine->Update();
		}

		return -1.0f;
	}

	void AircraftManager::HandleSlowPositionUpdate(const string& callsign, AircraftVisualState visualState, double speed)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
			return;

		auto now = chrono::steady_clock::now();

		// The logic here is that if we have not received a fast position packet recently, then
		// we need to derive positional velocities from the last position that we received (either
		// fast or slow) and the position that we're currently processing. We also snap to the
		// newly-reported rotation rather than trying to derive rotational velocities.
		if (!ReceivingFastPositionUpdates(aircraft))
		{
			auto lastUpdateTimeStamp = (aircraft->LastSlowPositionTimestamp < aircraft->LastVelocityUpdate) ? aircraft->LastSlowPositionTimestamp : aircraft->LastVelocityUpdate;

			auto intervalMs = chrono::duration_cast<chrono::milliseconds>(now - lastUpdateTimeStamp).count();

			aircraft->PositionalVelocities = DerivePositionalVelocityVector(
				aircraft->VisualState,
				visualState,
				intervalMs
			);
			aircraft->RotationalVelocities = Vector3::Zero();

			AircraftVisualState newVisualState{};
			newVisualState.Lat = aircraft->PredictedVisualState.Lat;
			newVisualState.Lon = aircraft->PredictedVisualState.Lon;
			newVisualState.AltitudeTrue = aircraft->PredictedVisualState.AltitudeTrue;
			newVisualState.Pitch = visualState.Pitch;
			newVisualState.Heading = visualState.Heading;
			newVisualState.Bank = visualState.Bank;

			aircraft->PredictedVisualState = newVisualState;
			aircraft->VisualState = visualState;
		}

		aircraft->LastSlowPositionTimestamp = now;
		aircraft->GroundSpeed = speed;
	}

	void AircraftManager::HandleFastPositionUpdate(const string& callsign, const AircraftVisualState& visualState, Vector3 positionalVector, Vector3 rotationalVector)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
			return;

		aircraft->PositionalVelocities = positionalVector;
		aircraft->RotationalVelocities = rotationalVector;
		aircraft->VisualState = visualState;

		const auto now = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(now - aircraft->LastVelocityUpdate).count() > 500)
		{
			aircraft->RotationalVelocities = Vector3::Zero();
			aircraft->RotationalErrorVelocities = Vector3::Zero();
		}

		aircraft->LastVelocityUpdate = now;
		aircraft->RecordTerrainElevationHistory();
		aircraft->UpdateErrorVectors(chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count());
		aircraft->FastPositionsReceivedCount++;
	}

	NetworkAircraft* AircraftManager::GetAircraft(const string& callsign)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt == mapPlanes.end()) return nullptr;
		return planeIt->second.get();
	}

	bool AircraftManager::ReceivingFastPositionUpdates(NetworkAircraft* aircraft)
	{
		const auto now = chrono::steady_clock::now();
		const auto diff = chrono::duration_cast<chrono::milliseconds>(now - aircraft->LastVelocityUpdate);
		return diff.count() <= FAST_POSITION_INTERVAL_TOLERANCE;
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

		double altDelta = newVisualState.AltitudeTrue - previousVisualState.AltitudeTrue;

		double intervalSec = intervalMs / 1000.0;

		return Vector3(
			lonDelta / intervalSec,
			altDelta / intervalSec,
			latDelta / intervalSec
		);
	}

	void AircraftManager::HandleChangePlaneModel(const string& callsign, const string& typeIcao, const string& airlineIcao)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft) return;

		aircraft->ChangeModel(typeIcao.c_str(), airlineIcao.c_str(), "");
	}
}
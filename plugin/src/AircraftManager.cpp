/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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
#include <chrono>

namespace xpilot
{
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
		mEnv(instance)
	{
		FlightModel::InitializeModels();
		ThisThreadIsXplane();
		XPLMRegisterFlightLoopCallback(&AircraftManager::AircraftMaintenanceCallback, -1.0f, this);
		XPMPRegisterPlaneNotifierFunc(&AircraftManager::AircraftNotifierCallback, this);
	}

	AircraftManager::~AircraftManager()
	{
		XPLMUnregisterFlightLoopCallback(&AircraftManager::AircraftMaintenanceCallback, nullptr);
		XPMPUnregisterPlaneNotifierFunc(&AircraftManager::AircraftNotifierCallback, nullptr);
	}

	void AircraftManager::HandleAddPlane(const std::string& callsign, const AircraftVisualState& visualState,
		const std::string& airline, const std::string& typeCode)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt != mapPlanes.end())
		{
			HandleRemovePlane(callsign); // remove plane, the client will try adding it again
			return;
		}

		NetworkAircraft* plane = new NetworkAircraft(callsign.c_str(), visualState, typeCode.c_str(), airline.c_str(), "", 0, "");
		mapPlanes.emplace(callsign, std::move(plane));
	}

	void AircraftManager::HandleAircraftConfig(const std::string& callsign, const AircraftConfigDto& config)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt == mapPlanes.end()) return;

		NetworkAircraft* plane = planeIt->second.get();
		if (!plane) return;

		if (config.flaps.has_value())
		{
			if (config.flaps.value() != plane->TargetFlapsPosition)
			{
				plane->TargetFlapsPosition = config.flaps.value();
			}
		}
		if (config.gearDown.has_value())
		{
			if (config.gearDown.value() != plane->IsGearDown)
			{
				plane->IsGearDown = config.gearDown.value();
			}
		}
		if (config.spoilersDeployed.has_value())
		{
			if (config.spoilersDeployed.value() != plane->IsSpoilersDeployed)
			{
				plane->IsSpoilersDeployed = config.spoilersDeployed.value();
			}
		}
		if (config.strobeLightsOn.has_value())
		{
			if (config.strobeLightsOn.value() != plane->Surfaces.lights.strbLights)
			{
				plane->Surfaces.lights.strbLights = config.strobeLightsOn.value();
			}
		}
		if (config.taxiLightsOn.has_value())
		{
			if (config.taxiLightsOn.value() != plane->Surfaces.lights.taxiLights)
			{
				plane->Surfaces.lights.taxiLights = config.taxiLightsOn.value();
			}
		}
		if (config.navLightsOn.has_value())
		{
			if (config.navLightsOn.value() != plane->Surfaces.lights.navLights)
			{
				plane->Surfaces.lights.navLights = config.navLightsOn.value();
			}
		}
		if (config.landingLightsOn.has_value())
		{
			if (config.landingLightsOn.value() != plane->Surfaces.lights.landLights)
			{
				plane->Surfaces.lights.landLights = config.landingLightsOn.value();
			}
		}
		if (config.beaconLightsOn.has_value())
		{
			if (config.beaconLightsOn.value() != plane->Surfaces.lights.bcnLights)
			{
				plane->Surfaces.lights.bcnLights = config.beaconLightsOn.value();
			}
		}
		if (config.enginesOn.has_value())
		{
			if (config.enginesOn.value() != plane->IsEnginesRunning)
			{
				plane->IsEnginesRunning = config.enginesOn.value();
			}
		}
		if (config.enginesReversing.has_value())
		{
			if (config.enginesReversing.value() != plane->IsEnginesReversing)
			{
				plane->IsEnginesReversing = config.enginesReversing.value();
			}
		}
		if (config.onGround.has_value())
		{
			if (config.onGround.value() != plane->IsReportedOnGround)
			{
				plane->IsReportedOnGround = config.onGround.value();
			}
		}
	}

	void AircraftManager::HandleRemovePlane(const std::string& callsign)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft) return;

		mapPlanes.erase(callsign);
		mEnv->AircraftDeleted(callsign);
	}

	void AircraftManager::RemoveAllPlanes()
	{
		mapPlanes.clear();
	}

	float AircraftManager::AircraftMaintenanceCallback(float, float inElapsedTimeSinceLastFlightLoop, int, void* ref)
	{
		auto* instance = static_cast<AircraftManager*>(ref);

		if (instance)
		{
			if (!instance->IsXplaneThread())
			{
				return -1.0f;
			}

			// The client will take care of any stale aircraft (if the last position packet was more than 15 seconds ago).
			// If the client doesn't close cleanly for some reason, the aircraft might not get deleted from the sim.
			std::list<std::string> stalePlanes;
			for (const auto& plane : mapPlanes)
			{
				int64_t timeSinceLastUpdate = PrecisionTimestamp() - plane.second->LastUpdated;
				if (timeSinceLastUpdate > 30 * 1000)
				{
					stalePlanes.push_back(plane.first);
					LOG_MSG(logINFO, "Removing Stale Aircraft: %s", plane.first.c_str());
				}
			}

			for (const auto& plane : stalePlanes)
			{
				mapPlanes.erase(plane);
			}
		}

		return -1.0f;
	}

	void AircraftManager::AircraftNotifierCallback(XPMPPlaneID inPlaneID, XPMPPlaneNotification inNotification, void* ref)
	{
		auto* instance = static_cast<AircraftManager*>(ref);
		if (instance)
		{
			XPMP2::Aircraft* pAc = XPMP2::AcFindByID(inPlaneID);
			if (pAc)
			{
				if (inNotification == xpmp_PlaneNotification_Created)
				{
					instance->mEnv->AircraftAdded(pAc->label);
				}
			}
		}
	}

	void AircraftManager::HandleFastPositionUpdate(const std::string& callsign, const AircraftVisualState& visualState,
		Vector3 positionalVector, Vector3 rotationalVector, double speed)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
			return;

		aircraft->PositionalVelocities = positionalVector;
		aircraft->RotationalVelocities = rotationalVector;
		aircraft->VisualState = visualState;
		aircraft->GroundSpeed = speed;
		aircraft->UpdateVelocityVectors();
	}

	void AircraftManager::HandleHeartbeat(const std::string& callsign)
	{
		auto aircraft = GetAircraft(callsign);
		if (!aircraft)
			return;

		aircraft->LastUpdated = PrecisionTimestamp();
	}

	NetworkAircraft* AircraftManager::GetAircraft(const std::string& callsign)
	{
		auto planeIt = mapPlanes.find(callsign);
		if (planeIt == mapPlanes.end()) return nullptr;
		return planeIt->second.get();
	}
}

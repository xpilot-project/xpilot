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

#ifndef AircraftManager_h
#define AircraftManager_h

#include <string>
#include <map>
#include <mutex>

#include "XPilot.h"
#include "NetworkAircraft.h"
#include "NetworkAircraftConfig.h"

namespace xpilot
{
	typedef std::map<std::string, std::unique_ptr<NetworkAircraft>> mapPlanesTy;
	extern mapPlanesTy mapPlanes;
	inline mapPlanesTy::iterator mapGetNextAircraft(mapPlanesTy::iterator iter)
	{
		return std::find_if(std::next(iter), mapPlanes.end(), [](const mapPlanesTy::value_type& p)
		{
			return p.second.get();
		});
	}
	mapPlanesTy::iterator mapGetAircraftByIndex(int idx);

	inline double NormalizeHeading(double heading)
	{
		if (heading <= 0.0) {
			heading += 360.0;
		}
		else if (heading > 360.0) {
			heading -= 360.0;
		}
		return heading;
	}

	class AircraftManager
	{
	public:
		AircraftManager(XPilot* instance);
		~AircraftManager() {};

		void HandleAddPlane(const std::string& callsign, const AircraftVisualState& visualState, const std::string& airline, const std::string& typeCode);
		void HandleAircraftConfig(const std::string& callsign, const NetworkAircraftConfig& config);
		void HandleChangePlaneModel(const std::string& callsign, const std::string& typeIcao, const std::string& airlineIcao);
		void HandleSlowPositionUpdate(const std::string& callsign, AircraftVisualState visualState, double speed);
		void HandleFastPositionUpdate(const std::string& callsign, const AircraftVisualState& visualState, Vector3 positionalVector, Vector3 rotationalVector);
		void HandleRemovePlane(const std::string& callsign);
		void RemoveAllPlanes();

	private:
		XPilot* mEnv;
		NetworkAircraft* GetAircraft(const std::string& callsign);
		bool ReceivingFastPositionUpdates(NetworkAircraft* aircraft);
		Vector3 DerivePositionalVelocityVector(AircraftVisualState previousVisualState, AircraftVisualState newVisualState, long intervalMs);
	};
}

#endif // !AircraftManager_h

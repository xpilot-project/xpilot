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

#ifndef AircraftManager_h
#define AircraftManager_h

#include "XPilot.h"
#include "NetworkAircraft.h"
#include "DataRefAccess.h"
#include "AudioEngine.h"

#include <string>
#include <map>
#include <mutex>

namespace xpilot {
	typedef std::map<std::string, std::unique_ptr<NetworkAircraft>> mapPlanesTy;
	extern mapPlanesTy mapPlanes;
	inline mapPlanesTy::iterator mapGetNextAircraft(mapPlanesTy::iterator iter) {
		return std::find_if(std::next(iter), mapPlanes.end(), [](const mapPlanesTy::value_type& p) {
			return p.second.get();
		});
	}
	mapPlanesTy::iterator mapGetAircraftByIndex(int idx);

	class AircraftManager
	{
	public:
		AircraftManager(XPilot* instance);
		virtual ~AircraftManager();

		void HandleAddPlane(const std::string& callsign, const AircraftVisualState& visualState, const std::string& airline, const std::string& typeCode);
		void HandleAircraftConfig(const std::string& callsign, const AircraftConfigDto& config);
		void HandleFastPositionUpdate(const std::string& callsign, const AircraftVisualState& visualState, Vector3 positionalVector, Vector3 rotationalVector, double speed);
		void HandleHeartbeat(const std::string& callsign);
		void HandleRemovePlane(const std::string& callsign);
		void RemoveAllPlanes();

	protected:
		DataRefAccess<int> m_soundOn;
		DataRefAccess<int> m_simPaused;
		DataRefAccess<float> m_masterVolumeRatio;
		DataRefAccess<float> m_engineVolumeRatio;
		DataRefAccess<float> m_exteriorVolumeRatio;
		DataRefAccess<float> m_propVolumeRatio;
		DataRefAccess<float> m_environmentVolumeRatio;
		DataRefAccess<int> m_isViewExternal;
		DataRefAccess<float> m_canopyOpenRatio;
		DataRefAccess<std::vector<float>> m_userDoorOpenRatio;

	private:
		XPilot* mEnv;
		std::unique_ptr<CAudioEngine> m_audioEngine;

		NetworkAircraft* GetAircraft(const std::string& callsign);
		static float AircraftMaintenanceCallback(float, float, int, void* ref);
		static void AircraftNotifierCallback(XPMPPlaneID inPlaneID, XPMPPlaneNotification inNotification, void* ref);

		std::thread::id m_xplaneThread;
		void ThisThreadIsXplane() {
			m_xplaneThread = std::this_thread::get_id();
		}
		bool IsXplaneThread()const {
			return std::this_thread::get_id() == m_xplaneThread;
		}
	};
}

#endif // !AircraftManager_h

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
#include "NetworkAircraftConfig.h"
#include "DataRefAccess.h"
#include "AudioEngine.h"

#include <string>
#include <map>
#include <mutex>

using namespace std;

namespace xpilot
{
	typedef map<string, unique_ptr<NetworkAircraft>> mapPlanesTy;
	extern mapPlanesTy mapPlanes;
	inline mapPlanesTy::iterator mapGetNextAircraft(mapPlanesTy::iterator iter)
	{
		return find_if(next(iter), mapPlanes.end(), [](const mapPlanesTy::value_type& p)
		{
			return p.second.get();
		});
	}
	mapPlanesTy::iterator mapGetAircraftByIndex(int idx);

	class AircraftManager
	{
	public:
		AircraftManager(XPilot* instance);
		virtual ~AircraftManager();

		void HandleAddPlane(const string& callsign, const AircraftVisualState& visualState, const string& airline, const string& typeCode);
		void HandleAircraftConfig(const string& callsign, const NetworkAircraftConfig& config);
		void HandleChangePlaneModel(const string& callsign, const string& typeIcao, const string& airlineIcao);
		void HandleSlowPositionUpdate(const string& callsign, AircraftVisualState visualState, double speed);
		void HandleFastPositionUpdate(const string& callsign, const AircraftVisualState& visualState, Vector3 positionalVector, Vector3 rotationalVector);
		void HandleRemovePlane(const string& callsign);
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
		DataRefAccess<vector<float>> m_userDoorOpenRatio;

	private:
		XPilot* mEnv;
		CAudioEngine* m_audioEngine = nullptr;

		NetworkAircraft* GetAircraft(const string& callsign);
		bool ReceivingFastPositionUpdates(NetworkAircraft* aircraft);
		Vector3 DerivePositionalVelocityVector(AircraftVisualState previousVisualState, AircraftVisualState newVisualState, long intervalMs);
		void UpdateAircraft(NetworkAircraft* aircraft);
		static float UpdateAircraftSounds(float, float, int, void* ref);

		thread::id m_xplaneThread;
		void ThisThreadIsXplane()
		{
			m_xplaneThread = this_thread::get_id();
		}
		bool IsXplaneThread()const
		{
			return this_thread::get_id() == m_xplaneThread;
		}
	};
}

#endif // !AircraftManager_h

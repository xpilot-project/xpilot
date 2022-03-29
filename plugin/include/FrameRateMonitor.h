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

#ifndef FrameRateMonitor_h
#define FrameRateMonitor_h

#include "DataRefAccess.h"
#include "Stopwatch.h"
#include "XPLMProcessing.h"

#include <atomic>
#include <vector>
#include <sstream>

namespace xpilot {
	class XPilot;

	class FrameRateMonitor
	{
	public:
		FrameRateMonitor(XPilot* env);
		~FrameRateMonitor();
		void StopMonitoring();
		void StartMonitoring();
	protected:
		DataRefAccess<float> m_frameRatePeriod;
		DataRefAccess<float> m_groundSpeed;
		DataRefAccess<int> m_isExternalVisual;
		DataRefAccess<std::vector<int>> m_overridePlanePath;
		DataRefAccess<int> m_timePaused;
	private:
		bool SkipMonitoring();
		void ResetFrameRateDetection();
		static float FlightLoopCallback(float, float, int, void* ref);

		XPilot* m_environment;
		Stopwatch m_stopwatch;
		bool m_gaveFirstWarning;
		bool m_gaveSecondWarning;
		bool m_gaveDisconnectWarning;
		bool m_gaveHealthyWarning;
	};
}

#endif // !FrameRateMonitor_h

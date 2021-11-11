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

#include "FrameRateMonitor.h"
#include "Utilities.h"
#include "XPilot.h"

namespace xpilot
{
	FrameRateMonitor::FrameRateMonitor(XPilot* env) :
		m_environment(env),
		m_frameRatePeriod("sim/operation/misc/frame_rate_period", ReadOnly),
		m_groundSpeed("sim/flightmodel/position/groundspeed", ReadOnly),
		m_isExternalVisual("sim/network/dataout/is_external_visual", ReadOnly),
		m_overridePlanePath("sim/operation/override/override_planepath", ReadOnly),
		m_timePaused("sim/time/paused", ReadOnly),
		m_gaveFirstWarning(false),
		m_gaveSecondWarning(false),
		m_gaveDisconnectWarning(false),
		m_gaveHealthyWarning(false)
	{

	}

	void FrameRateMonitor::startMonitoring()
	{
		resetFrameRateDetection();

		// give 30 second "warm-up" period after connecting
		XPLMRegisterFlightLoopCallback(flightLoopCallback, 30.0f, this);
	}

	void FrameRateMonitor::stopMonitoring()
	{
		resetFrameRateDetection();
		XPLMUnregisterFlightLoopCallback(flightLoopCallback, this);
	}

	float FrameRateMonitor::flightLoopCallback(float, float, int, void* ref)
	{
		auto* monitor = static_cast<FrameRateMonitor*>(ref);

		if (monitor->skipMonitoring())
			return -1.0f;

		float fps = 1 / monitor->m_frameRatePeriod;

		if (fps < 20.0f)
		{
			float elapsed = monitor->m_stopwatch.elapsed(xpilot::Stopwatch::SECONDS);
			if (!monitor->m_stopwatch.isRunning())
			{
				monitor->m_stopwatch.start();
			}

			monitor->m_gaveHealthyWarning = false;

			std::stringstream warningMsg;
			warningMsg
				<< "X-Plane is not running in real-time because your frame rate is less than 20fps."
				<< " xPilot will automatically disconnect in "
				<< static_cast<int>(floor(30 - elapsed))
				<< " seconds if this is not corrected.";

			if (elapsed >= 10 && elapsed < 20)
			{
				if (!monitor->m_gaveFirstWarning)
				{
					monitor->m_environment->AddNotificationPanelMessage(warningMsg.str(), 241, 196, 15);
					monitor->m_environment->RadioMessageReceived(warningMsg.str(), 241, 196, 15);
					LOG_MSG(logMSG, warningMsg.str().c_str());
					monitor->m_gaveFirstWarning = true;
				}
			}
			else if (elapsed >= 20 && elapsed < 30)
			{
				if (monitor->m_gaveFirstWarning && !monitor->m_gaveSecondWarning)
				{
					monitor->m_environment->AddNotificationPanelMessage(warningMsg.str(), 241, 196, 15);
					monitor->m_environment->RadioMessageReceived(warningMsg.str(), 241, 196, 15);
					LOG_MSG(logMSG, warningMsg.str().c_str());
					monitor->m_gaveSecondWarning = true;
				}
			}
			else if (elapsed >= 30)
			{
				if (monitor->m_gaveFirstWarning && monitor->m_gaveSecondWarning && !monitor->m_gaveDisconnectWarning)
				{
					std::string msg = "Disconnecting from VATSIM because your frame rates have been less than 20fps for more than 30 seconds. Please adjust your X-Plane performance before reconnecting to the network.";
					monitor->m_environment->AddNotificationPanelMessage(msg, 241, 196, 15);
					monitor->m_environment->RadioMessageReceived(msg, 241, 196, 15);
					monitor->m_environment->forceDisconnect(msg);
					LOG_MSG(logMSG, msg.c_str());
					monitor->m_gaveDisconnectWarning = true;
				}
			}
		}
		else
		{
			if ((monitor->m_gaveFirstWarning || monitor->m_gaveSecondWarning) && !monitor->m_gaveHealthyWarning)
			{
				std::string msg = "X-Plane is now running in real time. The automatic disconnect has been cancelled.";
				monitor->m_environment->AddNotificationPanelMessage(msg, 241, 196, 15);
				monitor->m_environment->RadioMessageReceived(msg, 241, 196, 15);
				LOG_MSG(logMSG, msg.c_str());
				monitor->m_gaveHealthyWarning = true;
			}
			monitor->m_gaveFirstWarning = false;
			monitor->m_gaveSecondWarning = false;
			monitor->m_gaveDisconnectWarning = false;
			monitor->m_stopwatch.reset();
		}

		return -1.0;
	}

	void FrameRateMonitor::resetFrameRateDetection()
	{
		m_gaveFirstWarning = false;
		m_gaveSecondWarning = false;
		m_gaveDisconnectWarning = false;
		m_gaveHealthyWarning = false;
		m_stopwatch.reset();
	}

	bool FrameRateMonitor::skipMonitoring()
	{
		return m_groundSpeed < 10.0f || m_isExternalVisual == 1 || m_overridePlanePath[0] == 1 || m_timePaused == 1;
	}
}
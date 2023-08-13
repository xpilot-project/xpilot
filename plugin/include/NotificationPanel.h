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

#pragma once

#include "Config.h"
#include "ImgWindow.h"
#include "Utilities.h"
#include "XplaneCommand.h"

namespace xpilot
{
	class NotificationPanel : ImgWindow
	{
	public:
		NotificationPanel(int left, int top, int right, int bottom);
		~NotificationPanel();
		void AddMessage(const std::string& message, rgb color = { 255,255,255 }, bool showPanel = false);
		void Toggle();
		bool IsAlwaysVisible()const { return m_alwaysVisible; }
		void SetAlwaysVisible(bool visible) { m_alwaysVisible = visible; }
	protected:
		void buildInterface()override;
	private:
		static float OnFlightLoop(float, float, int, void* refcon);
		XPLMFlightLoopID m_flightLoopId;
		std::chrono::system_clock::time_point m_disappearTime;
		XplaneCommand m_togglePanelCommand;
		bool m_scrollToBottom;
		bool m_alwaysVisible;
	};
}
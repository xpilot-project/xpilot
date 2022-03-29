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

#ifndef NotificationPanel_h
#define NotificationPanel_h

#include "ImgWindow.h"
#include "XplaneCommand.h"
#include "Config.h"

#include <list>
#include <chrono>

namespace xpilot {
	class NotificationPanel : ImgWindow
	{
	public:
		NotificationPanel(int left, int top, int right, int bottom);
		~NotificationPanel();
		void AddNotificationPanelMessage(const std::string& message, float red = 255, float green = 255, float blue = 255, bool forceShow = false);
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

#endif // !NotificationPanel_h



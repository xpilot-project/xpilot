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

#include "NotificationPanel.h"
#include "Utilities.h"
#include "Config.h"

namespace xpilot {
	struct NotificationTy
	{
		std::string message;
		float red;
		float green;
		float blue;
	};

	static std::list<NotificationTy> NotificationHistory;

	NotificationPanel::NotificationPanel(int left, int top, int right, int bottom) :
		m_scrollToBottom(false),
		m_alwaysVisible(false),
		m_togglePanelCommand("xpilot/toggle_notification_panel", "xPilot: Notification Panel", [this] { Toggle(); }),
		ImgWindow(left, top, right, bottom, xplm_WindowDecorationSelfDecorated, xplm_WindowLayerFloatingWindows) {
		SetWindowTitle("Notification Panel");
		SetVisible(false);

		XPLMCreateFlightLoop_t flightLoopParams = {
			sizeof(flightLoopParams),
			xplm_FlightLoop_Phase_AfterFlightModel,
			OnFlightLoop,
			reinterpret_cast<void*>(this)
		};
		m_flightLoopId = XPLMCreateFlightLoop(&flightLoopParams);
		XPLMScheduleFlightLoop(m_flightLoopId, -1.0f, true);
	}

	NotificationPanel::~NotificationPanel() {
		XPLMUnregisterFlightLoopCallback(OnFlightLoop, this);
	}

	void NotificationPanel::buildInterface() {
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowBorderSize = 0.0f;

		for (auto& e : NotificationHistory) {
			const ImVec4& color = ImVec4(e.red, e.green, e.blue, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextWrapped("%s", e.message.c_str());
			ImGui::PopStyleColor();
		}
		if (m_scrollToBottom) {
			ImGui::SetScrollHereY(1.0f);
			m_scrollToBottom = false;
		}

		if (!IsAlwaysVisible()) {
			if (m_disappearTime != std::chrono::system_clock::time_point()
				&& std::chrono::system_clock::now() > m_disappearTime) {
				SetVisible(false);
				m_disappearTime = std::chrono::system_clock::time_point();
			}
		}
	}

	float NotificationPanel::OnFlightLoop(float, float, int, void* refcon) {
		auto* panel = reinterpret_cast<NotificationPanel*>(refcon);

		if (panel->GetVisible()) {
			int panelWidth = 600;
			int panelHeight = 100;
			int panelMargin = 35;

			int left, top, right, bottom, screenTop, screenRight, screenLeft, screenBottom;
			XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

			switch (Config::GetInstance().GetNotificationPanelPosition()) {
			default:
			case NotificationPanelPosition::TopRight:
				right = screenRight - panelMargin; /*margin right*/
				top = screenTop - panelMargin; /*margin top*/
				left = screenRight - panelWidth; /*width*/
				bottom = top - panelHeight; /*height*/
				break;
			case NotificationPanelPosition::TopLeft:
				right = screenLeft + panelWidth; /*width*/
				top = screenTop - panelMargin; /*margin top*/
				left = screenLeft + panelMargin; /*margin left*/
				bottom = top - panelHeight; /*height*/
				break;
			case NotificationPanelPosition::BottomLeft:
				right = screenLeft + panelWidth; /*width*/
				top = screenBottom + panelHeight + panelMargin; /*height*/
				left = screenLeft + panelMargin; /*margin left*/
				bottom = screenBottom + panelMargin; /*margin bottom*/
				break;
			case NotificationPanelPosition::BottomRight:
				right = screenRight - panelMargin; /*margin right*/
				top = screenBottom + panelHeight + panelMargin; /*height*/
				left = screenRight - panelWidth; /*margin left*/
				bottom = screenBottom + panelMargin; /*margin bottom*/
				break;
			}

			panel->SetWindowGeometry(left, top, right, bottom);
		}

		return -1.0f;
	}

	void NotificationPanel::AddNotificationPanelMessage(const std::string& message, float red, float green, float blue, bool forceShow) {
		if (!message.empty()) {
			NotificationTy notification;
			notification.message = string_format("[%s] %s", UtcTimestamp().c_str(), message.c_str());
			notification.red = red / 255;
			notification.green = green / 255;
			notification.blue = blue / 255;
			NotificationHistory.push_back(notification);
			m_scrollToBottom = true;

			if (Config::GetInstance().GetNotificationPanelVisible() || forceShow) {
				SetVisible(true);
				m_disappearTime = std::chrono::system_clock::now() +
					std::chrono::milliseconds(Config::GetInstance().GetActualMessagePreviewTime() * 1000);
			}
		}
	}

	void NotificationPanel::Toggle() {
		SetVisible(!GetVisible());
		m_alwaysVisible = !m_alwaysVisible;
		m_disappearTime = std::chrono::system_clock::time_point();
	}
}
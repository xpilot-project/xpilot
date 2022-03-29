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

#ifndef Config_h
#define Config_h

#include "Constants.h"

#include <string>
#include <vector>

namespace xpilot {
	struct CslPackage
	{
		std::string path;
		bool enabled;

		inline bool empty() const {
			return path.empty();
		}
		inline bool operator== (const CslPackage& o) const {
			return path == o.path;
		}
		inline bool operator== (const std::string& s) const {
			return path == s;
		}
	};

	enum class NotificationPanelPosition
	{
		TopRight,
		TopLeft,
		BottomLeft,
		BottomRight
	};

	class Config
	{
	public:
		static Config& GetInstance();
		~Config() = default;
		Config(const Config&) = delete;
		void operator=(const Config&) = delete;
		Config(Config&&)noexcept = default;
		Config& operator=(Config&&)noexcept = default;

		bool LoadConfig();
		bool SaveConfig();

		std::vector<CslPackage> GetCSLPackages()const;
		void SaveCSLPath(int idx, const std::string path);
		void SaveCSLEnabled(int idx, bool enabled);
		bool HasValidPaths() const;

		std::string GetDefaultAcIcaoType() const;
		void SetDefaultAcIcaoType(const std::string type);

		void SetShowHideLabels(bool status);
		bool GetShowHideLabels() const;

		void SetDebugModelMatching(bool status);
		bool GetDebugModelMatching() const;

		void SetTcpPort(std::string port);
		std::string GetTcpPort() const;

		void SetDefaultAtisEnabled(bool status);
		bool GetDefaultAtisEnabled() const;

		void SetNotificationPanelVisible(bool enabled);
		bool GetNotificationPanelVisible() const;

		void SetNotificationPanelTimeout(int timeout);
		int GetNotificationPanelTimeout() const;
		int GetActualMessagePreviewTime() const;

		void SetNotificationPanelPosition(NotificationPanelPosition position);
		NotificationPanelPosition GetNotificationPanelPosition() const;

		void SetOverrideContactAtcCommand(bool status);
		bool GetOverrideContactAtcCommand() const;

		void SetAircraftLabelColor(int color);
		int GetAircraftLabelColor() const;

		void SetTcasDisabled(bool status);
		bool GetTcasDisabled() const;

		void SetMaxLabelDistance(int distance);
		int GetMaxLabelDistance() const;

		void SetLabelCutoffVis(bool value);
		bool GetLabelCutoffVis() const;

		void SetLogLevel(int level);
		int GetLogLevel() const;

		void SetTransmitIndicatorEnabled(bool value);
		bool GetTransmitIndicatorEnabled() const;

		void SetAircraftSoundsEnabled(bool value);
		bool GetAircraftSoundsEnabled() const;

		void SetAircraftSoundVolume(int volume);
		int GetAircraftSoundVolume() const;

	private:
		Config() = default;
		std::vector<CslPackage> m_cslPackages;
		std::string m_defaultAcIcaoType = "A320";
		bool m_showHideLabels = true;
		bool m_debugModelMatching = false;
		bool m_defaultAtisEnabled = false;
		std::string m_tcpPort = "53100";
		bool m_overrideContactAtcCommand = false;
		int m_labelColor = COLOR_YELLOW;
		bool m_tcasDisabled = false;
		bool m_notificationPanelVisible = true;
		int m_notificationPanelTimeout = 10;
		NotificationPanelPosition m_notificationPanelPosition = NotificationPanelPosition::TopRight;
		int m_maxLabelDist = 3;
		bool m_labelCutoffVis = true;
		bool m_transmitIndicatorEnabled = false;
		bool m_aircraftSoundsEnabled = true;
		int m_aircraftSoundsVolume = 50;
		int m_logLevel = 2; // 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Fatal, 5=Msg
	};
}

#endif // !Config_h

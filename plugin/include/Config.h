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
#include "XPMPMultiplayer.h"

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

		std::string GetDefaultAcIcaoType() const { return m_defaultAcIcaoType; }
		void SetDefaultAcIcaoType(const std::string type) {
			m_defaultAcIcaoType = type;
			XPMPSetDefaultPlaneICAO(type.c_str());
		}

		void SetShowHideLabels(bool status) { m_showHideLabels = status; }
		bool GetShowHideLabels() const { return m_showHideLabels; }

		void SetDebugModelMatching(bool status) { m_debugModelMatching = status; }
		bool GetDebugModelMatching() const { return m_debugModelMatching; }

		void SetTcpPort(int port) { m_tcpPort = port; }
		int GetTcpPort() const { return m_tcpPort; }
		void SetUseTcpSocket(bool enabled) { m_useTcpSocket = enabled; }
		bool GetUseTcpSocket() const { return m_useTcpSocket; }

		void SetDefaultAtisEnabled(bool status) { m_defaultAtisEnabled = status; }
		bool GetDefaultAtisEnabled() const { return m_defaultAtisEnabled; }

		void SetNotificationPanelVisible(bool enabled) { m_notificationPanelVisible = enabled; }
		bool GetNotificationPanelVisible() const { return m_notificationPanelVisible; }

		void SetNotificationPanelTimeout(int timeout);
		int GetNotificationPanelTimeout() const { return m_notificationPanelTimeout; }
		int GetActualMessagePreviewTime() const;

		void SetNotificationPanelPosition(NotificationPanelPosition position) { m_notificationPanelPosition = position; }
		NotificationPanelPosition GetNotificationPanelPosition() const { return m_notificationPanelPosition; }

		void SetOverrideContactAtcCommand(bool status) { m_overrideContactAtcCommand = status; }
		bool GetOverrideContactAtcCommand() const { return m_overrideContactAtcCommand; }

		void SetAircraftLabelColor(int color);
		int GetAircraftLabelColor() const { return m_labelColor; }

		void SetTcasDisabled(bool status) { m_tcasDisabled = status; }
		bool GetTcasDisabled() const { return m_tcasDisabled; }

		void SetMaxLabelDistance(int distance) { m_maxLabelDist = distance; }
		int GetMaxLabelDistance() const { return m_maxLabelDist; }

		void SetLabelCutoffVis(bool value) { m_labelCutoffVis = value; }
		bool GetLabelCutoffVis() const { return m_labelCutoffVis; }

		void SetLogLevel(int level) { m_logLevel = std::max(0, std::min(level, 5)); }
		int GetLogLevel() const { return std::max(0, std::min(m_logLevel, 5)); }

		void SetTransmitIndicatorEnabled(bool enabled) { m_transmitIndicatorEnabled = enabled; }
		bool GetTransmitIndicatorEnabled() const { return m_transmitIndicatorEnabled; }

		void SetAircraftSoundsEnabled(bool enabled) { m_aircraftSoundsEnabled = enabled; }
		bool GetAircraftSoundsEnabled() const { return m_aircraftSoundsEnabled; }

		void SetAircraftSoundVolume(int volume) { m_aircraftSoundsVolume = volume; }
		int GetAircraftSoundVolume() const { return std::max(0, std::min(m_aircraftSoundsVolume, 100)); }

	private:
		Config() = default;
		std::vector<CslPackage> m_cslPackages;
		std::string m_defaultAcIcaoType = "A320";
		bool m_showHideLabels = true;
		bool m_debugModelMatching = false;
		bool m_defaultAtisEnabled = false;
		int m_tcpPort = 53100;
		bool m_useTcpSocket = false;
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

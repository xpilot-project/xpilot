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

namespace xpilot
{
    struct CslPackage
    {
        std::string path;
        bool enabled;

        inline bool empty() const
        {
            return path.empty();
        }
        inline bool operator== (const CslPackage& o) const
        {
            return path == o.path;
        }
        inline bool operator== (const std::string& s) const
        {
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
        static Config& getInstance();
        ~Config() = default;
        Config(const Config&) = delete;
        void operator=(const Config&) = delete;
        Config(Config&&)noexcept = default;
        Config& operator=(Config&&)noexcept = default;

        bool loadConfig();
        bool saveConfig();

        typedef std::vector<CslPackage> vecCslPackages;
        std::vector<CslPackage> getCSLPackages()const;
        void saveCSLPath(int idx, const std::string path);
        void saveCSLEnabled(int idx, bool enabled);
        bool loadCSLPackage(int idx);
        bool hasValidPaths() const;

        std::string getDefaultAcIcaoType() const;
        void setDefaultAcIcaoType(const std::string type);

        void setShowHideLabels(bool status);
        bool getShowHideLabels() const;

        void setDebugModelMatching(bool status);
        bool getDebugModelMatching() const;

        void setTcpPort(std::string port);
        std::string getTcpPort() const;

        void setDefaultAtisEnabled(bool status);
        bool getDefaultAtisEnabled() const;

        void setNotificationPanelVisible(bool enabled);
        bool getNotificationPanelVisible() const;

        void setNotificationPanelTimeout(int timeout);
        int getNotificationPanelTimeout() const;
        int getActualMessagePreviewTime() const;

        void setNotificationPanelPosition(NotificationPanelPosition position);
        NotificationPanelPosition getNotificationPanelPosition() const;

        void setOverrideContactAtcCommand(bool status);
        bool getOverrideContactAtcCommand() const;

        void setAircraftLabelColor(int color);
        int getAircraftLabelColor() const;

        void setTcasDisabled(bool status);
        bool getTcasDisabled() const;

        void setMaxLabelDistance(int distance);
        int getMaxLabelDistance() const;

        void setLabelCutoffVis(bool value);
        bool getLabelCutoffVis() const;

        void setLogLevel(int level);
        int getLogLevel() const;

        void setTransmitIndicatorEnabled(bool value);
        bool getTransmitIndicatorEnabled() const;

        void setAircraftSoundsEnabled(bool value);
        bool getAircraftSoundsEnabled() const;

        void setAircraftSoundVolume(int volume);
        int getAircraftSoundVolume() const;

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

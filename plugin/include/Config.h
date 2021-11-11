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

#ifndef Config_h
#define Config_h

#include <string>
#include <vector>
#include "Constants.h"

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

    class Config
    {
    public:
        static Config& Instance();
        ~Config() = default;
        Config(const Config&) = delete;
        void operator=(const Config&) = delete;
        Config(Config&&)noexcept = default;
        Config& operator=(Config&&)noexcept = default;

        bool loadConfig();
        bool saveConfig();

        typedef std::vector<CslPackage> vecCslPackages;
        std::vector<CslPackage> getCSLPackages()const
        {
            return m_cslPackages;
        }
        void saveCSLPath(int idx, const std::string path);
        void saveCSLEnabled(int idx, bool enabled);
        bool loadCSLPackage(int idx);
        bool hasValidPaths() const;

        std::string getDefaultAcIcaoType() const
        {
            return m_defaultAcIcaoType;
        }
        bool setDefaultAcIcaoType(const std::string type);

        bool setShowHideLabels(bool status);
        bool getShowHideLabels() const
        {
            return m_showHideLabels;
        }

        bool setDebugModelMatching(bool status);
        bool getDebugModelMatching() const
        {
            return m_debugModelMatching;
        }

        bool setTcpPort(std::string port);
        std::string getTcpPort() const
        {
            return m_tcpPort;
        }

        bool setDefaultAtisEnabled(bool status);
        bool getDefaultAtisEnabled() const
        {
            return m_defaultAtis;
        }

        bool setShowMessagePreview(bool enabled);
        bool getShowNotificationBar()const
        {
            return m_showNotificationBar;
        }

        bool setMessagePreviewTimeout(int timeout);
        int getNotificationBarDisappaerTime() const
        {
            return m_notificationBarDisappearTime;
        }
        int getActualMessagePreviewTime()const
        {
            switch (m_notificationBarDisappearTime)
            {
                case 0:
                    return 5;
                case 1:
                    return 10;
                case 2:
                    return 15;
                case 3:
                    return 30;
                case 4:
                default:
                    return 60;
            }
        }

        bool setOverrideContactAtcCommand(bool status);
        bool getOverrideContactAtcCommand() const
        {
            return m_overrideContactAtcCommand;
        }

        bool setAircraftLabelColor(int c);
        int getAircraftLabelColor()
        {
            return m_labelColor;
        }

        bool setDisableTcas(bool status);
        bool getDisableTcas()const
        {
            return m_disableTcas;
        }

        bool setMaxLabelDistance(int d);
        int getMaxLabelDistance()const
        {
            return m_maxLabelDist;
        }

        bool setLabelCutoffVis(bool b);
        bool getLabelCutoffVis()const
        {
            return m_labelCutoffVis;
        }

        bool setLogLevel(int lvl);
        int getLogLevel()const
        {
            return m_logLevel;
        }

    private:
        Config() = default;
        std::vector<CslPackage> m_cslPackages;
        std::string m_defaultAcIcaoType = "A320";
        bool m_showHideLabels = true;
        bool m_debugModelMatching = false;
        bool m_defaultAtis = false;
        std::string m_tcpPort = "53100";
        bool m_overrideContactAtcCommand = false;
        int m_labelColor = COLOR_YELLOW;
        bool m_disableTcas = false;
        bool m_showNotificationBar = true;
        int m_notificationBarDisappearTime = 10;
        int m_maxLabelDist = 3;
        bool m_labelCutoffVis = true;
        int m_logLevel = 2; // 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Fatal, 5=Msg
    };
}

#endif // !Config_h

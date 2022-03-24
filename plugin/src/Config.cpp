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

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include "Constants.h"
#include "Config.h"
#include "Utilities.h"
#include "XPMPMultiplayer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace xpilot
{
    void to_json(json& j, const CslPackage& p)
    {
        j = json{ {"Path", RemoveSystemPath(p.path)},{"Enabled",p.enabled} };
    }

    void from_json(const json& j, CslPackage& p)
    {
        j.at("Path").get_to(p.path);
        j.at("Enabled").get_to(p.enabled);
    }

    inline std::string json_to_string(const json& j)
    {
        if (j.type() == json::value_t::string)
        {
            return j.get<std::string>();
        }

        return j.dump(1);
    }

    Config& Config::getInstance()
    {
        static auto&& config = Config();
        return config;
    }

    bool Config::loadConfig()
    {
        std::string configPath(GetPluginPath() + "Resources/Config.json");
        std::ifstream ifs(configPath);

        if (!ifs)
            saveConfig(); // create configuration file if it does not exist

        // conversion method for config file
        enum cfgConvE { CFG_NO_CONV = 0, CFG_PORT_CONV } conv = CFG_NO_CONV;

        json jf = json::parse(ifs, nullptr, false);
        if (!jf.empty())
        {
            if (!jf.contains("Version"))
            {
                conv = CFG_PORT_CONV;
            }
            if (jf.contains("ShowAircraftLabels"))
            {
                setShowHideLabels(jf["ShowAircraftLabels"]);
            }
            if (jf.contains("DefaultIcaoType"))
            {
                setDefaultAcIcaoType(jf["DefaultIcaoType"]);
            }
            if (jf.contains("PluginPort"))
            {
                std::string v = json_to_string(jf["PluginPort"]);
                if (conv == CFG_PORT_CONV && v == "45001")
                {
                    // convert to 53100 if previously on 45001
                    v = "53100";
                }
                setTcpPort(v);
            }
            if (jf.contains("DebugModelMatching"))
            {
                setDebugModelMatching(jf["DebugModelMatching"]);
            }
            if (jf.contains("EnableDefaultAtis"))
            {
                setDefaultAtisEnabled(jf["EnableDefaultAtis"]);
            }
            if (jf.contains("ShowNotificationBar"))
            {
                setNotificationPanelVisible(jf["ShowNotificationBar"]);
            }
            if (jf.contains("NotificationBarDisappearTime"))
            {
                setNotificationPanelTimeout(jf["NotificationBarDisappearTime"]);
            }
            if (jf.contains("NotificationPanelPosition"))
            {
                setNotificationPanelPosition(jf["NotificationPanelPosition"]);
            }
            if (jf.contains("OverrideContactAtc"))
            {
                setOverrideContactAtcCommand(jf["OverrideContactAtc"]);
            }
            if (jf.contains("DisableTcas"))
            {
                setTcasDisabled(jf["DisableTcas"]);
            }
            if (jf.contains("LabelColor"))
            {
                setAircraftLabelColor(jf["LabelColor"]);
            }
            if (jf.contains("MaxLabelDist"))
            {
                int dist = std::max(1, std::min(jf.at("MaxLabelDist").get<int>(), 20));
                setMaxLabelDistance(dist);
            }
            if (jf.contains("LabelCutoffVis"))
            {
                setLabelCutoffVis(jf["LabelCutoffVis"]);
            }
            if (jf.contains("LogLevel"))
            {
                setLogLevel(jf["LogLevel"]);
            }
            if (jf.contains("EnableTransmitIndicator"))
            {
                setTransmitIndicatorEnabled(jf["EnableTransmitIndicator"]);
            }
            if (jf.contains("EnableAircraftSounds"))
            {
                setAircraftSoundsEnabled(jf["EnableAircraftSounds"]);
            }
            if (jf.contains("AircraftSoundVolume"))
            {
                int vol = std::max(0, std::min(jf.at("AircraftSoundVolume").get<int>(), 100));
                setAircraftSoundVolume(vol);
            }
            if (jf.contains("CSL"))
            {
                json cslpackages = jf["CSL"];
                for (auto& p : cslpackages)
                {
                    auto csl = p.get<CslPackage>();
                    if (std::find(m_cslPackages.begin(), m_cslPackages.end(), csl.path) == m_cslPackages.end())
                    {
                        m_cslPackages.emplace_back(csl);
                    }
                }
            }
            saveConfig();
            return true;
        }

        saveConfig();
        return false;
    }

    bool Config::saveConfig()
    {
        std::string configPath(GetPluginPath() + "Resources/Config.json");
        std::ofstream file(configPath);

        json j;

        j["Version"] = CONFIG_VERSION;
        j["ShowAircraftLabels"] = getShowHideLabels();
        j["DefaultIcaoType"] = getDefaultAcIcaoType();
        j["PluginPort"] = getTcpPort();
        j["DebugModelMatching"] = getDebugModelMatching();
        j["EnableDefaultAtis"] = getDefaultAtisEnabled();
        j["ShowNotificationBar"] = getNotificationPanelVisible();
        j["NotificationBarDisappearTime"] = getNotificationPanelTimeout();
        j["NotificationPanelPosition"] = getNotificationPanelPosition();
        j["OverrideContactAtc"] = getOverrideContactAtcCommand();
        j["LabelColor"] = getAircraftLabelColor();
        j["DisableTcas"] = getTcasDisabled();
        j["MaxLabelDist"] = getMaxLabelDistance();
        j["LabelCutoffVis"] = getLabelCutoffVis();
        j["LogLevel"] = getLogLevel();
        j["EnableTransmitIndicator"] = getTransmitIndicatorEnabled();
        j["EnableAircraftSounds"] = getAircraftSoundsEnabled();
        j["AircraftSoundVolume"] = getAircraftSoundVolume();

        auto jsonObjects = json::array();
        if (!m_cslPackages.empty())
        {
            for (CslPackage& p : m_cslPackages)
            {
                if (!p.path.empty())
                {
                    json j = p;
                    jsonObjects.push_back(j);
                }
            }
        }
        j["CSL"] = jsonObjects;

        file << j.dump(1);

        if (file.fail())
            return false;

        file.flush();
        file.close();
        return true;
    }

    bool Config::hasValidPaths() const
    {
        return (std::count_if(m_cslPackages.begin(), m_cslPackages.end(), [](const CslPackage& p) {
            return !p.path.empty() && p.enabled && CountFilesInPath(p.path) > 0;
            }) > 0);
    }

    std::vector<CslPackage> Config::getCSLPackages() const
    {
        return m_cslPackages;
    }

    void Config::saveCSLPath(int idx, std::string path)
    {
        while (size_t(idx) >= m_cslPackages.size())
        {
            m_cslPackages.push_back({});
        }
        m_cslPackages[idx].path = path;
    }

    void Config::saveCSLEnabled(int idx, bool enabled)
    {
        while (size_t(idx) >= m_cslPackages.size())
        {
            m_cslPackages.push_back({});
        }
        m_cslPackages[idx].enabled = enabled;
    }

    // Load a CSL package interactively
    bool Config::loadCSLPackage(int idx)
    {
        if (size_t(idx) < m_cslPackages.size())
        {
            const std::string path = GetPluginPath() + m_cslPackages[idx].path;

            if (CountFilesInPath(path) > 1)
            {
                LOG_MSG(logDEBUG, "Found CSL Path: %s", path.c_str());
                if (!path.empty())
                {
                    auto err = XPMPLoadCSLPackage(path.c_str());
                    if (*err)
                    {
                        LOG_MSG(logERROR, "Error loading CSL package (%s): %s", path.c_str(), err);
                    }
                    else
                    {
                        LOG_MSG(logDEBUG, "CSL package successfully loaded: %s", path.c_str());
                        return true;
                    }
                }
            }
            else
            {
                LOG_MSG(logDEBUG, "Skipping CSL path '%s' because it does not exist or the folder is empty.", path.c_str());
            }
        }
        return false;
    }

    std::string Config::getDefaultAcIcaoType() const
    {
        return m_defaultAcIcaoType;
    }

    void Config::setDefaultAcIcaoType(const std::string type)
    {
        m_defaultAcIcaoType = type;
        XPMPSetDefaultPlaneICAO(type.c_str());
    }

    void Config::setShowHideLabels(bool status)
    {
        m_showHideLabels = status;
    }

    bool Config::getShowHideLabels() const
    {
        return m_showHideLabels;
    }

    void Config::setDebugModelMatching(bool status)
    {
        m_debugModelMatching = status;
    }

    bool Config::getDebugModelMatching() const
    {
        return m_debugModelMatching;
    }

    void Config::setTcpPort(std::string port)
    {
        m_tcpPort = port;
    }

    std::string Config::getTcpPort() const
    {
        return m_tcpPort;
    }

    void Config::setDefaultAtisEnabled(bool status)
    {
        m_defaultAtisEnabled = status;
    }

    bool Config::getDefaultAtisEnabled() const
    {
        return m_defaultAtisEnabled;
    }

    void Config::setNotificationPanelPosition(NotificationPanelPosition position)
    {
        m_notificationPanelPosition = position;
    }

    NotificationPanelPosition Config::getNotificationPanelPosition() const
    {
        return m_notificationPanelPosition;
    }

    void Config::setOverrideContactAtcCommand(bool status)
    {
        m_overrideContactAtcCommand = status;
    }

    bool Config::getOverrideContactAtcCommand() const
    {
        return m_overrideContactAtcCommand;
    }

    void Config::setAircraftLabelColor(int color)
    {
        if (color > 0 && color <= 0xFFFFFF)
        {
            m_labelColor = color;
        }
        else
        {
            m_labelColor = COLOR_YELLOW;
        }
    }

    int Config::getAircraftLabelColor() const
    {
        return m_labelColor;
    }

    void Config::setTcasDisabled(bool status)
    {
        m_tcasDisabled = status;
    }

    bool Config::getTcasDisabled() const
    {
        return m_tcasDisabled;
    }

    void Config::setNotificationPanelVisible(bool show)
    {
        m_notificationPanelVisible = show;
    }

    bool Config::getNotificationPanelVisible() const
    {
        return m_notificationPanelVisible;
    }

    void Config::setNotificationPanelTimeout(int timeout)
    {
        switch (timeout)
        {
        case 5:
            m_notificationPanelTimeout = 0;
            break;
        case 10:
            m_notificationPanelTimeout = 1;
            break;
        case 15:
            m_notificationPanelTimeout = 2;
            break;
        case 30:
            m_notificationPanelTimeout = 3;
            break;
        case 60:
            m_notificationPanelTimeout = 4;
            break;
        default:
            if (timeout <= 4)
            {
                m_notificationPanelTimeout = timeout;
            }
            else
            {
                m_notificationPanelTimeout = 2;
            }
            break;
        }
    }

    int Config::getNotificationPanelTimeout() const
    {
        return m_notificationPanelTimeout;
    }

    int Config::getActualMessagePreviewTime() const
    {
        switch (m_notificationPanelTimeout)
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

    void Config::setMaxLabelDistance(int v)
    {
        m_maxLabelDist = v;
    }

    int Config::getMaxLabelDistance() const
    {
        return m_maxLabelDist;
    }

    void Config::setLabelCutoffVis(bool value)
    {
        m_labelCutoffVis = value;
    }

    bool Config::getLabelCutoffVis() const
    {
        return m_labelCutoffVis;
    }

    void Config::setLogLevel(int level)
    {
        if (level > 5) level = 5;
        if (level < 0) level = 0;
        m_logLevel = level;
    }

    int Config::getLogLevel() const
    {
        return m_logLevel;
    }

    void Config::setTransmitIndicatorEnabled(bool value)
    {
        m_transmitIndicatorEnabled = value;
    }

    bool Config::getTransmitIndicatorEnabled() const
    {
        return m_transmitIndicatorEnabled;
    }

    void Config::setAircraftSoundsEnabled(bool value)
    {
        m_aircraftSoundsEnabled = value;
    }

    bool Config::getAircraftSoundsEnabled() const
    {
        return m_aircraftSoundsEnabled;
    }

    void Config::setAircraftSoundVolume(int volume)
    {
        m_aircraftSoundsVolume = volume;
    }

    int Config::getAircraftSoundVolume() const
    {
        return std::max(0, std::min(m_aircraftSoundsVolume, 100));
    }
}
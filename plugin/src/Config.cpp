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
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace xpilot {
	void to_json(json& j, const CslPackage& p) {
		j = json{ {"Path", RemoveSystemPath(p.path)},{"Enabled",p.enabled} };
	}

	void from_json(const json& j, CslPackage& p) {
		j.at("Path").get_to(p.path);
		j.at("Enabled").get_to(p.enabled);
	}

	inline std::string json_to_string(const json& j) {
		if (j.type() == json::value_t::string) {
			return j.get<std::string>();
		}

		return j.dump(1);
	}

	Config& Config::GetInstance() {
		static auto&& config = Config();
		return config;
	}

	bool Config::LoadConfig() {
		std::string configPath(GetPluginPath() + "Resources/Config.json");
		std::ifstream ifs(configPath);

		if (!ifs)
			SaveConfig(); // create configuration file if it does not exist

		json jf = json::parse(ifs, nullptr, false);
		if (!jf.empty()) {
			if (jf.contains("ShowAircraftLabels")) {
				SetShowHideLabels(jf["ShowAircraftLabels"]);
			}
			if (jf.contains("DefaultIcaoType")) {
				SetDefaultAcIcaoType(jf["DefaultIcaoType"]);
			}
			if (jf.contains("PluginPort")) {
				if (jf["PluginPort"].is_string()) {
					try {
						int port = std::stoi(jf["PluginPort"].get<std::string>());
						SetTcpPort(port);
					}
					catch (...) {
						SetTcpPort(53100);
					}
				}
				else {
					SetTcpPort(jf["PluginPort"]);
				}
			}
			if (jf.contains("UseTcpSocket")) {
				SetUseTcpSocket(jf["UseTcpSocket"]);
			}
			if (jf.contains("DebugModelMatching")) {
				SetDebugModelMatching(jf["DebugModelMatching"]);
			}
			if (jf.contains("EnableDefaultAtis")) {
				SetDefaultAtisEnabled(jf["EnableDefaultAtis"]);
			}
			if (jf.contains("ShowNotificationBar")) {
				SetNotificationPanelVisible(jf["ShowNotificationBar"]);
			}
			if (jf.contains("NotificationBarDisappearTime")) {
				SetNotificationPanelTimeout(jf["NotificationBarDisappearTime"]);
			}
			if (jf.contains("NotificationPanelPosition")) {
				SetNotificationPanelPosition(jf["NotificationPanelPosition"]);
			}
			if (jf.contains("OverrideContactAtc")) {
				SetOverrideContactAtcCommand(jf["OverrideContactAtc"]);
			}
			if (jf.contains("DisableTcas")) {
				SetTcasDisabled(jf["DisableTcas"]);
			}
			if (jf.contains("LabelColor")) {
				SetAircraftLabelColor(jf["LabelColor"]);
			}
			if (jf.contains("MaxLabelDist")) {
				int dist = std::max(1, std::min(jf.at("MaxLabelDist").get<int>(), 20));
				SetMaxLabelDistance(dist);
			}
			if (jf.contains("LabelCutoffVis")) {
				SetLabelCutoffVis(jf["LabelCutoffVis"]);
			}
			if (jf.contains("LogLevel")) {
				SetLogLevel(jf["LogLevel"]);
			}
			if (jf.contains("EnableTransmitIndicator")) {
				SetTransmitIndicatorEnabled(jf["EnableTransmitIndicator"]);
			}
			if (jf.contains("EnableAircraftSounds")) {
				SetAircraftSoundsEnabled(jf["EnableAircraftSounds"]);
			}
			if (jf.contains("AircraftSoundVolume")) {
				int vol = std::max(0, std::min(jf.at("AircraftSoundVolume").get<int>(), 100));
				SetAircraftSoundVolume(vol);
			}
			if (jf.contains("CSL")) {
				json cslpackages = jf["CSL"];
				for (auto& p : cslpackages) {
					auto csl = p.get<CslPackage>();
					if (std::find(m_cslPackages.begin(), m_cslPackages.end(), csl.path) == m_cslPackages.end()) {
						m_cslPackages.emplace_back(csl);
					}
				}
			}
			SaveConfig();
			return true;
		}

		SaveConfig();
		return false;
	}

	bool Config::SaveConfig() {
		std::string configPath(GetPluginPath() + "Resources/Config.json");
		std::ofstream file(configPath);

		json j;

		j["Version"] = CONFIG_VERSION;
		j["ShowAircraftLabels"] = GetShowHideLabels();
		j["DefaultIcaoType"] = GetDefaultAcIcaoType();
		j["PluginPort"] = GetTcpPort();
		j["UseTcpSocket"] = GetUseTcpSocket();
		j["DebugModelMatching"] = GetDebugModelMatching();
		j["EnableDefaultAtis"] = GetDefaultAtisEnabled();
		j["ShowNotificationBar"] = GetNotificationPanelVisible();
		j["NotificationBarDisappearTime"] = GetNotificationPanelTimeout();
		j["NotificationPanelPosition"] = GetNotificationPanelPosition();
		j["OverrideContactAtc"] = GetOverrideContactAtcCommand();
		j["LabelColor"] = GetAircraftLabelColor();
		j["DisableTcas"] = GetTcasDisabled();
		j["MaxLabelDist"] = GetMaxLabelDistance();
		j["LabelCutoffVis"] = GetLabelCutoffVis();
		j["LogLevel"] = GetLogLevel();
		j["EnableTransmitIndicator"] = GetTransmitIndicatorEnabled();
		j["EnableAircraftSounds"] = GetAircraftSoundsEnabled();
		j["AircraftSoundVolume"] = GetAircraftSoundVolume();

		auto jsonObjects = json::array();
		if (!m_cslPackages.empty()) {
			for (CslPackage& p : m_cslPackages) {
				if (!p.path.empty()) {
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

	bool Config::HasValidPaths() const {
		return (std::count_if(m_cslPackages.begin(), m_cslPackages.end(), [](const CslPackage& p) {
			return !p.path.empty() && p.enabled && CountFilesInPath(p.path) > 0;
			}) > 0);
	}

	std::vector<CslPackage> Config::GetCSLPackages() const {
		return m_cslPackages;
	}

	void Config::SaveCSLPath(int idx, std::string path) {
		while (size_t(idx) >= m_cslPackages.size()) {
			m_cslPackages.push_back({});
		}
		m_cslPackages[idx].path = path;
	}

	void Config::SaveCSLEnabled(int idx, bool enabled) {
		while (size_t(idx) >= m_cslPackages.size()) {
			m_cslPackages.push_back({});
		}
		m_cslPackages[idx].enabled = enabled;
	}

	void Config::SetAircraftLabelColor(int color) {
		if (color > 0 && color <= 0xFFFFFF) {
			m_labelColor = color;
		}
		else {
			m_labelColor = COLOR_YELLOW;
		}
	}

	void Config::SetNotificationPanelTimeout(int timeout) {
		switch (timeout) {
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
			if (timeout <= 4) {
				m_notificationPanelTimeout = timeout;
			}
			else {
				m_notificationPanelTimeout = 2;
			}
			break;
		}
	}

	int Config::GetActualMessagePreviewTime() const {
		switch (m_notificationPanelTimeout) {
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
}
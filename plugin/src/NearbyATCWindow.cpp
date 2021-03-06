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

#include <list>
#include <mutex>
#include "XPImgWindow.h"
#include "NearbyATCWindow.h"
#include "XPilot.h"
#include "Utilities.h"

namespace xpilot {

	static std::list<NearbyATCList> NearbyList;

	NearbyATCWindow::NearbyATCWindow(XPilot* instance) :
		XPImgWindow(WND_MODE_FLOAT_CENTERED, WND_STYLE_SOLID, WndRect(0, 300, 500, 0)),
		m_com1Frequency("sim/cockpit2/radios/actuators/com1_frequency_hz_833", ReadWrite),
		m_env(instance) {
		SetWindowTitle("Nearby ATC");
		SetWindowResizingLimits(535, 300, 535, 300);
	}

	void NearbyATCWindow::UpdateList(const NearbyAtcDto data) {
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			NearbyList.clear();
			if (data.stations.size() > 0) {
				for (auto& station : data.stations) {
					NearbyATCList atc;
					atc.SetCallsign(station.callsign.c_str());
					atc.SetFrequency(station.frequency.c_str());
					atc.SetXplaneFrequency(station.xplaneFrequency);
					atc.SetRealName(station.name);
					NearbyList.push_back(atc);
				}
			}
		}
	}

	void NearbyATCWindow::ClearList() {
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			NearbyList.clear();
		}
	}

	void NearbyATCWindow::buildInterface() {
		static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		static ImVec4 headerColor{ 0.003f, 0.352f, 0.607f, 1.0f };

		std::lock_guard<std::mutex> lock(m_mutex);
		{
			if (ImGui::BeginChild("OnlineControllers")) {
				auto ctrCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_CTR") || ends_with<std::string>(v.GetCallsign(), "_FSS");
				});

				if (ctrCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Center", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Center");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#center", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_CTR") || ends_with<std::string>(station.GetCallsign(), "_FSS")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto appCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_APP") || ends_with<std::string>(v.GetCallsign(), "_DEP");
				});

				if (appCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Approach", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Approach/Departure");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#approach", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_APP") || ends_with<std::string>(station.GetCallsign(), "_DEP")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto towerCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_TWR");
				});

				if (towerCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Tower", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Tower");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#tower", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_TWR")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto groundCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_GND");
				});

				if (groundCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#ground", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Ground");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#ground", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_GND")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto deliveryCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_DEL");
				});

				if (deliveryCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#delivery", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Delivery");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#delivery", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_DEL")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto atisCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.GetCallsign(), "_ATIS");
				});

				if (atisCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#atis", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" ATIS");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#atis", 4, ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList) {
							if (ends_with<std::string>(station.GetCallsign(), "_ATIS")) {
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.GetCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.GetFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.GetRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.GetCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information")) {
									m_env->RequestStationInfo(station.GetCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.GetCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency")) {
									m_com1Frequency = station.GetXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}
				ImGui::EndChild();
			}
		}
	}
}
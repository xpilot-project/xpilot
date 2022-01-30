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
		m_env(instance)
	{
		SetWindowTitle("Nearby ATC");
		SetWindowResizingLimits(535, 300, 535, 300);
	}

	void NearbyATCWindow::UpdateList(const nlohmann::json data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			NearbyList.clear();
			if (data.find("data") != data.end())
			{
				for (auto& item : data["data"].items())
				{
					try {
						std::string callsign = item.value()["callsign"];
						std::string frequency = item.value()["frequency"];
						int xplaneFrequency = static_cast<double>(item.value()["xplane_frequency"]);
						std::string realName = item.value()["real_name"];

						NearbyATCList atc;
						atc.setCallsign(callsign);
						atc.setFrequency(frequency);
						atc.setXplaneFrequency(xplaneFrequency);
						atc.setRealName(realName);
						NearbyList.push_back(atc);
					}
					catch (nlohmann::detail::type_error& e) { }
				}
			}
		}
	}

	void NearbyATCWindow::ClearList()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			NearbyList.clear();
		}
	}

	void NearbyATCWindow::buildInterface()
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		static ImVec4 headerColor{ 0.003f, 0.352f, 0.607f, 1.0f };

		std::lock_guard<std::mutex> lock(m_mutex);
		{
			if (ImGui::BeginChild("OnlineControllers"))
			{
				auto ctrCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_CTR") || ends_with<std::string>(v.getCallsign(), "_FSS");
				});

				if (ctrCount > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Center", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Center");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#center", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_CTR") || ends_with<std::string>(station.getCallsign(), "_FSS"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto appCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_APP") || ends_with<std::string>(v.getCallsign(), "_DEP");
				});

				if (appCount > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Approach", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Approach/Departure");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#approach", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_APP") || ends_with<std::string>(station.getCallsign(), "_DEP"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto towerCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_TWR");
				});

				if (towerCount > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#Tower", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Tower");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#tower", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_TWR"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto groundCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_GND");
				});

				if (groundCount > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#ground", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Ground");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#ground", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_GND"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto deliveryCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_DEL");
				});

				if (deliveryCount > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#delivery", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" Delivery");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#delivery", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_DEL"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
				}

				auto atisCount = std::count_if(NearbyList.begin(), NearbyList.end(), [](NearbyATCList& v) {
					return ends_with<std::string>(v.getCallsign(), "_ATIS");
				});

				if (atisCount > 0) 
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, headerColor);
					ImGui::BeginChild("#atis", ImVec2(600, 21));
					ImGui::AlignTextToFramePadding();
					ImGui::Text(" ATIS");
					ImGui::EndChild();
					ImGui::PopStyleColor();

					if (ImGui::BeginTable("#atis", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 70);

						for (auto& station : NearbyList)
						{
							if (ends_with<std::string>(station.getCallsign(), "_ATIS"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								char buf[32];
								sprintf(buf, " %s", station.getCallsign().c_str());
								ImGui::AlignTextToFramePadding();
								ImGui::Text(buf);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text(station.getFrequency().c_str());

								ImGui::TableSetColumnIndex(2);
								ImGui::Text(station.getRealName().c_str());

								ImGui::TableSetColumnIndex(3);
								std::string btn1 = "RequestInfo#" + station.getCallsign();
								ImGui::PushID(btn1.c_str());
								if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
								{
									m_env->requestStationInfo(station.getCallsign().c_str());
								}
								ImGui::PopID();

								ImGui::SameLine();

								std::string btn2 = "Frequency#" + station.getCallsign();
								ImGui::PushID(btn2.c_str());
								if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
								{
									m_com1Frequency = station.getXplaneFrequency();
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
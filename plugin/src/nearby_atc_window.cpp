/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2024 Justin Shannon
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

#include "nearby_atc_window.h"

namespace xpilot
{
	static std::list<NearbyAtcList> nearbyList;

	NearbyAtcWindow::NearbyAtcWindow(XPilot* instance) :
		XPImgWindow(WND_MODE_FLOAT_CENTERED, WND_STYLE_SOLID, WndRect(0, 300, 500, 0)),
		m_env(instance),
		m_com1Frequency("sim/cockpit2/radios/actuators/com1_frequency_hz_833", ReadWrite),
		m_com2Frequency("sim/cockpit2/radios/actuators/com2_frequency_hz_833", ReadWrite)
	{
		SetWindowTitle("Nearby ATC");
		SetWindowResizingLimits(535, 300, 535, 300);
	}

	void NearbyAtcWindow::UpdateList(const NearbyAtcDto& data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			nearbyList.clear();
			if (!data.stations.empty())
			{
				for (auto& station : data.stations)
				{
					NearbyAtcList atc;
					atc.SetCallsign(station.callsign);
					atc.SetFrequency(station.frequency);
					atc.SetXplaneFrequency(station.xplaneFrequency);
					atc.SetRealName(station.name);
					nearbyList.push_back(atc);
				}
			}
		}
	}

	void NearbyAtcWindow::ClearList()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		{
			nearbyList.clear();
		}
	}

	void NearbyAtcWindow::buildInterface()
	{
		static ImColor headerColor(1, 100, 173);

		if (m_env->IsNetworkConnected())
		{
			SetWindowTitle(string_format("Nearby ATC: %s", m_env->NetworkCallsign().c_str()));
		}
		else
		{
			SetWindowTitle("Nearby ATC");
		}

		std::lock_guard lock(m_mutex);
		{
			if (ImGui::BeginChild("OnlineControllers"))
			{
				RenderAtcTable("Center/FSS", { "_CTR", "_FSS" });
				RenderAtcTable("Approach/Departure", { "_APP", "_DEP" });
				RenderAtcTable("Tower", { "_TWR" });
				RenderAtcTable("Ground", { "_GND" });
				RenderAtcTable("Clearance Delivery", { "_DEL" });
				RenderAtcTable("ATIS", { "_ATIS" });

				if (m_env->IsNetworkConnected())
				{
					ImGui::PushStyleColor(ImGuiCol_TableRowBg, headerColor.Value);
					if (ImGui::BeginTable("#unicom", 4, ImGuiTableFlags_RowBg))
					{
						ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
						ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
						ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 90);

						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::AlignTextToFramePadding();
						ImGui::Text(" UNICOM");

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("122.800");

						ImGui::TableSetColumnIndex(3);

						ImGui::SameLine(22);
						std::string btn1 = "Frequency1#UNICOM";
						ImGui::PushID(btn1.c_str());
						if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
						{
							m_com1Frequency = 122800;
						}
						ImGui::PopID();

						ImGui::SameLine();
						std::string btn2 = "Frequency2#UNICOM";
						ImGui::PushID(btn2.c_str());
						if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM2 Frequency"))
						{
							m_com2Frequency = 122800;
						}
						ImGui::PopID();

						ImGui::EndTable();
					}
					ImGui::PopStyleColor();
				}

				ImGui::EndChild();
			}
		}
	}

	void NearbyAtcWindow::RenderAtcTable(const std::string& headerText,
		const std::vector<std::string>& callsignSuffixes)
	{
		for (const auto& callsignSuffix : callsignSuffixes)
		{
			const auto count = std::count_if(nearbyList.begin(), nearbyList.end(), [&](const NearbyAtcList& v)
			{
				return ends_with<std::string>(v.GetCallsign(), callsignSuffix);
			});

			if (count > 0)
			{
				std::string headerTextWithSpace = " " + headerText;

				// Render the header and table as before
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImColor(1, 100, 173).Value);
				ImGui::BeginChild(headerText.c_str(), ImVec2(600, 21));
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(headerTextWithSpace.c_str());
				ImGui::EndChild();
				ImGui::PopStyleColor();

				if (ImGui::BeginTable(headerText.c_str(), 4, ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("#callsign", ImGuiTableColumnFlags_WidthFixed, 110);
					ImGui::TableSetupColumn("#frequency", ImGuiTableColumnFlags_WidthFixed, 100);
					ImGui::TableSetupColumn("#name", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("#actions", ImGuiTableColumnFlags_WidthFixed, 90);

					for (auto& station : nearbyList)
					{
						if (ends_with<std::string>(station.GetCallsign(), callsignSuffix))
						{
							RenderAtcStationEntry(station);
						}
					}
					ImGui::EndTable();
				}
			}
		}
	}

	void NearbyAtcWindow::RenderAtcStationEntry(const NearbyAtcList& station)
	{
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		char buf[32];
		sprintf(buf, " %s", station.GetCallsign().c_str());
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(buf);

		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(station.GetFrequency().c_str());

		ImGui::TableSetColumnIndex(2);
		ImGui::TextUnformatted(station.GetRealName().c_str());

		ImGui::TableSetColumnIndex(3);
		const std::string btn1 = "RequestInfo#" + station.GetCallsign();
		ImGui::PushID(btn1.c_str());
		if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
		{
			m_env->RequestStationInfo(station.GetCallsign());
		}
		ImGui::PopID();

		ImGui::SameLine();
		const std::string btn2 = "Frequency1#" + station.GetCallsign();
		ImGui::PushID(btn2.c_str());
		if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
		{
			m_com1Frequency = station.GetXplaneFrequency();
		}
		ImGui::PopID();

		ImGui::SameLine();
		const std::string btn3 = "Frequency2#" + station.GetCallsign();
		ImGui::PushID(btn3.c_str());
		if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM2 Frequency"))
		{
			m_com2Frequency = station.GetXplaneFrequency();
		}
		ImGui::PopID();
	}
}
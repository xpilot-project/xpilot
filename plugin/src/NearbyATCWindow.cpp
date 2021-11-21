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

#include <list>
#include <mutex>
#include "XPImgWindow.h"
#include "NearbyATCWindow.h"
#include "XPilot.h"

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
		if (ImGui::BeginTable("##nearbyatc", 4, flags))
		{
			ImGui::TableSetupColumn("Callsign", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 170);
			ImGui::TableSetupColumn("Frequency", ImGuiTableColumnFlags_WidthFixed, 100);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 50);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();
			
			std::lock_guard<std::mutex> lock(m_mutex);
			{
				for (auto &station : NearbyList)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", station.getCallsign().c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%s", station.getRealName().c_str());
				
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", station.getFrequency().c_str());

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
}
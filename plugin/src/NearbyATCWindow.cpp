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
						NearbyATCList atc;
						atc.setCallsign(item.value()["callsign"]);
						atc.setFrequency(item.value()["frequency"]);
						atc.setXplaneFrequency(item.value()["xplane_frequency"]);
						atc.setRealName(item.value()["real_name"]);
						NearbyList.push_back(atc);
					}
					catch (nlohmann::detail::type_error& e) {

					}
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

	void NearbyATCWindow::buildInterface() {

		ImGui::PushFont(0);

		ImGui::Columns(4, "whosonline", false);
		ImGui::SetColumnWidth(0, 150);
		ImGui::SetColumnWidth(1, 200);
		ImGui::SetColumnWidth(2, 100);
		ImGui::SetColumnWidth(3, 50);
	

		ImGui::Separator();

		ImGui::Text("Callsign");
		ImGui::NextColumn();
		ImGui::Text("Real Name");
		ImGui::NextColumn();
		ImGui::Text("Frequency");
		ImGui::NextColumn();
		ImGui::Text("Actions");
		ImGui::NextColumn();

		ImGui::Separator();

		std::lock_guard<std::mutex> lock(m_mutex);
		{
			for (auto& e : NearbyList)
			{
				ImGui::Text(e.getCallsign().c_str());
				ImGui::NextColumn();
				ImGui::Text(e.getRealName().c_str());
				ImGui::NextColumn();
				ImGui::Text(e.getFrequency().c_str());
				ImGui::NextColumn();
				if (ImGui::ButtonIcon(ICON_FA_INFO, "Request Station Information"))
				{
					m_env->requestStationInfo(e.getCallsign());
				}
				ImGui::SameLine();
				if (ImGui::ButtonIcon(ICON_FA_HEADSET, "Tune COM1 Frequency"))
				{
					m_com1Frequency = e.getXplaneFrequency();
				}
			}
		}
		ImGui::PopFont();
	}
}
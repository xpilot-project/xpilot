/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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

#pragma once

#include "dto.h"
#include "xpilot.h"
#include "xp_img_window.h"

namespace xpilot
{
	class XPilot;

	class NearbyAtcList
	{
	public:
		std::string GetCallsign() const { return m_callsign; }
		std::string GetFrequency() const { return m_frequency; }
		std::string GetRealName() const { return m_realName; }
		int GetXplaneFrequency() const { return m_xplaneFrequency; }
		void SetCallsign(const std::string& value) { m_callsign = value; }
		void SetFrequency(const std::string& value) { m_frequency = value; }
		void SetRealName(const std::string& value) { m_realName = value; }
		void SetXplaneFrequency(const int value) { m_xplaneFrequency = value; }
	private:
		std::string m_callsign;
		std::string m_frequency;
		std::string m_realName;
		int m_xplaneFrequency = 0;
	};

	class NearbyAtcWindow : public XPImgWindow
	{
	public:
		NearbyAtcWindow(XPilot* instance);
		~NearbyAtcWindow() final = default;
		void UpdateList(const NearbyAtcDto& data);
		void ClearList();
	protected:
		void buildInterface() override;
		void RenderAtcStationEntry(const NearbyAtcList& station);
		void RenderAtcTable(const std::string& headerText, const std::vector<std::string>& callsignSuffixes);
	private:
		XPilot* m_env;
		std::mutex m_mutex;
		DataRefAccess<int> m_com1Frequency;
		DataRefAccess<int> m_com2Frequency;
	};
}
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

#ifndef NearbyATCWindow_h
#define NearbyATCWindow_h

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace xpilot 
{
	class XPilot;

	class NearbyATCList
	{
	public:
		std::string getCallsign() { return m_callsign; }
		std::string getFrequency() { return m_frequency; }
		std::string getRealName() { return m_realName; }
		int getXplaneFrequency() { return m_xplaneFrequency; }
		void setCallsign(std::string value) { m_callsign = value; }
		void setFrequency(std::string value) { m_frequency = value; }
		void setRealName(std::string value) { m_realName = value; }
		void setXplaneFrequency(int value) { m_xplaneFrequency = value; }
	private:
		std::string m_callsign;
		std::string m_frequency;
		std::string m_realName;
		int m_xplaneFrequency;
	};

	class NearbyATCWindow : public XPImgWindow {
	public:
		NearbyATCWindow(XPilot* instance);
		~NearbyATCWindow() final = default;
		void UpdateList(const nlohmann::json data);
		void ClearList();
	protected:
		void buildInterface() override;
	private:
		XPilot* m_env;
		std::mutex m_mutex;
		DataRefAccess<int> m_com1Frequency;
	};

}

#endif // !NearbyATCWindow_h

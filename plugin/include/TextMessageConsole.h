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

#ifndef TextMessageConsole_h
#define TextMessageConsole_h

#include "XPImgWindow.h"

#include <list>

namespace xpilot {

	class XPilot;

	class ConsoleMessage
	{
	public:
		std::string GetMessage() { return m_message; }
		float GetRed() { return m_red / 255; }
		float GetGreen() { return m_green / 255; }
		float GetBlue() { return m_blue / 255; }
		void SetMessage(std::string value) { m_message = value; }
		void SetRed(double value) { m_red = value; }
		void setGreen(double value) { m_green = value; }
		void setBlue(double value) { m_blue = value; }
	private:
		std::string m_message;
		double m_red;
		double m_green;
		double m_blue;
	};

	struct Tab
	{
		std::string tabName;
		std::string textInput;
		bool isOpen;
		bool scrollToBottom;
		std::list<ConsoleMessage> messageHistory;
	};

	enum class ConsoleTabType
	{
		Received,
		Sent
	};

	class TextMessageConsole : public XPImgWindow
	{
	public:
		TextMessageConsole(XPilot* instance);
		void CreateNonExistingTab(const std::string& tabName);
		void HandlePrivateMessage(const std::string& msg, const std::string& recipient, ConsoleTabType tabType);
		void RadioMessageReceived(std::string message, double red = 255, double green = 255, double blue = 255);
		void SendRadioMessage(const std::string& msg);
		void SendPrivateMessage(const std::string& tabName, const std::string& msg);
		void PrivateMessageError(std::string tabName, std::string error);
		void CloseTab(const std::string& tabName);
	protected:
		void buildInterface() override;
		void ShowErrorMessage(std::string error);
	private:
		bool m_scrollToBottom;
		XPilot* m_env;
	};

}

#endif // !TextMessageConsole_h

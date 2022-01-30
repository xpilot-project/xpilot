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

using namespace std;

namespace xpilot {

	class XPilot;

	class ConsoleMessage
	{
	private:
		string message;
		double red;
		double green;
		double blue;
	public:
		string getMessage() { return message; }
		float getRed() { return red / 255; }
		float getGreen() { return green / 255; }
		float getBlue() { return blue / 255; }
		void setMessage(string value) { message = value; }
		void setRed(double value) { red = value; }
		void setGreen(double value) { green = value; }
		void setBlue(double value) { blue = value; }
	};

	struct Tab 
	{
		string tabName;
		string textInput;
		bool isOpen;
		bool scrollToBottom;
		list<ConsoleMessage> messageHistory;
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
		void CreateNonExistingTab(const string& tabName);
		void HandlePrivateMessage(const string& msg, const string& recipient, ConsoleTabType tabType);
		void RadioMessageReceived(string message, double red = 255, double green = 255, double blue = 255);
		void SendRadioMessage(const string& msg);
		void SendPrivateMessage(const string& tabName, const string& msg);
		void PrivateMessageError(string tabName, string error);
		void CloseTab(const string& tabName);
	protected:
		void buildInterface() override;
		void ShowErrorMessage(string error);
	private:
		bool m_scrollToBottom;
		XPilot* m_env;
	};

}

#endif // !TextMessageConsole_h

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

#pragma once

#include "xp_img_window.h"

namespace xpilot
{

	class XPilot;

	class ConsoleMessage
	{
	public:
		std::string GetMessage() { return m_message; }
		void SetMessage(std::string value) { m_message = value; }
		void SetColor(rgb color) { m_color = color; }
		ImVec4 GetColor() const { return ImVec4(m_color.red / 255.0f, m_color.green / 255.0f, m_color.blue / 255.0f, 1.0f); }
	private:
		std::string m_message;
		rgb m_color;
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
		void HandlePrivateMessage(const std::string& recipient, const std::string& message, ConsoleTabType tabType);
		void AddMessage(const std::string& message, const rgb& color = Colors::White);
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
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

#include <algorithm>
#include <string>
#include <list>
#include <regex>

#include "TextMessageConsole.h"
#include "Utilities.h"
#include "XPilot.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace xpilot
{
	static std::string m_inputValue;
	static std::list<ConsoleMessage> m_messageHistory;
	static std::list<Tab> m_tabs;

	enum class CommandOptions
	{
		Chat,
		RequestAtis,
		MetarRequest,
		SetRadioFrequency,
		OverrideRx,
		OverrideTx,
		Close,
		CloseAll,
		Clear,
		None
	};

	CommandOptions resolveOption(std::string input)
	{
		std::string v(str_tolower(input));
		if (v == ".chat" || v == ".msg") return xpilot::CommandOptions::Chat;
		if (v == ".atis") return xpilot::CommandOptions::RequestAtis;
		if (v == ".metar" || v == ".wx") return xpilot::CommandOptions::MetarRequest;
		if (v == ".com1" || v == ".com2") return xpilot::CommandOptions::SetRadioFrequency;
		if (v == ".tx") return xpilot::CommandOptions::OverrideTx;
		if (v == ".rx") return xpilot::CommandOptions::OverrideRx;
		if (v == ".clear") return xpilot::CommandOptions::Clear;
		if (v == ".close") return xpilot::CommandOptions::Close;
		if (v == ".closeall") return xpilot::CommandOptions::CloseAll;
		return xpilot::CommandOptions::None;
	}

	TextMessageConsole::TextMessageConsole(XPilot* instance) :
		XPImgWindow(WND_MODE_FLOAT_CENTERED, WND_STYLE_SOLID, WndRect(0, 200, 600, 0)),
		m_scrollToBottom(false),
		m_env(instance)
	{
		SetWindowResizingLimits(300, 100, 1024, 1024);
		SetWindowTitle("Text Message Console");
	}

	void TextMessageConsole::SendRadioMessage(const std::string& message)
	{
		if (m_env->isNetworkConnected())
		{
			json msg;
			msg["type"] = "RadioMessageSent";
			msg["data"]["message"] = message;
			m_env->SendReply(msg.dump());
		}
	}

	void TextMessageConsole::RadioMessageReceived(std::string msg, double red, double green, double blue)
	{
		if (!msg.empty())
		{
			ConsoleMessage m;
			m.setMessage(string_format("[%s] %s", UtcTimestamp().c_str(), msg.c_str()));
			m.setRed(red);
			m.setGreen(green);
			m.setBlue(blue);
			m_messageHistory.push_back(m);
			m_scrollToBottom = true;
		}
	}

	void TextMessageConsole::ShowErrorMessage(std::string error)
	{
		ConsoleMessage m;
		m.setMessage(string_format("[%s] %s", UtcTimestamp().c_str(), error.c_str()));
		m.setRed(192);
		m.setGreen(57);
		m.setBlue(43);
		m_messageHistory.push_back(m);
		m_scrollToBottom = true;
	}

	void PrivateMessageError(std::string tabName, std::string error)
	{
		ConsoleMessage m;
		m.setMessage(string_format("[%s] %s", UtcTimestamp().c_str(), error.c_str()));
		m.setRed(192);
		m.setGreen(57);
		m.setBlue(43);

		auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&tabName](const Tab& t)
		{
			return t.tabName == tabName;
		});

		if (it != m_tabs.end())
		{
			it->scrollToBottom = true;
		}
	}

	void TextMessageConsole::SendPrivateMessage(const std::string& tabName, const std::string& message)
	{
		if (!tabName.empty() && !message.empty())
		{
			if (m_env->isNetworkConnected())
			{
				json msg;
				msg["type"] = "PrivateMessageSent";
				msg["data"]["to"] = tabName;
				msg["data"]["message"] = message;
				m_env->SendReply(msg.dump());
			}
		}
	}

	void TextMessageConsole::CreateNonExistingTab(const std::string& tabName)
	{
		auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&tabName](const Tab& t)
		{
			return t.tabName == tabName;
		});

		if (it == m_tabs.end())
		{
			Tab tab;
			tab.tabName = tabName;
			tab.isOpen = true;
			tab.messageHistory = std::list<ConsoleMessage>();
			m_tabs.push_back(tab);
		}
	}

	void TextMessageConsole::HandlePrivateMessage(const std::string& recipient, const std::string& msg, ConsoleTabType tabType)
	{
		switch (tabType)
		{
		case ConsoleTabType::Sent:
		{
			ConsoleMessage m;
			m.setMessage(string_format("[%s] %s: %s", UtcTimestamp().c_str(), m_env->ourCallsign().c_str(), msg.c_str()));
			m.setRed(0);
			m.setGreen(255);
			m.setBlue(255);

			auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&recipient](const Tab& t)
				{
					return t.tabName == recipient;
				});

			if (it != m_tabs.end())
			{
				it->messageHistory.push_back(m);
				it->scrollToBottom = true;
			}
			else
			{
				CreateNonExistingTab(recipient);
				HandlePrivateMessage(recipient, msg, ConsoleTabType::Sent);
			}
		}
		break;
		case ConsoleTabType::Received:
		{
			ConsoleMessage m;
			m.setMessage(string_format("[%s] %s: %s", UtcTimestamp().c_str(), recipient.c_str(), msg.c_str()));
			m.setRed(255);
			m.setGreen(255);
			m.setBlue(255);

			auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&recipient](const Tab& t)
				{
					return t.tabName == recipient;
				});

			if (it != m_tabs.end())
			{
				it->messageHistory.push_back(m);
				it->scrollToBottom = true;
			}
			else
			{
				CreateNonExistingTab(recipient);
				HandlePrivateMessage(recipient, msg, ConsoleTabType::Received);
			}
		}
		break;
		}
	}

	void CloseTab(const std::string& tabName)
	{
		auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&tabName](const Tab& t)
		{
			return t.tabName == tabName;
		});
		if (it != m_tabs.end())
		{
			m_tabs.erase(it);
		}
	}

	void TextMessageConsole::buildInterface()
	{
		ImGui::PushFont(0);

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_NoTooltip))
		{
			if (ImGui::BeginTabItem("Messages"))
			{
				ImGui::BeginChild("##Messages", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
				{
					for (auto& e : m_messageHistory)
					{
						const ImVec4& color = ImVec4(e.getRed(), e.getGreen(), e.getBlue(), 1.0f);
						ImGui::PushStyleColor(ImGuiCol_Text, color);
						ImGui::TextWrapped(e.getMessage().c_str());
						ImGui::PopStyleColor();
					}
					if (m_scrollToBottom)
					{
						ImGui::SetScrollHereY(1.0f);
						m_scrollToBottom = false;
					}
				}
				ImGui::EndChild();
				ImGui::PushItemWidth(-1.0f);
				if (ImGui::InputTextStd("##MessagesInput", &m_inputValue, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (!m_inputValue.empty())
					{
						std::vector<std::string> args;
						tokenize(m_inputValue, args, " ", true);
						if (args.size() > 0)
						{
							switch (resolveOption(args.at(0)))
							{
							case xpilot::CommandOptions::Chat:
								if (!m_env->isNetworkConnected()) 
								{
									ShowErrorMessage("Not connected to network.");
								}
								else
								{
									if (args.size() >= 2)
									{
										if (args.size() >= 3)
										{
											std::string m;
											join(args, ' ', m);

											CreateNonExistingTab(str_toupper(args.at(1)));
											SendPrivateMessage(str_toupper(args.at(1)), m);
										}
										else
										{
											CreateNonExistingTab(str_toupper(args.at(1)));
										}
										m_inputValue = "";
									}
									else
									{
										ShowErrorMessage("Invalid parameters. To open a new chat tab, use the command .chat <callsign> <message>");
										m_inputValue = "";
									}
								}
								break;
							case xpilot::CommandOptions::RequestAtis:
								if (!m_env->isNetworkConnected())
								{
									ShowErrorMessage("Not connected to the network.");
								}
								else
								{
									if (args.size() == 2)
									{
										m_env->requestStationInfo(args.at(1));
										m_inputValue = "";
									}
									else
									{
										ShowErrorMessage("Invalid parameters. To request an ATIS, use the command .atis <callsign>");
									}
								}
								break;
							case xpilot::CommandOptions::MetarRequest:
								if (!m_env->isNetworkConnected())
								{
									ShowErrorMessage("Not connected to the network.");
								}
								else
								{
									if (args.size() == 2)
									{
										m_env->requestMetar(args.at(1));
										m_inputValue = "";
									}
									else
									{
										ShowErrorMessage("Invalid parameters. To request a METAR, use the command .metar <station>");
									}
								}
								break;
							case xpilot::CommandOptions::SetRadioFrequency:
								if (args.size() == 2)
								{
									const std::regex regex("^1\\d\\d[\\.\\,]\\d{1,3}$");
									if (args.at(0) == ".com1")
									{
										if (!std::regex_match(args.at(1), regex))
										{
											ShowErrorMessage("Invalid frequency format.");
										}
										else
										{
											std::string freq(args.at(1));
											int freqInt = std::stod(freq) * 1000000;
											if (freqInt < 118000000 || freqInt > 136975000)
											{
												ShowErrorMessage("Invalid frequency range.");
												return;
											}
											m_env->setCom1Frequency(freqInt / 1000);
											m_inputValue = "";
										}
									}
									else if (args.at(0) == ".com2")
									{
										if (!std::regex_match(args.at(1), regex))
										{
											ShowErrorMessage("Invalid frequency format.");
										}
										else
										{
											std::string freq(args.at(1));
											int freqInt = std::stod(freq) * 1000000;
											if (freqInt < 118000000 || freqInt > 136975000)
											{
												ShowErrorMessage("Invalid frequency range.");
												return;
											}
											m_env->setCom2Frequency(freqInt / 1000);
											m_inputValue = "";
										}
									}
								}
								else
								{
									ShowErrorMessage("Invalid command format. To change the radio frequency, use the command .com1 123.45 or .com2 123.45");
								}
								break;
							case xpilot::CommandOptions::OverrideTx:
								if (args.size() == 2)
								{
									if (str_tolower(args.at(1)) != "com1" && str_tolower(args.at(1)) != "com2")
									{
										ShowErrorMessage("Invalid command parameters. Expected .tx com<n>. For example, .tx com1");
										return;
									}
									int radio = str_tolower(args.at(1)) == "com1" ? 1 : 2;
									m_env->setAudioComSelection(radio);
									m_inputValue = "";
								}
								else
								{
									ShowErrorMessage("Invalid command parameters. Expected .tx com<n>. For example, .tx com1");
								}
								break;
							case xpilot::CommandOptions::OverrideRx:
								if (args.size() == 3)
								{
									if (str_tolower(args.at(1)) != "com1" && str_tolower(args.at(1)) != "com2")
									{
										ShowErrorMessage("Invalid command parameters. Expected .rx com<n> on|off. For example, .rx com1 on");
										return;
									}
									if (str_tolower(args.at(2)) != "on" && str_tolower(args.at(2)) != "off")
									{
										ShowErrorMessage("Invalid command parameters. Expected .rx com<n> on|off. For example, .rx com1 on");
										return;
									}
									int radio = str_tolower(args.at(1)) == "com1" ? 1 : 2;
									bool on = str_tolower(args.at(2)) == "on" ? true : false;
									m_env->setAudioSelection(radio, on);
									m_inputValue = "";
								}
								else
								{
									ShowErrorMessage("Invalid command parameters. Expected .rx com<n> on|off. For example, .rx com1 on");
								}
								break;
							case xpilot::CommandOptions::Clear:
								m_messageHistory.clear();
								m_inputValue = "";
								break;
							case xpilot::CommandOptions::CloseAll:
								m_tabs.clear();
								m_inputValue = "";
								break;
							case xpilot::CommandOptions::Close:
							default:
								if (!m_env->isNetworkConnected())
								{
									ShowErrorMessage("Not connected to network.");
								}
								else
								{
									SendRadioMessage(m_inputValue);
									m_inputValue = "";
								}
								break;
							}
						}
					}
				}
				if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
				{
					m_inputValue = "";
				}
				ImGui::EndTabItem();
			}

			for (auto it = m_tabs.begin(); it != m_tabs.end(); ++it)
			{
				if (!it->isOpen)
				{
					m_tabs.erase(it);
				}
			}

			for (auto it = m_tabs.begin(); it != m_tabs.end(); ++it)
			{
				std::string key = it->tabName;

				if (ImGui::BeginTabItem(key.c_str(), &it->isOpen))
				{
					ImGui::BeginChild(key.c_str(), ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
					{
						for (auto& e : it->messageHistory)
						{
							const ImVec4& color = ImVec4(e.getRed(), e.getGreen(), e.getBlue(), 1.0f);
							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::TextWrapped(e.getMessage().c_str());
							ImGui::PopStyleColor();
						}
						if (it->scrollToBottom)
						{
							ImGui::SetScrollHereY(1.0f);
							it->scrollToBottom = false;
						}
					}
					ImGui::EndChild();
					ImGui::PushID(key.c_str());
					ImGui::PushItemWidth(-1.0f);
					if (ImGui::InputTextStd("##Input", &it->textInput, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (!it->textInput.empty())
						{
							std::vector<std::string> args;
							tokenize(it->textInput, args, " ", true);
							if (args.size() > 0)
							{
								switch (resolveOption(args.at(0)))
								{
								case xpilot::CommandOptions::Clear:
									it->messageHistory.clear();
									it->textInput = "";
									break;
								case xpilot::CommandOptions::Close:
									it->textInput = "";
									CloseTab(key);
									break;
								default:
									if (!m_env->isNetworkConnected())
									{
										PrivateMessageError(it->tabName, "Not connected to network.");
									}
									else
									{
										SendPrivateMessage(it->tabName, it->textInput);
										it->textInput = "";
									}
									break;
								}
							}
						}
					}
					if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
					{
						m_inputValue = "";
					}
					ImGui::PopID();
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
			ImGui::PopFont();
		}
	}
}

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

#include <string>
#include "imfilebrowser.h"
#include "imgui_stdlib.h"
#include "XPImgWindow.h"
#include "Utilities.h"
#include "Config.h"
#include "SettingsWindow.h"
#include "XPMPMultiplayer.h"

namespace xpilot {
	static std::string cslPaths[7];
	static bool pathsEnabled[7];
	static int selectedPathIdx;
	static bool showHideLabels;
	static bool debugModelMatching;
	static int logLevel;
	static std::string fallbackTypeCode;
	static bool overrideContactAtcCommand;
	static bool showMessagePreview = true;
	static int notificationPanelTimeout = 2;
	static int notificationPanelPosition = (int)NotificationPanelPosition::TopRight;
	static int labelMaxDistance = 3;
	static bool labelVisibilityCutoff = true;
	static bool enableTransmitIndicator = false;
	static bool enableAircraftSounds = true;
	static int aircraftSoundVolume = 50;
	static float lblCol[4];
	ImGui::FileBrowser fileBrowser(ImGuiFileBrowserFlags_SelectDirectory);

	static int nodeToClose = -1;
	static int currentNode = -1;

	SettingsWindow::SettingsWindow(WndMode _mode) :
		XPImgWindow(_mode, WND_STYLE_SOLID, WndRect(0, 370, 600, 0)) {
		SetWindowTitle(string_format("xPilot %s Settings", PLUGIN_VERSION_STRING));
		SetWindowResizingLimits(600, 370, 600, 370);

		fileBrowser.SetTitle("Browse...");
		fileBrowser.SetWindowSize(450, 250);
	}

	void SettingsWindow::LoadConfig() {
		auto paths = xpilot::Config::GetInstance().GetCSLPackages();
		for (int i = 0; i < 7; i++) {
			if (i < paths.size()) {
				if (!paths[i].path.empty()) {
					cslPaths[i] = paths[i].path;
					pathsEnabled[i] = paths[i].enabled;
				}
			}
		}

		debugModelMatching = xpilot::Config::GetInstance().GetDebugModelMatching();
		showHideLabels = xpilot::Config::GetInstance().GetShowHideLabels();
		fallbackTypeCode = xpilot::Config::GetInstance().GetDefaultAcIcaoType();
		showMessagePreview = xpilot::Config::GetInstance().GetNotificationPanelVisible();
		notificationPanelTimeout = xpilot::Config::GetInstance().GetNotificationPanelTimeout();
		notificationPanelPosition = (int)xpilot::Config::GetInstance().GetNotificationPanelPosition();
		overrideContactAtcCommand = xpilot::Config::GetInstance().GetOverrideContactAtcCommand();
		labelMaxDistance = xpilot::Config::GetInstance().GetMaxLabelDistance();
		labelVisibilityCutoff = xpilot::Config::GetInstance().GetLabelCutoffVis();
		logLevel = xpilot::Config::GetInstance().GetLogLevel();
		enableTransmitIndicator = xpilot::Config::GetInstance().GetTransmitIndicatorEnabled();
		enableAircraftSounds = xpilot::Config::GetInstance().GetAircraftSoundsEnabled();
		aircraftSoundVolume = xpilot::Config::GetInstance().GetAircraftSoundVolume();
		HexToRgb(xpilot::Config::GetInstance().GetAircraftLabelColor(), lblCol);
	}

	void Save() {
		if (!xpilot::Config::GetInstance().SaveConfig()) {
			ImGui::OpenPopup("Error Saving Settings");
		}
	}

	void SettingsWindow::buildInterface() {
		LoadConfig();
		ImGui::PushFont(0);

		struct TextFilters
		{
			static int FilterNumbersOnly(ImGuiInputTextCallbackData* data) {
				if (data->EventChar >= '0' && data->EventChar <= '9')
					return 0;
				return 1;
			}
		};

		if (nodeToClose == 0) {
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
			nodeToClose = -1;
		}

		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (currentNode == 0) {
				if (ImGui::BeginTable("##Settings", 2, ImGuiTableFlags_BordersInnerH)) {
					ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 315);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Automatically Show Notification Panel");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "If enabled, new messages will automatically appear at the top right of the X-Plane window.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##AutoShowMessagePreview", &showMessagePreview)) {
						xpilot::Config::GetInstance().SetNotificationPanelVisible(showMessagePreview);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Hide Notification Panel After...");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Automatically hide the Notification Panel after the selected number of seconds.");
					ImGui::TableSetColumnIndex(1);
					const float cbWidth = ImGui::CalcTextSize("5 Seconds_________").x;
					ImGui::SetNextItemWidth(cbWidth);
					const char* autoHideOptions[] = { "5 seconds", "10 seconds", "15 seconds", "30 seconds", "60 seconds" };
					if (ImGui::Combo("##AutoHide", &notificationPanelTimeout, autoHideOptions, IM_ARRAYSIZE(autoHideOptions))) {
						xpilot::Config::GetInstance().SetNotificationPanelTimeout(notificationPanelTimeout);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Notification Panel Position");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Specify where the notification panel should appear within the main X-Plane window.");
					ImGui::TableSetColumnIndex(1);
					const float notificationPanelCbWidth = ImGui::CalcTextSize("Top Right__________").x;
					ImGui::SetNextItemWidth(notificationPanelCbWidth);
					const char* positionOptions[] = { "Top Right", "Top Left", "Bottom Left", "Bottom Right" };
					if (ImGui::Combo("##NotificationPanelPosition", &notificationPanelPosition, positionOptions, IM_ARRAYSIZE(positionOptions))) {
						xpilot::Config::GetInstance().SetNotificationPanelPosition((NotificationPanelPosition)notificationPanelPosition);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Show Callsign Labels");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Enable this option to show callsign above other aircraft.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##ShowAircraftLabels", &showHideLabels)) {
						XPMPEnableAircraftLabels(showHideLabels);
						xpilot::Config::GetInstance().SetShowHideLabels(showHideLabels);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Callsign Label Color");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "The color of the aircraft callsign labels, if enabled. Choose a custom or pre-defined color.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::ColorButton("Click to Pick Label Color", ImVec4(lblCol[0], lblCol[1], lblCol[2], lblCol[3]), ImGuiColorEditFlags_NoAlpha)) {
						ImGui::OpenPopup("Label Color Picker");
					}
					if (ImGui::BeginPopup("Label Color Picker")) {
						if (ImGui::ColorPicker3("Label Color Picker", lblCol,
							ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoLabel |
							ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoSidePreview)) {
							const int col = (int)((lround(lblCol[0] * 255.0f) << 16)
								+ (lround(lblCol[1] * 255.0f) << 8)
								+ (lround(lblCol[2] * 255.0f) << 0));

							xpilot::Config::GetInstance().SetAircraftLabelColor(col);
							Save();
						}
						ImGui::EndPopup();
					}
					ImGui::SameLine();
					ImGui::TextUnformatted("Or Choose Color:");
					ImGui::SameLine();
					if (ImGui::ColorButton("Yellow", ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoTooltip)) {
						HexToRgb(COLOR_YELLOW, lblCol);
						xpilot::Config::GetInstance().SetAircraftLabelColor(COLOR_YELLOW);
						Save();
					}
					ImGui::SameLine();
					if (ImGui::ColorButton("Red", ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoTooltip)) {
						HexToRgb(COLOR_RED, lblCol);
						xpilot::Config::GetInstance().SetAircraftLabelColor(COLOR_RED);
						Save();
					}
					ImGui::SameLine();
					if (ImGui::ColorButton("Green", ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoTooltip)) {
						HexToRgb(COLOR_GREEN, lblCol);
						xpilot::Config::GetInstance().SetAircraftLabelColor(COLOR_GREEN);
						Save();
					}
					ImGui::SameLine();
					if (ImGui::ColorButton("Blue", ImVec4(0.0f, 0.94f, 0.94f, 1.0f), ImGuiColorEditFlags_NoTooltip)) {
						HexToRgb(COLOR_BLUE, lblCol);
						xpilot::Config::GetInstance().SetAircraftLabelColor(COLOR_BLUE);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Max Callsign Label Distance");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Specify how far away (in nautical miles) you want aircraft callsigns labels to be visible.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::SliderInt("##MaxDist", &labelMaxDistance, 1, 20, "%d nm")) {
						XPMPSetAircraftLabelDist(float(labelMaxDistance), labelVisibilityCutoff);
						xpilot::Config::GetInstance().SetMaxLabelDistance(labelMaxDistance);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Hide Callsign Labels at Visibility Distance");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Visibility can oftentimes be less than the \"Max Label Distance\" due to weather conditions.\n\nIf enabled, aircraft callsign labels will not be visible for planes beyond the current visibility range.\n\nIf disabled, callsign labels will show even if the plane is hidden behind fog or clouds.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##HideLabelsVisibility", &labelVisibilityCutoff)) {
						XPMPSetAircraftLabelDist(float(labelMaxDistance), labelVisibilityCutoff);
						xpilot::Config::GetInstance().SetLabelCutoffVis(labelVisibilityCutoff);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Default Aircraft Type ICAO");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Fallback aircraft ICAO type designator if no CSL model can be found for a plane.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::InputTextStd("##Fallback", &fallbackTypeCode, ImGuiInputTextFlags_CharsUppercase)) {
						xpilot::Config::GetInstance().SetDefaultAcIcaoType(fallbackTypeCode);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Enable Transmit Indicator");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Enable this option to see a green transmit indicator at the top left corner of your X-Plane window when you press your PTT.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##EnableTransmitIndicator", &enableTransmitIndicator)) {
						xpilot::Config::GetInstance().SetTransmitIndicatorEnabled(enableTransmitIndicator);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Enable Aircraft Engine Sounds");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Enable this option to add ambient engine sounds to other aircraft.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##EnableAircraftSounds", &enableAircraftSounds)) {
						xpilot::Config::GetInstance().SetAircraftSoundsEnabled(enableAircraftSounds);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Aircraft Engine Sound Volume");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "Use this slider to adjust the engine volume of other aircraft.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::SliderInt("##AircraftEngineVolume", &aircraftSoundVolume, 0, 100, "%d%%")) {
						xpilot::Config::GetInstance().SetAircraftSoundVolume(aircraftSoundVolume);
						Save();
					}

					ImGui::EndTable();
				}
			} else {
				nodeToClose = currentNode;
				currentNode = 0;
			}
		}

		if (nodeToClose == 1) {
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
			nodeToClose = -1;
		}

		if (ImGui::CollapsingHeader("CSL Configuration")) {
			if (currentNode == 1) {
				ImGui::Text("CSL Model Configuration");
				ImGui::Text("** You must restart X-Plane after making changes to the CSL Paths **");

				if (ImGui::BeginTable("##CSL", 4, ImGuiTableFlags_BordersInnerH)) {
					ImGui::TableSetupColumn("##Enabled", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 80);
					ImGui::TableSetupColumn("##Path", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
					ImGui::TableSetupColumn("##Browse", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 50);
					ImGui::TableSetupColumn("##Clear", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 40);

					for (int i = 0; i < 7; i++) {
						ImGui::PushID(i);
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						if (ImGui::Checkbox("Enabled", &pathsEnabled[i])) {
							xpilot::Config::GetInstance().SaveCSLEnabled(i, pathsEnabled[i]);
							Save();
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::SetNextItemWidth(-1);
						if (ImGui::InputTextStd("", &cslPaths[i])) {
							xpilot::Config::GetInstance().SaveCSLPath(i, cslPaths[i]);
							Save();
						}

						ImGui::TableSetColumnIndex(2);
						ImGui::SetNextItemWidth(-1);
						if (ImGui::Button("Browse")) {
							fileBrowser.Open();
							selectedPathIdx = i;
						}

						ImGui::TableSetColumnIndex(3);
						ImGui::SetNextItemWidth(-1);
						if (ImGui::Button("Clear")) {
							cslPaths[i] = "";
							pathsEnabled[i] = false;

							xpilot::Config::GetInstance().SaveCSLEnabled(i, pathsEnabled[i]);
							xpilot::Config::GetInstance().SaveCSLPath(i, cslPaths[i]);
							Save();
						}

						fileBrowser.Display();
						if (fileBrowser.HasSelected()) {
							cslPaths[selectedPathIdx] = fileBrowser.GetSelected().string();
							pathsEnabled[selectedPathIdx] = true;

							xpilot::Config::GetInstance().SaveCSLEnabled(selectedPathIdx, pathsEnabled[selectedPathIdx]);
							xpilot::Config::GetInstance().SaveCSLPath(selectedPathIdx, cslPaths[selectedPathIdx]);
							Save();

							fileBrowser.ClearSelected();
						}

						ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
						if (ImGui::BeginPopupModal("Error Saving Settings", NULL, ImGuiWindowFlags_NoResize)) {
							ImGui::TextWrapped("%s", "An error occured while trying to save the settings.\n\nMake sure read/write permissions are set properly for the \n\"Resources > Plugins > xPilot > Resources\" folder.\n\n");

							if (ImGui::Button("Close"))
								ImGui::CloseCurrentPopup();

							ImGui::EndPopup();
						}

						ImGui::PopID();
					}

					ImGui::EndTable();
				}
			} else {
				nodeToClose = currentNode;
				currentNode = 1;
			}
		}

		if (nodeToClose == 2) {
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
			nodeToClose = -1;
		}

		if (ImGui::CollapsingHeader("Advanced Options")) {
			if (currentNode == 2) {
				if (ImGui::BeginTable("##Settings", 2, ImGuiTableFlags_BordersInnerH)) {
					ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 315);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Override \"Contact ATC\" Command");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "If this option is enabled, xPilot will ignore the \"Contact ATC\" X-Plane Command. This is generally only useful for those who also use PilotEdge.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##OverrideContactATC", &overrideContactAtcCommand)) {
						xpilot::Config::GetInstance().SetOverrideContactAtcCommand(overrideContactAtcCommand);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Log.txt Log Level");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "This option manages the amount of information that is written to the X-Plane Log.txt.\n\n\"Debug\" will write the most information.\n\"Fatal\" will write the least amount of information.\n\nIt is recommended you only change this if you experience odd behavior and need to log additional information to provide to the developer.");
					ImGui::TableSetColumnIndex(1);
					const float logCbWidth = ImGui::CalcTextSize("Warning (default)_____").x;
					ImGui::SetNextItemWidth(logCbWidth);
					if (ImGui::Combo("##LogLevel", &logLevel, "Debug\0Info\0Warning (default)\0Error\0Fatal\0", 5)) {
						xpilot::Config::GetInstance().SetLogLevel(logLevel);
						Save();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Log Model Matching Results");
					ImGui::SameLine();
					ImGui::ButtonIcon(ICON_FA_QUESTION_CIRCLE, "If enabled, debug information will be logged to the X-Plane Log.txt about how a CSL model was chosen.\n\nOnly enable this option if you need to determine why planes aren't rendering as expected.");
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##ModelMatchingLog", &debugModelMatching)) {
						xpilot::Config::GetInstance().SetDebugModelMatching(debugModelMatching);
						Save();
					}
				}
				ImGui::EndTable();
			} else {
				nodeToClose = currentNode;
				currentNode = 2;
			}
		}

		ImGui::PopFont();
	}
}

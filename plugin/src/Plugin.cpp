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

#include "Plugin.h"
#include "XPilot.h"
#include "Config.h"
#include "Utilities.h"
#include "Constants.h"
#include "XPMPMultiplayer.h"
#include "XPLMPlugin.h"
#include "XPLMGraphics.h"

#if APL == 1
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#elif IBM == 1
#include <GL/gl.h>
#include <GL/glu.h>
#elif LIN == 1
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <memory>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if !defined(XPLM200) || !defined(XPLM210) || !defined(XPLM300) || !defined(XPLM301)
#error xPilot requires XPLM301 SDK or newer
#endif

std::unique_ptr<XPilot> environment;

static int DrawTransmitIndicator(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon) {
	XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);

	static float color[3] = { 0.19f, 0.80f, 0.19f }; // lime green

	int sx, sy;
	XPLMGetScreenSize(&sx, &sy);

	int triangleAmount = 100;
	float twicePi = 2.0f * M_PI;
	int x = 15;
	int y = sy - 15;
	int radius = 5;

	glBegin(GL_LINES);
	glLineWidth(5.0);
	glColor3f(color[0], color[1], color[2]);
	for (unsigned int i = 0; i <= triangleAmount; i++) {
		glVertex2f(x, y);
		glVertex2f(x + (radius * cos(i * twicePi / triangleAmount)), y + (radius * sin(i * twicePi / triangleAmount)));
	}
	glEnd();

	if (environment != nullptr) {
		XPLMDrawString(color, 28, sy - 18, environment->GetTxRadio() == 6 ? (char*)"COM1" : (char*)"COM2", NULL, xplmFont_Proportional);
	}

	return 1;
}

void ShowTransmitIndicator() {
	if (Config::GetInstance().GetTransmitIndicatorEnabled() && environment->IsNetworkConnected() && environment->IsRadiosPowered()) {
		XPLMRegisterDrawCallback(DrawTransmitIndicator, xplm_Phase_Window, 1, nullptr);
	}
}

void HideTransmitIndicator() {
	XPLMUnregisterDrawCallback(DrawTransmitIndicator, xplm_Phase_Window, 1, nullptr);
}

PLUGIN_API int XPluginStart(char* outName, char* outSignature, char* outDescription) {
	strncpy_s(outName, 255, string_format("%s %s", PLUGIN_NAME, PLUGIN_VERSION_STRING).c_str(), 100);
	strncpy_s(outSignature, 255, "org.vatsim.xpilot", 100);
	strncpy_s(outDescription, 255, "X-Plane pilot client for VATSIM.", 100);

	try {
		XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
		XPMPSetPluginName(PLUGIN_NAME);
		RegisterMenuItems();
		LOG_MSG(logMSG, "xPilot version %s initialized", PLUGIN_VERSION_STRING);
	}
	catch (const std::exception& e) {
		LOG_MSG(logERROR, "Exception in XPluginStart: %s", e.what());
		return 0;
	}
	catch (...) {
		return 0;
	}

	return 1;
}

PLUGIN_API int XPluginEnable(void) {
	try {
		if (environment) {
			environment.reset();
		}
		XPImgWindowInit();
		Config::GetInstance().LoadConfig();
		environment = std::make_unique<xpilot::XPilot>();
		LOG_MSG(logMSG, "xPilot plugin enabled");
	}
	catch (std::exception& e) {
		LOG_MSG(logERROR, "Exception in XPluginEnable: %s", e.what());
		return 0;
	}
	catch (...) {
		return 0;
	}

	return 1;
}

PLUGIN_API void XPluginDisable(void) {
	try {
		environment->DeleteAllAircraft();
		environment->Shutdown();
		environment.reset();
		XPMPMultiplayerDisable();
		XPMPMultiplayerCleanup();
		LOG_MSG(logMSG, "xPilot plugin disabled");
	}
	catch (std::exception& e) {
		LOG_MSG(logERROR, "Exception in XPluginDisable: %s", e.what());
	}
	catch (...) {
	}
}

PLUGIN_API void XPluginStop(void) {
	try {
		XPImgWindowCleanup();
		XPLMDestroyMenu(PluginMenu);
		XPLMUnregisterCommandHandler(PttCommand, PttCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ToggleMessgePreviewPanelCommnd, ToggleMessagePreviewPanelCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ToggleNearbyATCWindowCommand, ToggleNearbyATCWindowCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ToggleMessageConsoleCommand, ToggleMessageConsoleCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ContactAtcCommand, ContactAtcCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ToggleDefaultAtisCommand, ToggleDefaultAtisCommandHandler, 0, 0);
		XPLMUnregisterCommandHandler(ToggleTcasCommand, ToggleTcasCommandHandler, 0, 0);
	}
	catch (const std::exception& e) {
		LOG_MSG(logERROR, "Exception in XPluginStop: %s", e.what());
	}
	catch (...) {
	}
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int msg, void* inParam) {

}

int ContactAtcCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (xpilot::Config::GetInstance().GetOverrideContactAtcCommand()) {
		if (inPhase == xplm_CommandContinue) {
			ShowTransmitIndicator();
			environment->SetPttActive(1);
		} else if (inPhase == xplm_CommandEnd) {
			HideTransmitIndicator();
			environment->SetPttActive(0);
		}
		return 0;
	}
	return 1;
}

int  PttCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandContinue) {
		ShowTransmitIndicator();
		environment->SetPttActive(1);
	} else if (inPhase == xplm_CommandEnd) {
		HideTransmitIndicator();
		environment->SetPttActive(0);
	}
	return 0;
}

int ToggleNearbyATCWindowCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		environment->ToggleNearbyAtcWindow();
	}
	return 0;
}

int ToggleMessagePreviewPanelCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		environment->SetNotificationPanelAlwaysVisible(!environment->GetNotificationPanelAlwaysVisible());
	}
	return 0;
}

int ToggleDefaultAtisCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		environment->DisableXplaneAtis(environment->IsXplaneAtisDisabled());
	}
	return 0;
}

int ToggleTcasCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		if (XPMPHasControlOfAIAircraft()) {
			environment->ReleaseTcasControl();
		} else {
			environment->TryGetTcasControl();
		}
	}
	return 0;
}

int ToggleMessageConsoleCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		environment->ToggleTextMessageConsole();
	}
	return 0;
}

int ToggleAircraftLabelsCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon) {
	if (inPhase == xplm_CommandEnd) {
		bool enabled = !xpilot::Config::GetInstance().GetShowHideLabels();
		xpilot::Config::GetInstance().SetShowHideLabels(enabled);
		xpilot::Config::GetInstance().SaveConfig();
		XPMPEnableAircraftLabels(enabled);
	}
	return 0;
}

void MenuHandler(void* mRef, void* iRef) {
	if (!strcmp((const char*)iRef, "Settings")) {
		environment->ToggleSettingsWindow();
	}
}

void RegisterMenuItems() {
	PttCommand = XPLMCreateCommand("xpilot/ptt", "xPilot: Radio Push-to-Talk (PTT)");
	XPLMRegisterCommandHandler(PttCommand, PttCommandHandler, 1, (void*)0);

	ToggleMessageConsoleCommand = XPLMCreateCommand("xpilot/toggle_text_message_console", "xPilot: Toggle Text Message Console");
	XPLMRegisterCommandHandler(ToggleMessageConsoleCommand, ToggleMessageConsoleCommandHandler, 1, (void*)0);

	ToggleMessgePreviewPanelCommnd = XPLMCreateCommand("xpilot/toggle_notification_panel", "xPilot: Toggle Notification Panel");
	XPLMRegisterCommandHandler(ToggleMessgePreviewPanelCommnd, ToggleMessagePreviewPanelCommandHandler, 1, (void*)0);

	ToggleNearbyATCWindowCommand = XPLMCreateCommand("xpilot/toggle_nearby_atc", "xPilot: Toggle Nearby ATC Window");
	XPLMRegisterCommandHandler(ToggleNearbyATCWindowCommand, ToggleNearbyATCWindowCommandHandler, 1, (void*)0);

	ToggleDefaultAtisCommand = XPLMCreateCommand("xpilot/toggle_default_atis", "xPilot: Toggle Default X-Plane ATIS");
	XPLMRegisterCommandHandler(ToggleDefaultAtisCommand, ToggleDefaultAtisCommandHandler, 1, (void*)0);

	ToggleTcasCommand = XPLMCreateCommand("xpilot/toggle_tcas", "xPilot: Toggle TCAS Control");
	XPLMRegisterCommandHandler(ToggleTcasCommand, ToggleTcasCommandHandler, 1, (void*)0);

	ToggleAircraftLabelsCommand = XPLMCreateCommand("xpilot/toggle_aircraft_labels", "xPilot: Toggle Aircraft Labels");
	XPLMRegisterCommandHandler(ToggleAircraftLabelsCommand, ToggleAircraftLabelsCommandHandler, 1, (void*)0);

	XPLMRegisterCommandHandler(ContactAtcCommand, ContactAtcCommandHandler, 1, (void*)0);

	PluginMenuIdx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "xPilot", nullptr, 0);
	PluginMenu = XPLMCreateMenu("xPilot", XPLMFindPluginsMenu(), PluginMenuIdx, MenuHandler, nullptr);

	MenuSettings = XPLMAppendMenuItem(PluginMenu, "Settings", (void*)"Settings", 0);
	MenuNearbyAtc = XPLMAppendMenuItemWithCommand(PluginMenu, "Nearby ATC", ToggleNearbyATCWindowCommand);
	MenuTextMessageConsole = XPLMAppendMenuItemWithCommand(PluginMenu, "Text Message Console", ToggleMessageConsoleCommand);
	MenuNotificationPanel = XPLMAppendMenuItemWithCommand(PluginMenu, "Notification Panel", ToggleMessgePreviewPanelCommnd);
	MenuDefaultAtis = XPLMAppendMenuItemWithCommand(PluginMenu, "Default ATIS", ToggleDefaultAtisCommand);
	MenuToggleTcas = XPLMAppendMenuItemWithCommand(PluginMenu, "TCAS", ToggleTcasCommand);
	MenuToggleAircraftLabels = XPLMAppendMenuItemWithCommand(PluginMenu, "Aircraft Labels", ToggleAircraftLabelsCommand);
}

void UpdateMenuItems() {
	XPLMSetMenuItemName(PluginMenu, MenuDefaultAtis, environment->IsXplaneAtisDisabled() ? "X-Plane ATIS: Disabled" : "X-Plane ATIS: Enabled", 0);
	XPLMSetMenuItemName(PluginMenu, MenuToggleTcas, XPMPHasControlOfAIAircraft() ? "Release TCAS Control" : "Request TCAS Control", 0);
	XPLMSetMenuItemName(PluginMenu, MenuToggleAircraftLabels, xpilot::Config::GetInstance().GetShowHideLabels() ? "Aircraft Labels: On" : "Aircraft Labels: Off", 0);
}

#ifdef _WIN32
#include <Windows.h>
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	return TRUE;
}
#endif // _WIN32
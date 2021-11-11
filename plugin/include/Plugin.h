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

#ifndef Plugin_h
#define Plugin_h

#include <stddef.h>

#include "XPLMPlugin.h"
#include "XPLMMenus.h"

void RegisterMenuItems();
void UpdateMenuItems();

inline XPLMCommandRef PttCommand = NULL;
inline int PttCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleSettingsCommand = NULL;
inline int ToggleSettingsCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleNearbyATCWindowCommand = NULL;
inline int ToggleNearbyATCWindowCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleMessgePreviewPanelCommnd = NULL;
inline int ToggleMessagePreviewPanelCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleDefaultAtisCommand = NULL;
inline int ToggleDefaultAtisCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleMessageConsoleCommand = NULL;
inline int ToggleMessageConsoleCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleTcasCommand = NULL;
inline int ToggleTcasCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ToggleAircraftLabelsCommand = NULL;
inline int ToggleAircraftLabelsCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline XPLMCommandRef ContactAtcCommand = XPLMFindCommand("sim/operation/contact_atc");
inline int ContactAtcCommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon);

inline int PluginMenuIdx;
inline XPLMMenuID PluginMenu;
static int MenuPreferences = 0;
static int MenuNearbyAtc = 0;
static int MenuDefaultAtis = 0;
static int MenuTextMessageConsole = 0;
static int MenuNotificationPanel = 0;
static int MenuToggleTcas = 0;
static int MenuToggleAircraftLabels = 0;

#endif // !Plugin_h

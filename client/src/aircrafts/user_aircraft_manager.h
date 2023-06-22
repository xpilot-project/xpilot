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

#ifndef USER_AIRCRAFT_MANAGER_H
#define USER_AIRCRAFT_MANAGER_H

#include <optional>

#include <QObject>
#include <QTimer>

#include "network/networkmanager.h"
#include "simulator/xplane_adapter.h"
#include "aircrafts/user_aircraft_data.h"
#include "aircrafts/user_aircraft_config_data.h"
#include "aircrafts/radio_stack_state.h"
#include "aircrafts/aircraft_configuration.h"
#include "qinjection/dependencypointer.h"

using namespace xpilot;

class UserAircraftManager : public QObject
{
    Q_OBJECT
public:
    UserAircraftManager(QObject* parent = nullptr);

private:
    void OnUserAircraftConfigDataUpdated(UserAircraftConfigData data);
    void OnRadioStackUpdated(RadioStackState radioStack);
    void OnAircraftConfigurationInfoReceived(QString from, QString json);

private:
    NetworkManager& m_networkManager;
    XplaneAdapter& m_xplaneAdapter;

    QTimer m_tokenRefreshTimer;
    const int AcconfigTokenRefreshInterval = 5000;
    const int AcconfigMaxTokens = 10;
    int m_tokensAvailable = AcconfigMaxTokens;

    UserAircraftConfigData m_userAircraftConfigData;
    RadioStackState m_radioStackState;
    std::optional<AircraftConfiguration> m_lastBroadcastConfig;
    bool m_airborne = false;
    bool m_simConnected = false;
    bool m_initialAircraftDataReceived = false;
};

#endif

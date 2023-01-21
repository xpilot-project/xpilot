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

#include <QTimer>

#include "user_aircraft_manager.h"
#include "src/config/appconfig.h"

using namespace xpilot;

UserAircraftManager::UserAircraftManager(QObject* parent) :
    QObject(parent),
    m_xplaneAdapter(*QInjection::Pointer<XplaneAdapter>().data()),
    m_networkManager(*QInjection::Pointer<NetworkManager>().data())
{
    connect(&m_xplaneAdapter, &XplaneAdapter::userAircraftConfigDataChanged, this, &UserAircraftManager::OnUserAircraftConfigDataUpdated);
    connect(&m_xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, &UserAircraftManager::OnRadioStackUpdated);
    connect(&m_xplaneAdapter, &XplaneAdapter::simConnectionStateChanged, this, [&](bool connected) {
        if(!connected) {
            m_initialAircraftDataReceived = false;
        }
        m_simConnected = connected;
    });
    connect(&m_networkManager, &NetworkManager::aircraftConfigurationInfoReceived, this, &UserAircraftManager::OnAircraftConfigurationInfoReceived);
    connect(&m_tokenRefreshTimer, &QTimer::timeout, this, [&] {
        if(m_tokensAvailable < AcconfigMaxTokens) {
            m_tokensAvailable++;
        }
    });
    m_tokenRefreshTimer.start(AcconfigTokenRefreshInterval);
}

void UserAircraftManager::OnUserAircraftConfigDataUpdated(UserAircraftConfigData data)
{
    if(m_userAircraftConfigData != data)
    {
        m_userAircraftConfigData = data;
    }

    if(!m_lastBroadcastConfig.has_value())
    {
        m_lastBroadcastConfig = AircraftConfiguration::FromUserAircraftData(m_userAircraftConfigData);
    }
    else if(m_tokensAvailable > 0)
    {
        AircraftConfiguration newCfg = AircraftConfiguration::FromUserAircraftData(m_userAircraftConfigData);
        if(newCfg != m_lastBroadcastConfig.value()) {
            AircraftConfiguration incremental = m_lastBroadcastConfig->CreateIncremental(newCfg);
            m_networkManager.SendAircraftConfigurationUpdate(incremental);
            m_lastBroadcastConfig = newCfg;
            m_tokensAvailable--;
        }
    }

    if(m_simConnected && m_initialAircraftDataReceived)
    {
        bool wasAirborne = m_airborne;
        m_airborne = !m_userAircraftConfigData.OnGround;
        if(!wasAirborne && m_airborne && !m_radioStackState.SquawkingModeC && AppConfig::getInstance()->AutoModeC)
        {
            m_xplaneAdapter.transponderModeToggle();
        }
    }

    m_initialAircraftDataReceived = true;
}

void UserAircraftManager::OnRadioStackUpdated(RadioStackState radioStack)
{
    m_radioStackState = radioStack;
}

void UserAircraftManager::OnAircraftConfigurationInfoReceived(QString from, QString json)
{
    auto acconfig = AircraftConfigurationInfo::FromJson(json);

    if(acconfig.FullRequest.has_value() && acconfig.FullRequest.value())
    {
        AircraftConfiguration cfg = AircraftConfiguration::FromUserAircraftData(m_userAircraftConfigData);
        cfg.IsFullData = true;
        m_networkManager.SendAircraftConfigurationUpdate(from, cfg);
    }
}

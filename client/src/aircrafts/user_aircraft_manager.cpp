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

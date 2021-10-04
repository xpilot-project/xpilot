#include <QTimer>
#include "user_aircraft_manager.h"

using namespace xpilot;

UserAircraftManager::UserAircraftManager(UdpClient& udpClient, NetworkManager& networkManager, QObject* parent) :
    QObject(parent),
    m_networkManager(networkManager)
{
    connect(&udpClient, &UdpClient::userAircraftDataChanged, this, &UserAircraftManager::OnUserAircraftDataUpdated);
    connect(&m_networkManager, &NetworkManager::aircraftConfigurationInfoReceived, this, &UserAircraftManager::OnAircraftConfigurationInfoReceived);

    QTimer* tokenRefreshTimer = new QTimer(this);
    connect(tokenRefreshTimer, &QTimer::timeout, this, [=] {
        if(m_tokensAvailable < AcconfigMaxTokens) {
            m_tokensAvailable++;
        }
    });
    tokenRefreshTimer->start(AcconfigTokenRefreshInterval);
}

void UserAircraftManager::OnUserAircraftDataUpdated(UserAircraftData data)
{
    if(m_userAircraftData != data)
    {
        m_userAircraftData = data;
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
        bool wasAirborne = m_airborne;
        m_airborne = !m_userAircraftConfigData.OnGround;
    }
}

void UserAircraftManager::OnAircraftConfigurationInfoReceived(QString from, QString json)
{
    AircraftConfigurationInfo info;
    info.FromJson(json);

    if(info.IsFullRequest)
    {
        AircraftConfiguration cfg = AircraftConfiguration::FromUserAircraftData(m_userAircraftConfigData);
        cfg.IsFullData = true;
        m_networkManager.SendAircraftConfigurationUpdate(from, cfg);
    }
}

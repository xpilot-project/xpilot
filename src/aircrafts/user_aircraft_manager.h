#ifndef USER_AIRCRAFT_MANAGER_H
#define USER_AIRCRAFT_MANAGER_H

#include <QObject>
#include <optional>

#include "src/network/networkmanager.h"
#include "src/simulator/udpclient.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/aircraft_configuration.h"

using namespace xpilot;

class UserAircraftManager : public QObject
{
    Q_OBJECT
public:
    UserAircraftManager(UdpClient& udpClient, NetworkManager& networkManager, QObject* parent = nullptr);

private:
    void OnUserAircraftDataUpdated(UserAircraftData data);
    void OnAircraftConfigurationInfoReceived(QString from, QString json);

private:
    NetworkManager& m_networkManager;

    const int AcconfigTokenRefreshInterval = 5000;
    const int AcconfigMaxTokens = 10;
    int m_tokensAvailable = AcconfigMaxTokens;

    UserAircraftData m_userAircraftData;
    std::optional<AircraftConfiguration> m_lastBroadcastConfig;
    bool m_airborne;
};

#endif

#ifndef USER_AIRCRAFT_MANAGER_H
#define USER_AIRCRAFT_MANAGER_H

#include <QObject>
#include <QTimer>
#include <optional>

#include "src/network/networkmanager.h"
#include "src/simulator/xplane_adapter.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/radio_stack_state.h"
#include "src/aircrafts/aircraft_configuration.h"

using namespace xpilot;

class UserAircraftManager : public QObject
{
    Q_OBJECT
public:
    UserAircraftManager(XplaneAdapter& udpClient, NetworkManager& networkManager, QObject* parent = nullptr);

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

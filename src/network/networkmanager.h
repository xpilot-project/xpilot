#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QVector>

#include "connectinfo.h"
#include "src/fsd/fsd_client.h"
#include "src/simulator/udpclient.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/aircraft_configuration.h"
#include "src/network/events/radio_message_received.h"

namespace xpilot
{
    class NetworkManager : public QObject
    {
        Q_OBJECT

    public:
        NetworkManager(UdpClient& udpClient, QObject *owner = nullptr);

        Q_INVOKABLE void connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer);
        Q_INVOKABLE void disconnectFromNetwork();

        void RequestMetar(QString station);
        void RequestRealName(QString callsign);
        void RequestIsValidATC(QString callsign);
        void RequestCapabilities(QString callsign);
        void SendAircraftConfigurationUpdate(QString to, AircraftConfiguration config);
        void SendAircraftConfigurationUpdate(AircraftConfiguration config);
        void SendCapabilities(QString to);

    signals:
        void networkConnected();
        void networkDisconnected();
        void notificationPosted(int type, QString message);
        void metarReceived(QString from, QString metar);
        void controllerDeleted(QString from);
        void pilotDeleted(QString from);
        void serverMessageReceived(QString message);
        void privateMessageReceived(QString from, QString message);
        void broadcastMessageReceived(QString from, QString message);
        void radioMessageReceived(RadioMessageReceived args);
        void selcalAlertReceived(QString from, QList<uint> frequencies);
        void aircraftConfigurationInfoReceived(QString from, QString json);
        void controllerUpdateReceived(QString from, uint frequency, double lat, double lon);

    private:
        FsdClient m_fsd { this };
        QTimer* m_slowPositionTimer;
        QTimer* m_fastPositionTimer;
        UserAircraftData m_userAircraftData;
        UserAircraftConfigData m_userAircraftConfigData;
        RadioStackState m_radioStackState;
        ConnectInfo m_connectInfo{};
        QString m_publicIp;
        bool m_intentionalDisconnect =  false;
        bool m_forcedDisconnect = false;
        QString m_forcedDisconnectReason = "";
        bool m_velocityEnabled = false;

        void OnNetworkError(QString error);
        void OnNetworkConnected();
        void OnNetworkDisconnected();
        void OnServerIdentificationReceived(PDUServerIdentification pdu);
        void OnClientQueryReceived(PDUClientQuery pdu);
        void OnClientQueryResponseReceived(PDUClientQueryResponse pdu);
        void OnPilotPositionReceived(PDUPilotPosition pdu);
        void OnFastPilotPositionReceived(PDUFastPilotPosition pdu);
        void OnATCPositionReceived(PDUATCPosition pdu);
        void OnMetarResponseReceived(PDUMetarResponse pdu);
        void OnDeletePilotReceived(PDUDeletePilot pdu);
        void OnDeleteATCReceived(PDUDeleteATC pdu);
        void OnPingReceived(PDUPing pdu);
        void OnTextMessageReceived(PDUTextMessage pdu);
        void OnBroadcastMessageReceived(PDUBroadcastMessage pdu);
        void OnRadioMessageReceived(PDURadioMessage pdu);
        void OnPlaneInfoRequestReceived(PDUPlaneInfoRequest pdu);
        void OnPlaneInfoResponseReceived(PDUPlaneInfoResponse pdu);
        void OnKillRequestReceived(PDUKillRequest pdu);

        void OnUserAircraftDataUpdated(UserAircraftData data);
        void OnUserAircraftConfigDataUpdated(UserAircraftConfigData data);
        void OnRadioStackStateChanged(RadioStackState radioStack);

        void SendSlowPositionPacket();
        void SendFastPositionPacket();
        void SendEmptyFastPositionPacket();

        void OnSlowPositionTimerElapsed();
        void OnFastPositionTimerElapsed();
    };
}

#endif

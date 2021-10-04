#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTimer>
#include "connectinfo.h"
#include "src/fsd/fsd_client.h"
#include "src/simulator/udpclient.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/aircraft_configuration.h"

namespace xpilot
{
    class NetworkManager : public QObject
    {
        Q_OBJECT

    public:
        NetworkManager(UdpClient& udpClient, QObject *owner = nullptr);

        void SendAircraftConfigurationUpdate(QString to, AircraftConfiguration config);
        void SendAircraftConfigurationUpdate(AircraftConfiguration config);
        void SendCapabilities(QString to);

    signals:
        void networkConnected();
        void networkDisconnected();
        void notificationPosted(int type, QString message);
        void AircraftConfigurationInfoReceived(QString from, QString json);

    public slots:
        void connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer);
        void disconnectFromNetwork();

    private:
        FsdClient m_fsd { this };
        QTimer* m_slowPositionTimer;
        QTimer* m_fastPositionTimer;
        UserAircraftData m_userAircraftData;
        ConnectInfo m_connectInfo{};

        void OnNetworkConnected();
        void OnNetworkDisconnected();
        void OnServerIdentificationReceived(PDUServerIdentification pdu);
        void OnClientQueryReceived(PDUClientQuery pdu);
        void OnClientQueryResponseReceived(PDUClientQueryResponse pdu);
        void OnPilotPositionReceived(PDUPilotPosition pdu);
        void OnFastPilotPositionReceived(PDUFastPilotPosition pdu);
        void OnATCPositionReceived(PDUATCPosition pdu);
        void OnMetarResponseReceived(PDUMetarResponse pdu);
        void OnMetarRequestReceived(PDUMetarRequest pdu);
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
        void OnRadioStackStateChanged(RadioStackState radioStack);

        void SendSlowPositionPacket();
        void SendFastPositionPacket();
        void SendEmptyFastPositionPacket();

        void OnSlowPositionTimerElapsed();
        void OnFastPositionTimerElapsed();

        void HandleServerListDownloaded();
    };
}

#endif

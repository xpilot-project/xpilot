#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QTextStream>
#include <QFile>

#include "connectinfo.h"
#include "src/fsd/fsd_client.h"
#include "src/simulator/xplane_adapter.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/aircraft_visual_state.h"
#include "src/aircrafts/aircraft_configuration.h"
#include "src/network/events/radio_message_received.h"
#include "src/common/build_config.h"

namespace xpilot
{
    class NetworkManager : public QObject
    {
        Q_OBJECT

    public:
        NetworkManager(XplaneAdapter& xplaneAdapter, QObject *owner = nullptr);
        ~NetworkManager();

        Q_INVOKABLE void connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer);
        Q_INVOKABLE void connectTowerView(QString callsign, QString address);
        Q_INVOKABLE void disconnectFromNetwork();
        Q_INVOKABLE void sendRadioMessage(QString message);
        Q_INVOKABLE void sendPrivateMessage(QString to, QString message);
        Q_INVOKABLE void requestRealName(QString callsign);
        Q_INVOKABLE void requestControllerAtis(QString callsign);
        Q_INVOKABLE void requestMetar(QString station);
        Q_INVOKABLE void sendWallop(QString message);

        void RequestMetar(QString station);
        void RequestIsValidATC(QString callsign);
        void RequestCapabilities(QString callsign);
        void SendAircraftInfoRequest(QString callsign);
        void SendAircraftConfigurationRequest(QString callsign);
        void SendAircraftConfigurationUpdate(QString to, AircraftConfiguration config);
        void SendAircraftConfigurationUpdate(AircraftConfiguration config);
        void SendCapabilities(QString to);

    signals:
        void networkConnected(QString callsign, bool enableVoice);
        void networkDisconnected(bool forced);
        void notificationPosted(int type, QString message);
        void metarReceived(QString from, QString metar);
        void controllerDeleted(QString from);
        void pilotDeleted(QString from);
        void serverMessageReceived(QString message);
        void privateMessageReceived(QString from, QString message);
        void privateMessageSent(QString from, QString message);
        void broadcastMessageReceived(QString from, QString message);
        void wallopSent(QString message);
        void radioMessageReceived(RadioMessageReceived args);
        void selcalAlertReceived(QString from, QList<uint> frequencies);
        void aircraftConfigurationInfoReceived(QString from, QString json);
        void capabilitiesRequestReceived(QString callsign);
        void capabilitiesResponseReceived(QString callsign, QString data);
        void controllerUpdateReceived(QString from, uint frequency, double lat, double lon);
        void isValidAtcReceived(QString callsign);
        void realNameReceived(QString callsign, QString name);
        void controllerAtisReceived(QString callsign, QStringList atis);
        void slowPositionUpdateReceived(QString callsign, AircraftVisualState visualState, double groundSpeed);
        void fastPositionUpdateReceived(QString callsign, AircraftVisualState visualState, VelocityVector positionalVelocityVector, VelocityVector rotationalVelocityVector);
        void aircraftInfoReceived(QString callsign, QString equipment, QString airline);

    private:
        FsdClient m_fsd { this };
        XplaneAdapter& m_xplaneAdapter;
        QTimer m_slowPositionTimer;
        QTimer m_fastPositionTimer;
        UserAircraftData m_userAircraftData;
        UserAircraftConfigData m_userAircraftConfigData;
        RadioStackState m_radioStackState;
        ConnectInfo m_connectInfo{};
        QString m_publicIp;
        bool m_intentionalDisconnect =  false;
        bool m_forcedDisconnect = false;
        QString m_forcedDisconnectReason = "";
        QList<uint> m_transmitFreqs;
        QFile m_networkLog;
        QTextStream m_rawDataStream;
        bool m_sendEmptyFastPosition = false;

        QMap<QString, QStringList> m_mapAtisMessages;

        void OnNetworkError(QString error);
        void OnProtocolErrorReceived(PDUProtocolError error);
        void OnNetworkConnected();
        void OnNetworkDisconnected();
        void OnForceDisconnected(QString reason);
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
        void OnSendFastReceived(PDUSendFast pdu);
        void OnRawDataSent(QString data);
        void OnRawDataReceived(QString data);
        void OnSendWallop(QString message);

        void OnUserAircraftDataUpdated(UserAircraftData data);
        void OnUserAircraftConfigDataUpdated(UserAircraftConfigData data);
        void OnRadioStackStateChanged(RadioStackState radioStack);
        void OnRequestControllerInfo(QString callsign);

        void SendSlowPositionPacket();
        void SendFastPositionPacket();
        void SendEmptyFastPositionPacket();

        void OnSlowPositionTimerElapsed();
        void OnFastPositionTimerElapsed();

        const double POSITIONAL_VELOCITY_ZERO_TOLERANCE = 0.005;
        bool PositionalVelocityIsZero(UserAircraftData data)
        {
            return (abs(data.LongitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.AltitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.LatitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE);
        }
    };
}

#endif

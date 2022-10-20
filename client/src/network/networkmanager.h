#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QTextStream>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QtPromise>

using namespace QtPromise;

#include "connectinfo.h"
#include "src/fsd/fsd_client.h"
#include "src/simulator/xplane_adapter.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/aircraft_visual_state.h"
#include "src/aircrafts/aircraft_configuration.h"
#include "src/network/events/radio_message_received.h"

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

        QtPromise::QPromise<QByteArray> GetJwtToken();

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
        void microphoneCalibrationRequired();

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
        bool m_simPaused = false;

        QNetworkAccessManager *nam = nullptr;
        QPointer<QNetworkReply> m_reply;

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
        void OnSimPaused(bool isPaused);

        void OnUserAircraftDataUpdated(UserAircraftData data);
        void OnUserAircraftConfigDataUpdated(UserAircraftConfigData data);
        void OnRadioStackStateChanged(RadioStackState radioStack);
        void OnRequestControllerInfo(QString callsign);

        void SendSlowPositionPacket();
        void SendFastPositionPacket(bool sendSlowFast = false);
        void SendZeroVelocityFastPositionPacket();
        void SendStoppedFastPositionPacket();

        void OnSlowPositionTimerElapsed();
        void OnFastPositionTimerElapsed();

        void LoginToNetwork(QString password);

        double CalculatePressureAltitude() const
        {
            if(m_xplaneAdapter.XplaneVersion() >= 120000) {
                return m_userAircraftData.AltitudePressure;
            }
            const double deltaPressure = 1013.25 - m_userAircraftData.BarometerSeaLevel;
            const double deltaAltitudeV = deltaPressure * 30.0; // 30.0 ft per millibar
            return (m_userAircraftData.AltitudeMslM * 3.28084) + deltaAltitudeV;
        }

        double AdjustIncomingAltitude(double altitude) {
            if(m_xplaneAdapter.XplaneVersion() < 120000) {
                return altitude;
            }

            double verticalDistance = std::abs(m_userAircraftData.AltitudePressure - altitude);
            if (verticalDistance > 6000.0) {
                return altitude;
            }

            double weight = 1.0;
            if (verticalDistance > 3000.0) {
                weight = 1.0 - ((verticalDistance - 3000.0) / 3000.0);
            }

            double offset = m_userAircraftData.AltitudePressure - (m_userAircraftData.AltitudeMslM * 3.28084);
            return altitude - (offset * weight);
        }

        const double POSITIONAL_VELOCITY_ZERO_TOLERANCE = 0.005;
        bool PositionalVelocityIsZero(UserAircraftData data)
        {
            return (abs(data.LongitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.AltitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.LatitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.PitchVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.AltitudeVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE &&
                    abs(data.HeadingVelocity) < POSITIONAL_VELOCITY_ZERO_TOLERANCE);
        }
    };
}

#endif

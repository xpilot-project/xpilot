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

#include "src/fsd/fsd_client.h"
#include "src/network/vatsim_auth.h"
#include "src/network/connectinfo.h"
#include "src/network/events/radio_message_received.h"
#include "src/simulator/xplane_adapter.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/aircraft_visual_state.h"
#include "src/aircrafts/aircraft_configuration.h"
#include "src/aircrafts/velocity_vector.h"
#include "src/config/appconfig.h"
#include "src/common/build_config.h"
#include "src/common/frequency_utils.h"
#include "src/common/notificationtype.h"
#include "src/common/utils.h"
#include "src/qinjection/dependencypointer.h"

namespace xpilot
{
    class NetworkManager : public QObject
    {
        Q_OBJECT

    public:
        NetworkManager(QObject *owner = nullptr);
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
        void disableVoiceTransmit();
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
        QString m_jwtToken;
        bool m_intentionalDisconnect =  false;
        bool m_forcedDisconnect = false;
        QString m_forcedDisconnectReason = "";
        QList<uint> m_transmitFreqs;
        QFile m_networkLog;
        QTextStream m_rawDataStream;
        bool m_simPaused = false;
        double m_altitudeDelta = 0.0;

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
        QtPromise::QPromise<QString> GetBestFsdServer();

        bool IsXplane12() const { return m_xplaneAdapter.XplaneVersion() >= 120000; }
        double CalculatePressureAltitude() const;
        double GetPressureAltitude() const;

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

        double AdjustIncomingAltitude(double altitude);
    };
}

#endif

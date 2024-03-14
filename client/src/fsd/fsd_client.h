/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2024 Justin Shannon
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

#ifndef FsdSession_h
#define FsdSession_h

#include <QtGlobal>
#include <QPointer>
#include <QObject>
#include <QThread>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QMetaEnum>
#include <QHash>
#include <QCryptographicHash>
#include <QStringEncoder>

#include "client_properties.h"

#include "pdu/pdu_base.h"
#include "pdu/pdu_add_atc.h"
#include "pdu/pdu_server_identification.h"
#include "pdu/pdu_client_identification.h"
#include "pdu/pdu_atc_position.h"
#include "pdu/pdu_delete_atc.h"
#include "pdu/pdu_add_pilot.h"
#include "pdu/pdu_delete_pilot.h"
#include "pdu/pdu_broadcast_message.h"
#include "pdu/pdu_wallop.h"
#include "pdu/pdu_radio_message.h"
#include "pdu/pdu_text_message.h"
#include "pdu/pdu_plane_info_request.h"
#include "pdu/pdu_plane_info_response.h"
#include "pdu/pdu_ping.h"
#include "pdu/pdu_pong.h"
#include "pdu/pdu_client_query.h"
#include "pdu/pdu_client_query_response.h"
#include "pdu/pdu_kill_request.h"
#include "pdu/pdu_protocol_error.h"
#include "pdu/pdu_fast_pilot_position.h"
#include "pdu/pdu_pilot_position.h"
#include "pdu/pdu_metar_response.h"
#include "pdu/pdu_metar_request.h"
#include "pdu/pdu_send_fast.h"
#include "pdu/pdu_auth_challenge.h"
#include "pdu/pdu_auth_response.h"
#include "pdu/pdu_change_server.h"
#include "pdu/pdu_mute.h"

namespace xpilot
{
    class FsdClient : public QObject
    {
        Q_OBJECT

    public:
        explicit FsdClient(QObject *parent = nullptr);

        void SetClientProperties(ClientProperties clientProperties);
        void Connect(QString address, quint32 port, bool challengeServer = true);
        void Disconnect();

        template<class T>
        void SendPDU(const T &message)
        {
            if(!m_connected) return;
            sendData(Serialize(message));
        }

        bool IsConnected() const { return m_connected; }

    signals:
        void RaiseNetworkConnected();
        void RaiseNetworkDisconnected();
        void sendDataError();
        void RaiseNetworkError(QString error);
        void RaiseServerIdentificationReceived(PDUServerIdentification pdu);
        void RaiseClientIdentificationReceived(PDUClientIdentification pdu);
        void RaisePilotPositionReceived(PDUPilotPosition pdu);
        void RaiseFastPilotPositionReceived(PDUFastPilotPosition pdu);
        void RaiseATCPositionReceived(PDUATCPosition pdu);
        void RaiseAddATCReceived(PDUAddATC pdu);
        void RaiseDeleteATCReceived(PDUDeleteATC pdu);
        void RaiseAddPilotReceived(PDUAddPilot pdu);
        void RaiseDeletePilotReceived(PDUDeletePilot pdu);
        void RaiseBroadcastMessageReceived(PDUBroadcastMessage pdu);
        void RaiseWallopReceived(PDUWallop pdu);
        void RaiseRadioMessageReceived(PDURadioMessage pdu);
        void RaiseTextMessageReceived(PDUTextMessage pdu);
        void RaiseMetarResponseReceived(PDUMetarResponse pdu);
        void RaisePlaneInfoRequestReceived(PDUPlaneInfoRequest pdu);
        void RaisePlaneInfoResponseReceived(PDUPlaneInfoResponse pdu);
        void RaisePingReceived(PDUPing pdu);
        void RaisePongReceived(PDUPong pdu);
        void RaiseClientQueryReceived(PDUClientQuery pdu);
        void RaiseClientQueryResponseReceived(PDUClientQueryResponse pdu);
        void RaiseKillRequestReceived(PDUKillRequest pdu);
        void RaiseProtocolErrorReceived(PDUProtocolError pdu);
        void RaiseSendFastReceived(PDUSendFast pdu);
        void RaiseRawDataSent(QString data);
        void RaiseRawDataReceived(QString data);
        void RaiseMuteReceived(PDUMute pdu);

    private:
        void connectSocketSignals();
        void handleSocketError(QAbstractSocket::SocketError socketError);
        void handleSocketConnected();
        void handleDataReceived();
        void handleChangeServer(const QStringList& fields);
        void processData(QString data);
        void sendData(QString data);
        void processTM(QStringList& fields);

        void sendSlowPositionUpdate();

        QString socketErrorString(QAbstractSocket::SocketError error) const;
        static QString socketErrorToQString(QAbstractSocket::SocketError error);

        QString performDnsLookup(const QString address);

        QString toMd5(QString value);

    private:
        std::unique_ptr<QTcpSocket> m_socket = std::make_unique<QTcpSocket>(this);
        bool m_connected = false;
        bool m_serverChangeInProgress = false;

        bool m_challengeServer;
        QString m_partialPacket = "";

        QString m_fsdServerAddress = "";

        std::string m_clientAuthSessionKey;
        std::string m_clientAuthChallengeKey;

        static int constexpr m_slowPositionTimerInterval = 5000;
        static int constexpr m_fastPositionTimerInterval = 200;

        ClientProperties m_clientProperties;
    };
}

#endif

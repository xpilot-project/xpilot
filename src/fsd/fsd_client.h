#ifndef FsdSession_h
#define FsdSession_h

#include <QtGlobal>
#include <QPointer>
#include <QObject>
#include <QThread>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QTextCodec>
#include <QMetaEnum>
#include <QHash>
#include <QCryptographicHash>

#include "../vatsim_auth.h"
#include "../private_tokens.h"
#include "client_properties.h"
#include "pdu/pdu_base.h"
#include "pdu/pdu_add_atc.h"
#include "pdu/pdu_server_identification.h"
#include "pdu/pdu_client_identification.h"
#include "pdu/pdu_delete_atc.h"
#include "pdu/pdu_add_pilot.h"
#include "pdu/pdu_delete_pilot.h"
#include "pdu/pdu_broadcast_message.h"
#include "pdu/pdu_wallop.h"
#include "pdu/pdu_radio_message.h"
#include "pdu/pdu_text_message.h"
#include "pdu/pdu_plane_info_request.h"
#include "pdu/pdu_plane_info_response.h"
#include "pdu/pdu_flight_plan.h"
#include "pdu/pdu_ping.h"
#include "pdu/pdu_pong.h"
#include "pdu/pdu_client_query.h"
#include "pdu/pdu_client_query_response.h"
#include "pdu/pdu_kill_request.h"
#include "pdu/pdu_protocol_error.h"
#include "pdu/pdu_auth_challenge.h"
#include "pdu/pdu_auth_response.h"

namespace xpilot
{
    class FsdClient : public QObject
    {
        Q_OBJECT

    public:
        explicit FsdClient(QObject *parent = nullptr);

        void Connect(QString address, quint32 port, bool challengeServer = true);
        void Disconnect();

        void SendPDU(PDUBase&& message);

        bool getConnectionStatus() const { return m_connected; }

    signals:
        void networkConnected();
        void networkDisconnected();
        void sendDataError();

        void RaiseServerIdentificationReceived(PDUServerIdentification pdu);
        void RaiseClientIdentificationReceived(PDUClientIdentification pdu);
        void RaiseAddATCReceived(PDUAddATC pdu);
        void RaiseDeleteATCReceived(PDUDeleteATC pdu);
        void RaiseAddPilotReceived(PDUAddPilot pdu);
        void RaiseDeletePilotReceived(PDUDeletePilot pdu);
        void RaiseBroadcastMessageReceived(PDUBroadcastMessage pdu);
        void RaiseWallopReceived(PDUWallop pdu);
        void RaiseRadioMessageReceived(PDURadioMessage pdu);
        void RaiseTextMessageReceived(PDUTextMessage pdu);
        void RaisePlaneInfoRequestReceived(PDUPlaneInfoRequest pdu);
        void RaisePlaneInfoResponseReceived(PDUPlaneInfoResponse pdu);
        void RaiseFlightPlanReceived(PDUFlightPlan pdu);
        void RaisePingReceived(PDUPing pdu);
        void RaisePongReceived(PDUPong pdu);
        void RaiseClientQueryReceived(PDUClientQuery pdu);
        void RaiseClientQueryResponseReceived(PDUClientQueryResponse pdu);
        void RaiseKillRequestReceived(PDUKillRequest pdu);
        void RaiseProtocolErrorReceived(PDUProtocolError pdu);

    private:
        void handleSocketError(QAbstractSocket::SocketError socketError);
        void handleSocketConnected();
        void handleSocketDisconnected();
        void handleDataReceived();
        void processData(QString data);
        void sendData(QString data);

        void handleAuthChallenge(QString& data);
        void processTM(QStringList& fields);

        void sendSlowPositionUpdate();

        QString socketErrorString(QAbstractSocket::SocketError error) const;
        static QString socketErrorToQString(QAbstractSocket::SocketError error);

        QString toMd5(QString value);

        void CheckServerAuthChallengeResponse(QString response);
        void CheckServerAuth();
        void ChallengeServer();
        void ResetServerAuthSession();

    private:
        QTextCodec* m_fsdTextCodec = nullptr;

        QTcpSocket m_socket { this };
        bool m_connected { false };

        QTimer m_slowPositionUpdateTimer { this };
        QTimer m_fastPositionUpdateTimer { this };

        QString m_currentCallsign;

        bool m_challengeServer;
        QString m_partialPacket = "";

        ushort m_clientId;
        const char* m_privateKey;

        QString m_serverAuthSessionKey;
        QString m_serverAuthChallengeKey;
        QString m_lastServerAuthChallenge;
        QString m_clientAuthSessionKey;
        QString m_clientAuthChallengeKey;

        static int constexpr m_slowPositionTimerInterval = 5000;
        static int constexpr m_fastPositionTimerInterval = 200;

        static int constexpr m_serverAuthChallengeInterval = 60000;
        static int constexpr m_serverAuthChallengeResponseWindow = 30000;
    };
}

#endif

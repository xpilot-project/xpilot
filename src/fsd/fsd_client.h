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

#include "pdu/pdu_base.h"
#include "pdu/pdu_server_identification.h"
#include "pdu/pdu_client_identification.h"

namespace xpilot
{
    class FsdClient : public QObject
    {
        Q_OBJECT

    public:
        explicit FsdClient(QObject *parent = nullptr);

        void Connect(QString address, quint32 port, bool challengeServer = true);
        void Disconnect();

        template <class T>
        void SendPDU(const T& message)
        {
            sendData(message + PDUBase::PacketDelimeter);
        }

        bool getConnectionStatus() const { return m_connected; }

    signals:
        void networkConnected();
        void networkDisconnected();
        void sendDataError();

        void RaiseServerIdentificationReceived(PDUServerIdentification pdu);

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

    private:
        QTextCodec* m_fsdTextCodec = nullptr;

        QTcpSocket m_socket { this };
        bool m_connected { false };

        QTimer m_slowPositionUpdateTimer { this };
        QTimer m_fastPositionUpdateTimer { this };

        bool m_challengeServer;
        QString m_partialPacket = "";

        static int constexpr m_serverAuthChallengeInterval = 60000;
        static int constexpr m_serverAuthChallengeResponseWindow = 30000;

        static int constexpr m_slowPositionTimerInterval = 5000;
        static int constexpr m_fastPositionTimerInterval = 200;
    };
}

#endif

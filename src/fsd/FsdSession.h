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

#include "../core/worker.h"
#include "pdu/PDUBase.h"
#include "pdu/PDUServerIdentification.h"
#include "pdu/PDUClientIdentification.h"

namespace xpilot
{
    class FsdSession : public xpilot::core::ContinuousWorker
    {
        Q_OBJECT

    public:
        explicit FsdSession(QObject *parent = nullptr);

        void init();
        bool getConnectStatus() const { return m_connectFlag; }

        void ConnectToServer(QString address, quint16 port, bool challengeServer = true);
        void DisconnectFromServer();

        template <class T>
        void SendPDU(const T& message)
        {
            if(m_challengeServer)
            {

            }

            sendData(message + PDUBase::PacketDelimeter);
        }

    signals:
        void networkConnected();
        void networkDisconnected();
        void sendDataError();

        void RaiseServerIdentificationReceived(PDUServerIdentification pdu);

    private:
        void handleSocketError(QAbstractSocket::SocketError socketError);
        void readDataFromSocket();
        void processData(QString data);
        void sendData(QString data);

        QString socketErrorString(QAbstractSocket::SocketError error) const;
        static QString socketErrorToQString(QAbstractSocket::SocketError error);

    private:
        QTextCodec* m_fsdTextCodec = nullptr;

        QTcpSocket m_socket { this };
        bool m_connectFlag { false };

        bool m_challengeServer;
        QString m_partialPacket = "";

        static int constexpr m_serverAuthChallengeInterval = 60000;
        static int constexpr m_serverAuthChallengeResponseWindow = 30000;
    };
}

#endif

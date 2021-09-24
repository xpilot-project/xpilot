#include "FsdSession.h"
#include "../core/threadutils.h"

namespace xpilot
{
    FsdSession::FsdSession(QObject * parent) : QObject(parent)
    {
        init();

        connect(m_socket, &QTcpSocket::readyRead, this, &FsdSession::readDataFromSocket);
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &FsdSession::handleSocketError);
        connect(m_socket, &QTcpSocket::connected, this, [=]() {
            m_connectFlag = true;
        });
        connect(m_socket, &QTcpSocket::disconnected, this, [=]() {
            m_connectFlag = false;
        });

        m_fsdTextCodec = QTextCodec::codecForName("ISO-8859-1");
    }

    FsdSession::~FsdSession()
    {
//        if(m_thread)
//        {
//            m_thread->quit();
//            m_thread->wait();
//            delete m_thread;
//            m_thread = nullptr;
//        }

        if(m_socket)
        {
            delete m_socket;
            m_socket = nullptr;
        }
    }

    void FsdSession::init()
    {
//        if(!m_thread)
//        {
//            m_thread = new QThread;
//            this->moveToThread(m_thread);
//            m_thread->start();
//        }

        if(!m_socket)
        {
            m_socket = new QTcpSocket;
        }
    }

    void FsdSession::ConnectToServer(QString address, quint16 port, bool challengeServer)
    {
//        if(!ThreadUtils::isInThisThread(this))
//        {
//            QMetaObject::invokeMethod(this, [=]
//            {
//                ConnectToServer(address, port, challengeServer);
//            });
//            return;
//        }

        if(m_socket->isOpen()) return;
        m_challengeServer = challengeServer;
        m_socket->connectToHost(address, port);
    }

    void FsdSession::DisconnectFromServer()
    {
//        if(!ThreadUtils::isInThisThread(this))
//        {
//            QMetaObject::invokeMethod(this, [=]
//            {
//                DisconnectFromServer();
//            });
//            return;
//        }

        if(m_socket)
        {
            m_socket->close();
        }
    }

    void FsdSession::readDataFromSocket()
    {
        if(m_socket->bytesAvailable() < 1) return;

        const QByteArray dataEncoded = m_socket->readAll();
        if(dataEncoded.isEmpty()) return;

        const QString data = m_fsdTextCodec->toUnicode(dataEncoded);
        processData(data);
    }

    void FsdSession::processData(QString data)
    {
        if(data.length() == 0) return;

        // Strip out trailing null, if any
        if(data.right(data.length() - 1) == "\0")
        {
            data = data.left(data.length() - 1);
        }

        data = m_partialPacket + data;
        m_partialPacket = "";

        qDebug() << "<< " << data;

        QStringList packets = data.split(QLatin1String("\r\n"));

        // If he last packet has content, it's an incomplete packet.
        int topIndex = packets.length() - 1;
        if(packets[topIndex].length() > 0)
        {
            m_partialPacket = packets[topIndex];
            packets[topIndex] = "";
        }

        for(const auto& packet : packets)
        {
            if(packet.length() == 0) continue;

            QStringList fields = packet.split(QLatin1String(":"));
            const QCharRef prefixChar = fields[0][0];

            if(prefixChar == '@')
            {

            }
            else if(prefixChar == '^')
            {

            }
            else if(prefixChar == '%')
            {

            }
            else if(prefixChar == '\\')
            {

            }
            else if(prefixChar == '#' || prefixChar == '$')
            {
                if(fields[0].length() < 3)
                {
                    qDebug() << "Invalid PDU type %1" << packet;
                }

                QString pduTypeId = fields[0].mid(0, 3);
                fields[0] = fields[0].mid(3);

                if(pduTypeId == "$DI")
                {
                    auto pdu = PDUServerIdentification::Parse(fields);
                    auto reply = new PDUClientIdentification(pdu.From, 0xd8f2, "xPilot", 1, 0, "1215759", "", "");
                    SendPDU(reply->Serialize());
                }
                else if(pduTypeId == "$ID")
                {

                }
                else if(pduTypeId == "#AA")
                {

                }
                else if(pduTypeId == "#DA")
                {

                }
                else if(pduTypeId == "#AP")
                {

                }
                else if(pduTypeId == "#DP")
                {

                }
                else if(pduTypeId == "#TM")
                {

                }
                else if(pduTypeId == "#WX")
                {

                }
                else if(pduTypeId == "#WD")
                {

                }
                else if(pduTypeId == "#DL")
                {

                }
                else if(pduTypeId == "#TD")
                {

                }
                else if(pduTypeId == "#CD")
                {

                }
                else if(pduTypeId == "#PC")
                {

                }
                else if(pduTypeId == "#SB")
                {

                }
                else if(pduTypeId == "$FP")
                {

                }
                else if(pduTypeId == "$AM")
                {

                }
                else if(pduTypeId == "$PI")
                {

                }
                else if(pduTypeId == "$PO")
                {

                }
                else if(pduTypeId == "$HO")
                {

                }
                else if(pduTypeId == "$HA")
                {

                }
                else if(pduTypeId == "$AX")
                {

                }
                else if(pduTypeId == "$AR")
                {

                }
                else if(pduTypeId == "$CQ")
                {

                }
                else if(pduTypeId == "$ZC")
                {

                }
                else if(pduTypeId == "$ZR")
                {

                }
                else if(pduTypeId == "$!!")
                {

                }
                else if(pduTypeId == "$ER")
                {

                }
                else
                {
                    qDebug() << "Unknown PDU type: %1" << packet;
                }
            }
            else
            {
                qDebug() << "Unknown PDU prefix: %1" << prefixChar;
            }
        }
    }

    void FsdSession::sendData(QString data)
    {
        if(!m_connectFlag || data.isEmpty()) return;

        const QByteArray bufferEncoded = data.toUtf8();

        qDebug() << ">> " << bufferEncoded;

        m_socket->write(bufferEncoded);
    }

    void FsdSession::handleSocketError(QAbstractSocket::SocketError socketError)
    {
        const QString error = this->socketErrorString(socketError);
        qDebug() << "FSD socket error: " << error;

        switch(socketError)
        {
            case QAbstractSocket::RemoteHostClosedError:
            this->networkDisconnected();
            break;
        }
    }

    QString FsdSession::socketErrorToQString(QAbstractSocket::SocketError error)
    {
        static const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        return metaEnum.valueToKey(error);
    }

    QString FsdSession::socketErrorString(QAbstractSocket::SocketError error) const
    {
        QString e = socketErrorToQString(error);
        if(!m_socket->errorString().isEmpty())
        {
            e += QStringLiteral(": ") % m_socket->errorString();
        }
        return e;
    }
}

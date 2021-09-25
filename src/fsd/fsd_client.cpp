#include "fsd_client.h"
#include "../core/threadutils.h"

namespace xpilot
{
    FsdClient::FsdClient(QObject * parent) : QObject(parent)
    {
        m_fsdTextCodec = QTextCodec::codecForName("ISO-8859-1");

        m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);

        connect(&m_socket, &QTcpSocket::readyRead, this, &FsdClient::handleDataReceived);
        connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &FsdClient::handleSocketError);
        connect(&m_socket, &QTcpSocket::connected, this, &FsdClient::handleSocketConnected);
        connect(&m_socket, &QTcpSocket::disconnected, this, &FsdClient::handleSocketDisconnected);

        m_slowPositionUpdateTimer.setObjectName(this->objectName().append(":m_slowPositionUpdateTimer"));
        connect(&m_slowPositionUpdateTimer, &QTimer::timeout, this, &FsdClient::sendSlowPositionUpdate);
    }

    void FsdClient::Connect(QString address, quint32 port, bool challengeServer)
    {
        if(!ThreadUtils::isInThisThread(this))
        {
            QMetaObject::invokeMethod(this, [=]
            {
                Connect(address, port, challengeServer);
            });
            return;
        }

        if(m_socket.isOpen()) return;
        m_challengeServer = challengeServer;
        m_socket.connectToHost(address, port);
        m_slowPositionUpdateTimer.start(m_slowPositionTimerInterval);
    }

    void FsdClient::Disconnect()
    {
        if(!ThreadUtils::isInThisThread(this))
        {
            QMetaObject::invokeMethod(this, [=]
            {
                Disconnect();
            });
            return;
        }

        m_slowPositionUpdateTimer.stop();
        m_socket.close();
    }

    void FsdClient::handleDataReceived()
    {
        if(m_socket.bytesAvailable() < 1) return;

        const QByteArray dataEncoded = m_socket.readAll();
        if(dataEncoded.isEmpty()) return;

        const QString data = m_fsdTextCodec->toUnicode(dataEncoded);
        processData(data);
    }

    void FsdClient::processData(QString data)
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

        QStringList packets = data.split(PDUBase::PacketDelimeter);

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

            QStringList fields = packet.split(PDUBase::Delimeter);
            const QCharRef prefixChar = fields[0][0];

            if(prefixChar == '@')
            {
                fields[0] = fields[0].mid(1);
            }
            else if(prefixChar == '^')
            {
                fields[0] = fields[0].mid(1);
            }
            else if(prefixChar == '%')
            {
                fields[0] = fields[0].mid(1);
            }
            else if(prefixChar == '#' || prefixChar == '$')
            {
                if(fields[0].length() < 3)
                {
                    qDebug() << "Invalid PDU type: " << packet;
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
                    processTM(fields);
                }
                else if(pduTypeId == "#SB")
                {

                }
                else if(pduTypeId == "$FP")
                {

                }
                else if(pduTypeId == "$PI")
                {

                }
                else if(pduTypeId == "$PO")
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
            }
            else
            {
                qDebug() << "Unknown PDU prefix: %1" << prefixChar;
            }
        }
    }

    void FsdClient::sendData(QString data)
    {
        if(!m_connected || data.isEmpty()) return;

        const QByteArray bufferEncoded = data.toUtf8();

        qDebug() << ">> " << bufferEncoded;

        m_socket.write(bufferEncoded);
    }

    void FsdClient::handleAuthChallenge(QString &data)
    {

    }

    void FsdClient::processTM(QStringList &fields)
    {
        if(fields.length() < 3)
        {
            qDebug() << "Invalid field count." << PDUBase::Reassemble(fields);
        }

        // #TMs are allowed to have colons in the message field, so here we need
        // to rejoin the fields then resplit with a limit of 3 substrings.
        QString raw = PDUBase::Reassemble(fields);
        fields = raw.split(PDUBase::Delimeter).mid(0, 3);

        if(fields[1] == "*")
        {

        }
        else if(fields[1] == "*s")
        {

        }
        else if(fields[1] == "@49999")
        {

        }
        else
        {
            if(fields[1].mid(0, 1) == "@")
            {

            }
            else
            {

            }
        }
    }

    void FsdClient::sendSlowPositionUpdate()
    {

    }

    void FsdClient::handleSocketError(QAbstractSocket::SocketError socketError)
    {
        const QString error = this->socketErrorString(socketError);
        qDebug() << "FSD socket error: " << error;

        switch(socketError)
        {
        case QAbstractSocket::RemoteHostClosedError:
            this->Disconnect();
            break;
        }
    }

    void FsdClient::handleSocketConnected()
    {
        m_connected = true;
    }

    void FsdClient::handleSocketDisconnected()
    {
        m_connected = false;
    }

    QString FsdClient::socketErrorToQString(QAbstractSocket::SocketError error)
    {
        static const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        return metaEnum.valueToKey(error);
    }

    QString FsdClient::socketErrorString(QAbstractSocket::SocketError error) const
    {
        QString e = socketErrorToQString(error);
        if(!m_socket.errorString().isEmpty())
        {
            e += QStringLiteral(": ") % m_socket.errorString();
        }
        return e;
    }
}

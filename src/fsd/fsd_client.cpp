#include "fsd_client.h"

namespace xpilot
{
    FsdClient::FsdClient(QObject * parent) : QObject(parent)
    {
        m_fsdTextCodec = QTextCodec::codecForName("ISO-8859-1");

        connect(&m_socket, &QTcpSocket::readyRead, this, &FsdClient::handleDataReceived);
        connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &FsdClient::handleSocketError);
        connect(&m_socket, &QTcpSocket::connected, this, &FsdClient::handleSocketConnected);
    }

    void FsdClient::Connect(QString address, quint32 port, bool challengeServer)
    {
        if(m_socket.isOpen()) return;
        m_challengeServer = challengeServer;
        m_socket.connectToHost(address, port);
    }

    void FsdClient::Disconnect()
    {
        emit RaiseNetworkDisconnected();
        m_socket.close();
    }

    void FsdClient::SendPDU(PDUBase&& message)
    {
        sendData(message.Serialize() + PDUBase::PacketDelimeter);
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

            qDebug() << "<< " << packet;

            QStringList fields = packet.split(PDUBase::Delimeter);
            const QCharRef prefixChar = fields[0][0];

            if(prefixChar == '@')
            {
                fields[0] = fields[0].mid(1);
                emit RaisePilotPositionReceived(PDUPilotPosition::Parse(fields));
            }
            else if(prefixChar == '^')
            {
                fields[0] = fields[0].mid(1);
                emit RaiseFastPilotPositionReceived(PDUFastPilotPosition::Parse(fields));
            }
            else if(prefixChar == '%')
            {
                fields[0] = fields[0].mid(1);
                emit RaiseATCPositionReceived(PDUATCPosition::Parse(fields));
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
                    m_clientAuthSessionKey = GenerateAuthResponse(pdu.InitialChallengeKey.toStdString().c_str(), NULL);
                    m_clientAuthChallengeKey = m_clientAuthSessionKey;
                    emit RaiseServerIdentificationReceived(pdu);
                }
                else if(pduTypeId == "$ID")
                {
                    emit RaiseClientIdentificationReceived(PDUClientIdentification::Parse(fields));
                }
                else if(pduTypeId == "#AA")
                {
                    emit RaiseAddATCReceived(PDUAddATC::Parse(fields));
                }
                else if(pduTypeId == "#DA")
                {
                    emit RaiseDeleteATCReceived(PDUDeleteATC::Parse(fields));
                }
                else if(pduTypeId == "#AP")
                {
                    emit RaiseAddPilotReceived(PDUAddPilot::Parse(fields));
                }
                else if(pduTypeId == "#DP")
                {
                    emit RaiseDeletePilotReceived(PDUDeletePilot::Parse(fields));
                }
                else if(pduTypeId == "#TM")
                {
                    processTM(fields);
                }
                else if(pduTypeId == "#SB")
                {
                    if(fields.length() >= 3) {
                        if(fields[2] == "PIR") {
                            emit RaisePlaneInfoRequestReceived(PDUPlaneInfoRequest::Parse(fields));
                        }
                        else if(fields[2] == "PI" && fields.length() >= 4) {
                            if(fields[3] == "GEN") {
                                emit RaisePlaneInfoResponseReceived(PDUPlaneInfoResponse::Parse(fields));
                            }
                        }
                    }
                }
                else if(pduTypeId == "$FP")
                {
                    emit RaiseFlightPlanReceived(PDUFlightPlan::Parse(fields));
                }
                else if(pduTypeId == "$PI")
                {
                    emit RaisePingReceived(PDUPing::Parse(fields));
                }
                else if(pduTypeId == "$PO")
                {
                    emit RaisePongReceived(PDUPong::Parse(fields));
                }
                else if(pduTypeId == "$CQ")
                {
                    emit RaiseClientQueryReceived(PDUClientQuery::Parse(fields));
                }
                else if(pduTypeId == "$CR")
                {
                    emit RaiseClientQueryResponseReceived(PDUClientQueryResponse::Parse(fields));
                }
                else if(pduTypeId == "$ZC")
                {
                    auto pdu = PDUAuthChallenge::Parse(fields);
                    QString response = GenerateAuthResponse(pdu.Challenge.toStdString().c_str(), m_clientAuthChallengeKey.toStdString().c_str());
                    std::string combined = m_clientAuthSessionKey.toStdString() + response.toStdString();
                    m_clientAuthChallengeKey = toMd5(combined.c_str());
                    SendPDU(PDUAuthResponse(pdu.To, pdu.From, response));
                }
                else if(pduTypeId == "$!!")
                {
                    emit RaiseKillRequestReceived(PDUKillRequest::Parse(fields));
                }
                else if(pduTypeId == "$ER")
                {
                    emit RaiseProtocolErrorReceived(PDUProtocolError::Parse(fields));
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

        const QByteArray bufferEncoded = m_fsdTextCodec->fromUnicode(data);

        qDebug() << ">> " << bufferEncoded;

        m_socket.write(bufferEncoded);
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
            emit RaiseBroadcastMessageReceived(PDUBroadcastMessage::Parse(fields));
        }
        else if(fields[1] == "*s")
        {
            emit RaiseWallopReceived(PDUWallop::Parse(fields));
        }
        else
        {
            if(fields[1].mid(0, 1) == "@")
            {
                emit RaiseRadioMessageReceived(PDURadioMessage::Parse(fields));
            }
            else
            {
                emit RaiseTextMessageReceived(PDUTextMessage::Parse(fields));
            }
        }
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
        emit RaiseNetworkConnected();
    }

    QString FsdClient::socketErrorToQString(QAbstractSocket::SocketError error)
    {
        static const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        return metaEnum.valueToKey(error);
    }

    QString FsdClient::toMd5(QString value)
    {
        return QString(QCryptographicHash::hash(value.toStdString().c_str(), QCryptographicHash::Md5).toHex());
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

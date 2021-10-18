#include "fsd_client.h"
#include "src/network/vatsim_config.h"

namespace xpilot
{
    FsdClient::FsdClient(QObject * parent) : QObject(parent)
    {
        m_socket = new QTcpSocket(this);

        m_fsdTextCodec = QTextCodec::codecForName("ISO-8859-1");

        connect(m_socket, &QTcpSocket::readyRead, this, &FsdClient::handleDataReceived);
        connect(m_socket, &QTcpSocket::connected, this, &FsdClient::handleSocketConnected);
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &FsdClient::handleSocketError);
    }

    void FsdClient::Connect(QString address, quint32 port, bool challengeServer)
    {
        if(VatsimClientId() == 0 || VatsimClientKey().isEmpty()) {
            emit RaiseNetworkError("Invalid pilot client build. Please download a new copy from the xPilot website.");
            return;
        }
        if(m_socket->isOpen()) return;
        m_challengeServer = challengeServer;
        m_socket->connectToHost(address, port);
    }

    void FsdClient::Disconnect()
    {
        m_connected = false;
        emit RaiseNetworkDisconnected();
        m_socket->close();
    }

    void FsdClient::handleDataReceived()
    {
        if(m_socket->bytesAvailable() < 1) return;

        const QByteArray dataEncoded = m_socket->readAll();
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

            emit RaiseRawDataReceived(packet + PDUBase::PacketDelimeter);

            QStringList fields = packet.split(PDUBase::Delimeter);
            const QCharRef prefixChar = fields[0][0];

            if(prefixChar == '@')
            {
                fields[0] = fields[0].mid(1);
                emit RaisePilotPositionReceived(PDUPilotPosition::fromTokens(fields));
            }
            else if(prefixChar == '^')
            {
                fields[0] = fields[0].mid(1);
                emit RaiseFastPilotPositionReceived(PDUFastPilotPosition::fromTokens(fields));
            }
            else if(prefixChar == '%')
            {
                fields[0] = fields[0].mid(1);
                emit RaiseATCPositionReceived(PDUATCPosition::fromTokens(fields));
            }
            else if(prefixChar == '#' || prefixChar == '$')
            {
                if(fields[0].length() < 3) break;

                QString pduTypeId = fields[0].mid(0, 3);
                fields[0] = fields[0].mid(3);

                if(pduTypeId == "$DI")
                {
                    auto pdu = PDUServerIdentification::fromTokens(fields);
                    m_clientAuthSessionKey = GenerateAuthResponse(pdu.InitialChallengeKey.toStdString().c_str(), VatsimClientId(), VatsimClientKey().toStdString().c_str());
                    m_clientAuthChallengeKey = m_clientAuthSessionKey;
                    emit RaiseServerIdentificationReceived(pdu);
                }
                else if(pduTypeId == "$ID")
                {
                    emit RaiseClientIdentificationReceived(PDUClientIdentification::fromTokens(fields));
                }
                else if(pduTypeId == "#AA")
                {
                    emit RaiseAddATCReceived(PDUAddATC::fromTokens(fields));
                }
                else if(pduTypeId == "#DA")
                {
                    emit RaiseDeleteATCReceived(PDUDeleteATC::fromTokens(fields));
                }
                else if(pduTypeId == "#AP")
                {
                    emit RaiseAddPilotReceived(PDUAddPilot::fromTokens(fields));
                }
                else if(pduTypeId == "#DP")
                {
                    emit RaiseDeletePilotReceived(PDUDeletePilot::fromTokens(fields));
                }
                else if(pduTypeId == "#TM")
                {
                    processTM(fields);
                }
                else if(pduTypeId == "#SB")
                {
                    if(fields.length() >= 3)
                    {
                        if(fields[2] == "PIR")
                        {
                            emit RaisePlaneInfoRequestReceived(PDUPlaneInfoRequest::fromTokens(fields));
                        }
                        else if(fields[2] == "PI" && fields.length() >= 4)
                        {
                            if(fields[3] == "GEN")
                            {
                                emit RaisePlaneInfoResponseReceived(PDUPlaneInfoResponse::fromTokens(fields));
                            }
                        }
                    }
                }
                else if(pduTypeId == "$PI")
                {
                    emit RaisePingReceived(PDUPing::fromTokens(fields));
                }
                else if(pduTypeId == "$PO")
                {
                    emit RaisePongReceived(PDUPong::fromTokens(fields));
                }
                else if(pduTypeId == "$CQ")
                {
                    emit RaiseClientQueryReceived(PDUClientQuery::fromTokens(fields));
                }
                else if(pduTypeId == "$CR")
                {
                    emit RaiseClientQueryResponseReceived(PDUClientQueryResponse::fromTokens(fields));
                }
                else if(pduTypeId == "$ZC")
                {
                    auto pdu = PDUAuthChallenge::fromTokens(fields);
                    QString response = GenerateAuthResponse(pdu.ChallengeKey.toStdString().c_str(), VatsimClientId(), m_clientAuthChallengeKey.toStdString().c_str());
                    std::string combined = m_clientAuthSessionKey.toStdString() + response.toStdString();
                    m_clientAuthChallengeKey = toMd5(combined.c_str());
                    SendPDU(PDUAuthResponse(pdu.To, pdu.From, response));
                }
                else if(pduTypeId == "$!!")
                {
                    emit RaiseKillRequestReceived(PDUKillRequest::fromTokens(fields));
                }
                else if(pduTypeId == "$ER")
                {
                    emit RaiseProtocolErrorReceived(PDUProtocolError::fromTokens(fields));
                }
            }
        }
    }

    void FsdClient::sendData(QString data)
    {
        if(!m_connected || data.isEmpty()) return;

        const QByteArray bufferEncoded = m_fsdTextCodec->fromUnicode(data);

        emit RaiseRawDataSent(bufferEncoded);

        m_socket->write(bufferEncoded);
    }

    void FsdClient::processTM(QStringList &fields)
    {
        if(fields.length() < 3) return;

        if(fields[1] == "*")
        {
            emit RaiseBroadcastMessageReceived(PDUBroadcastMessage::fromTokens(fields));
        }
        else if(fields[1] == "*s")
        {
            emit RaiseWallopReceived(PDUWallop::fromTokens(fields));
        }
        else
        {
            if(fields[1].mid(0, 1) == "@")
            {
                emit RaiseRadioMessageReceived(PDURadioMessage::fromTokens(fields));
            }
            else
            {
                emit RaiseTextMessageReceived(PDUTextMessage::fromTokens(fields));
            }
        }
    }

    void FsdClient::handleSocketError(QAbstractSocket::SocketError socketError)
    {
        const QString error = this->socketErrorString(socketError);

        switch(socketError)
        {
        case QAbstractSocket::RemoteHostClosedError:
            this->Disconnect();
            break;
        default:
            emit RaiseNetworkError(error);
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
        if(!m_socket->errorString().isEmpty())
        {
            e += QStringLiteral(": ") % m_socket->errorString();
        }
        return e;
    }
}

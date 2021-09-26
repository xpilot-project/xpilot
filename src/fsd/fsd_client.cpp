#include "fsd_client.h"

namespace xpilot
{
    FsdClient::FsdClient(QObject * parent) : QObject(parent)
    {
        m_fsdTextCodec = QTextCodec::codecForName("ISO-8859-1");

        //        m_clientId = Xpilot::PrivateTokens::VatsimClientId();
        //        m_privateKey = Xpilot::PrivateTokens::VatsimPrivateKey().toStdString().c_str();

        m_clientId = 0xd8f2;
        m_privateKey = "ImuL1WbbhVuD8d3MuKpWn2rrLZRa9iVP";

        m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);

        connect(&m_socket, &QTcpSocket::readyRead, this, &FsdClient::handleDataReceived);
        connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &FsdClient::handleSocketError);
        connect(&m_socket, &QTcpSocket::connected, this, &FsdClient::handleSocketConnected);
        connect(&m_socket, &QTcpSocket::disconnected, this, &FsdClient::handleSocketDisconnected);

        connect(&m_slowPositionUpdateTimer, &QTimer::timeout, this, &FsdClient::sendSlowPositionUpdate);
    }

    void FsdClient::Connect(QString address, quint32 port, bool challengeServer)
    {
        if(m_socket.isOpen()) return;
        m_challengeServer = challengeServer;
        m_socket.connectToHost(address, port);
        m_slowPositionUpdateTimer.start(m_slowPositionTimerInterval);
    }

    void FsdClient::Disconnect()
    {
        ResetServerAuthSession();
        m_slowPositionUpdateTimer.stop();
        m_socket.close();
    }

    void FsdClient::SendPDU(PDUBase&& message)
    {
        if(m_challengeServer)
        {
            if(typeid (message) == typeid(PDUClientIdentification))
            {
                PDUClientIdentification& pdu = dynamic_cast<PDUClientIdentification&>(message);
                if(pdu.InitialChallenge.isEmpty()) {
                    QString initialChallenge = GenerateAuthChallenge();
                    m_serverAuthSessionKey = GenerateAuthResponse(initialChallenge.toStdString().c_str(), m_privateKey, m_clientId);
                    pdu.InitialChallenge = initialChallenge;
                }
                message = dynamic_cast<PDUBase&>(pdu);
            }

            if(typeid (message) == typeid (PDUAddATC))
            {
                PDUAddATC& pdu = dynamic_cast<PDUAddATC&>(message);
                if(pdu.Protocol >= ProtocolRevision::VatsimAuth)
                {
                    m_currentCallsign = pdu.From;
                    QTimer::singleShot(m_serverAuthChallengeResponseWindow, this, [=]{
                        this->CheckServerAuth();
                    });
                }
            }
            else if(typeid (message) == typeid (PDUAddPilot))
            {
                PDUAddPilot& pdu = dynamic_cast<PDUAddPilot&>(message);
                if(pdu.Protocol >= ProtocolRevision::VatsimAuth)
                {
                    m_currentCallsign = pdu.From;
                    QTimer::singleShot(m_serverAuthChallengeResponseWindow, this, [=]{
                        this->CheckServerAuth();
                    });
                }
            }
        }
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
                    if(m_clientId != 0) {
                        m_clientAuthSessionKey = GenerateAuthResponse(pdu.InitialChallengeKey.toStdString().c_str(), m_privateKey, m_clientId);
                        m_clientAuthChallengeKey = m_clientAuthSessionKey;
                    }
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
                    if(m_clientId != 0)
                    {
                        auto pdu = PDUAuthChallenge::Parse(fields);
                        QString response = GenerateAuthResponse(pdu.Challenge.toStdString().c_str(), m_clientAuthChallengeKey.toStdString().c_str(), m_clientId);
                        std::string combined = m_clientAuthSessionKey.toStdString() + response.toStdString();
                        m_clientAuthChallengeKey = toMd5(combined.c_str());
                        SendPDU(PDUAuthResponse(pdu.To, pdu.From, response));
                    }
                }
                else if(pduTypeId == "$ZR")
                {
                    auto pdu = PDUAuthResponse::Parse(fields);
                    if(m_challengeServer && m_clientId != 0 && !m_serverAuthChallengeKey.isEmpty() && !m_lastServerAuthChallenge.isEmpty())
                    {
                        CheckServerAuthChallengeResponse(pdu.Response);
                    }
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

    QString FsdClient::toMd5(QString value)
    {
        return QString(QCryptographicHash::hash(value.toStdString().c_str(), QCryptographicHash::Md5).toHex());
    }

    void FsdClient::CheckServerAuthChallengeResponse(QString response)
    {
        QString expectedResponse = GenerateAuthResponse(m_lastServerAuthChallenge.toStdString().c_str(), m_serverAuthChallengeKey.toStdString().c_str(), m_clientId);
        if(response != expectedResponse)
        {
            qDebug() << "CheckServerAuthChallengeResponse() The server has failed to respond correctly to the authentication challenge.";
            Disconnect();
        }
        else
        {
            m_lastServerAuthChallenge = "";
            std::string combined = m_serverAuthSessionKey.toStdString() + response.toStdString();
            m_serverAuthChallengeKey = toMd5(combined.c_str());
            QTimer::singleShot(m_serverAuthChallengeInterval, this, [=]{
                CheckServerAuth();
            });
        }
    }

    void FsdClient::CheckServerAuth()
    {
        // Check if this is the first auth check. If so, we generate the session key and send a challenge.
        if(m_serverAuthChallengeKey.isEmpty())
        {
            m_serverAuthChallengeKey = m_serverAuthSessionKey;
            ChallengeServer();
            return;
        }

        // Check if we have a pending auth challenge. If we do, then the server has failed to respond to
        // the challenge in time, so we disconnect.
        if(!m_lastServerAuthChallenge.isEmpty())
        {
            qDebug() << "CheckServerAuth() The server has failed to respond to the authentication challenge.";
            Disconnect();
            return;
        }

        // If none of the above, challenge the server.
        ChallengeServer();
    }

    void FsdClient::ChallengeServer()
    {
        m_lastServerAuthChallenge = GenerateAuthChallenge();
        SendPDU(PDUAuthChallenge(m_currentCallsign, PDUBase::ServerCallsign, m_lastServerAuthChallenge));
        QTimer::singleShot(m_serverAuthChallengeResponseWindow, this, [=] {
            CheckServerAuth();
        });
    }

    void FsdClient::ResetServerAuthSession()
    {
        m_serverAuthSessionKey = "";
        m_serverAuthChallengeKey = "";
        m_lastServerAuthChallenge = "";
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

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

#include <QDnsLookup>
#include <QEventLoop>

#include "fsd_client.h"
#include "common/build_config.h"
#include "network/vatsim_auth.h"

namespace xpilot
{
    FsdClient::FsdClient(QObject * parent) : QObject(parent)
    {
        connectSocketSignals();
    }

    void FsdClient::SetClientProperties(ClientProperties clientProperties)
    {
        m_clientProperties = clientProperties;
    }

    void FsdClient::connectSocketSignals()
    {
        connect(m_socket.get(), &QTcpSocket::readyRead, this, &FsdClient::handleDataReceived);
        connect(m_socket.get(), &QTcpSocket::connected, this, &FsdClient::handleSocketConnected);
        connect(m_socket.get(), &QTcpSocket::errorOccurred, this, &FsdClient::handleSocketError);
    }

    void FsdClient::Connect(QString address, quint32 port, bool challengeServer)
    {
        // perform dns lookup
        m_fsdServerAddress = performDnsLookup(address);

        if(m_fsdServerAddress.isEmpty()) {
            emit RaiseNetworkError("Network server address returned null, possibly due to a failed DNS lookup.");
            return;
        }

        if(BuildConfig::TowerviewClientId() == 0 || BuildConfig::VatsimClientId() == 0 || BuildConfig::VatsimClientKey().isEmpty()) {
            emit RaiseNetworkError("Invalid pilot client build. Please download a new copy from the xPilot website.");
            return;
        }

        m_challengeServer = challengeServer;
        m_socket->connectToHost(m_fsdServerAddress, port);
        m_partialPacket = "";
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

        auto decoder = QStringDecoder(QStringDecoder::Latin1);
        const QString data = decoder(dataEncoded);

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

        // If the last packet has content, it's an incomplete packet.
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

            try {
                QStringList fields = packet.split(PDUBase::Delimeter);
                const QChar prefixChar = fields[0][0];

                if(prefixChar == '@')
                {
                    fields[0] = fields[0].mid(1);
                    emit RaisePilotPositionReceived(PDUPilotPosition::fromTokens(fields));
                }
                else if(prefixChar == '^')
                {
                    fields[0] = fields[0].mid(1);
                    emit RaiseFastPilotPositionReceived(PDUFastPilotPosition::fromTokens(FastPilotPositionType::Fast, fields));
                }
                else if(prefixChar == '%')
                {
                    fields[0] = fields[0].mid(1);
                    emit RaiseATCPositionReceived(PDUATCPosition::fromTokens(fields));
                }
                else if(prefixChar == '#' || prefixChar == '$')
                {
                    if(fields[0].length() < 3) {
                        throw new PDUFormatException("Invalid PDU type.", packet);
                    }

                    QString pduTypeId = fields[0].mid(0, 3);
                    fields[0] = fields[0].mid(3);

                    if(pduTypeId == "$DI")
                    {
                        auto pdu = PDUServerIdentification::fromTokens(fields);
                        m_clientAuthSessionKey = GenerateAuthResponse(pdu.InitialChallengeKey.toStdString(),
                                                                      m_clientProperties.ClientID,
                                                                      m_clientProperties.PrivateKey.toStdString());
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
                    else if(pduTypeId == "$AR")
                    {
                        emit RaiseMetarResponseReceived(PDUMetarResponse::fromTokens(fields));
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
                        std::string authResponse = GenerateAuthResponse(pdu.ChallengeKey.toStdString(),
                                                                        m_clientProperties.ClientID,
                                                                        m_clientAuthChallengeKey);
                        std::string combined = m_clientAuthSessionKey + authResponse;
                        m_clientAuthChallengeKey = toMd5(combined.c_str()).toStdString();
                        SendPDU(PDUAuthResponse(pdu.To, pdu.From, QString::fromStdString(authResponse)));
                    }
                    else if(pduTypeId == "$!!")
                    {
                        emit RaiseKillRequestReceived(PDUKillRequest::fromTokens(fields));
                    }
                    else if(pduTypeId == "$ER")
                    {
                        emit RaiseProtocolErrorReceived(PDUProtocolError::fromTokens(fields));
                    }
                    else if(pduTypeId == "$SF")
                    {
                        emit RaiseSendFastReceived(PDUSendFast::fromTokens(fields));
                    }
                    else if(pduTypeId == "#SL")
                    {
                        emit RaiseFastPilotPositionReceived(PDUFastPilotPosition::fromTokens(FastPilotPositionType::Slow, fields));
                    }
                    else if(pduTypeId == "#ST")
                    {
                        emit RaiseFastPilotPositionReceived(PDUFastPilotPosition::fromTokens(FastPilotPositionType::Stopped, fields));
                    }
                    else if(pduTypeId == "$XX")
                    {
                        handleChangeServer(fields);
                    }
                }
            }
            catch(PDUFormatException &e) {
                emit RaiseNetworkError(QString("%1 (Raw packet: %2)").arg(e.getError(), e.getRawMessage()));
            }
        }
    }

    void FsdClient::sendData(QString data)
    {
        if(!m_connected || data.isEmpty()) return;

        auto encoder = QStringEncoder(QStringEncoder::Latin1);
        const QByteArray bufferEncoded = encoder(data);

        emit RaiseRawDataSent(bufferEncoded);

        m_socket->write(bufferEncoded);
        m_socket->flush();
    }

    void FsdClient::processTM(QStringList &fields)
    {
        if(fields.length() < 3) {
            throw PDUFormatException("Invalid field count.", PDUBase::Reassemble(fields));
        }

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
        if(m_serverChangeInProgress)
            return;

        const QString error = this->socketErrorString(socketError);
        emit RaiseNetworkError(error);

        if(socketError == QAbstractSocket::RemoteHostClosedError) {
            this->Disconnect();
        }
    }

    void FsdClient::handleSocketConnected()
    {
        m_connected = true;
        emit RaiseNetworkConnected();
    }

    void FsdClient::handleChangeServer(const QStringList &fields)
    {
        m_serverChangeInProgress = true;

        const PDUChangeServer pdu = PDUChangeServer::fromTokens(fields);
        auto newSocket = new QTcpSocket(this);

        connect(newSocket, &QTcpSocket::connected, this, [this, newSocket]{
            handleDataReceived();
            QObject::disconnect(newSocket);
            m_socket.reset(newSocket);
            m_serverChangeInProgress = false;
            connectSocketSignals();
            handleDataReceived();
        });
        connect(newSocket, &QTcpSocket::errorOccurred, this, [this, newSocket]{
            m_serverChangeInProgress = false;
            delete newSocket;
            if(m_socket->state() != QAbstractSocket::ConnectedState) {
                Disconnect();
            }
        });

        QString newServerAddress = performDnsLookup(pdu.NewServer);
        newSocket->connectToHost(newServerAddress, m_socket->peerPort());
        m_partialPacket =  "";
    }

    QString FsdClient::socketErrorToQString(QAbstractSocket::SocketError error)
    {
        static const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        return metaEnum.valueToKey(error);
    }

    QString FsdClient::performDnsLookup(const QString address)
    {
        QHostAddress hostAddress(address);
        if(QAbstractSocket::IPv4Protocol == hostAddress.protocol()) {
            return address; // address is already an ipv4
        }

        QEventLoop eventLoop;
        QDnsLookup *dnsLookup = new QDnsLookup(this);
        dnsLookup->setType(QDnsLookup::A);
        dnsLookup->setName(address);

        QString dnsAddress;

        QObject::connect(dnsLookup, &QDnsLookup::finished, this, [&]() {
            if(dnsLookup->error() == QDnsLookup::NoError && !dnsLookup->hostAddressRecords().isEmpty()) {
                dnsAddress = dnsLookup->hostAddressRecords().constFirst().value().toString();
            }
            dnsLookup->deleteLater();
            eventLoop.quit();
        });

        dnsLookup->lookup();
        eventLoop.exec(); // wait for dns lookup to finish

        return dnsAddress;
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

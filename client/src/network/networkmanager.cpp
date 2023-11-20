/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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

#include <algorithm>
#include <chrono>
#include <random>

#include <QRandomGenerator>
#include <QDateTime>

#include "networkmanager.h"

namespace xpilot
{
    NetworkManager::NetworkManager(QObject *owner) :
        QObject(owner),
        m_xplaneAdapter(*QInjection::Pointer<XplaneAdapter>().data()),
        nam(new QNetworkAccessManager)
    {
        QDir networkLogPath(pathAppend(AppConfig::getInstance()->dataRoot(), "NetworkLogs"));
        if(!networkLogPath.exists()) {
            networkLogPath.mkpath(".");
        }

        // keep only the last 10 log files
        QFileInfoList files = networkLogPath.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
        const int MAX_LOGS_TO_RETAIN = 10;
        for(int index = files.size(); index >= MAX_LOGS_TO_RETAIN; --index) {
            const QFileInfo &info = files.at(index - 1);
            QFile::remove(info.absoluteFilePath());
        }

        m_networkLog.setFileName(pathAppend(networkLogPath.path(), QString("NetworkLog-%1.txt").arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss"))));
        if(m_networkLog.open(QFile::WriteOnly))
        {
            m_rawDataStream.setDevice(&m_networkLog);
        }

        connect(&m_fsd, &FsdClient::RaiseNetworkError, this, &NetworkManager::OnNetworkError);
        connect(&m_fsd, &FsdClient::RaiseProtocolErrorReceived, this, &NetworkManager::OnProtocolErrorReceived);
        connect(&m_fsd, &FsdClient::RaiseNetworkConnected, this, &NetworkManager::OnNetworkConnected);
        connect(&m_fsd, &FsdClient::RaiseNetworkDisconnected, this, &NetworkManager::OnNetworkDisconnected);
        connect(&m_fsd, &FsdClient::RaiseServerIdentificationReceived, this, &NetworkManager::OnServerIdentificationReceived);
        connect(&m_fsd, &FsdClient::RaiseClientQueryReceived, this, &NetworkManager::OnClientQueryReceived);
        connect(&m_fsd, &FsdClient::RaiseClientQueryResponseReceived, this, &NetworkManager::OnClientQueryResponseReceived);
        connect(&m_fsd, &FsdClient::RaisePilotPositionReceived, this, &NetworkManager::OnPilotPositionReceived);
        connect(&m_fsd, &FsdClient::RaiseFastPilotPositionReceived, this, &NetworkManager::OnFastPilotPositionReceived);
        connect(&m_fsd, &FsdClient::RaiseATCPositionReceived, this, &NetworkManager::OnATCPositionReceived);
        connect(&m_fsd, &FsdClient::RaiseDeletePilotReceived, this, &NetworkManager::OnDeletePilotReceived);
        connect(&m_fsd, &FsdClient::RaiseDeleteATCReceived, this, &NetworkManager::OnDeleteATCReceived);
        connect(&m_fsd, &FsdClient::RaisePingReceived, this, &NetworkManager::OnPingReceived);
        connect(&m_fsd, &FsdClient::RaiseTextMessageReceived, this, &NetworkManager::OnTextMessageReceived);
        connect(&m_fsd, &FsdClient::RaiseRadioMessageReceived, this, &NetworkManager::OnRadioMessageReceived);
        connect(&m_fsd, &FsdClient::RaiseBroadcastMessageReceived, this, &NetworkManager::OnBroadcastMessageReceived);
        connect(&m_fsd, &FsdClient::RaiseMetarResponseReceived, this, &NetworkManager::OnMetarResponseReceived);
        connect(&m_fsd, &FsdClient::RaisePlaneInfoRequestReceived, this, &NetworkManager::OnPlaneInfoRequestReceived);
        connect(&m_fsd, &FsdClient::RaisePlaneInfoResponseReceived, this, &NetworkManager::OnPlaneInfoResponseReceived);
        connect(&m_fsd, &FsdClient::RaiseKillRequestReceived, this, &NetworkManager::OnKillRequestReceived);
        connect(&m_fsd, &FsdClient::RaiseSendFastReceived, this, &NetworkManager::OnSendFastReceived);
        connect(&m_fsd, &FsdClient::RaiseRawDataSent, this, &NetworkManager::OnRawDataSent);
        connect(&m_fsd, &FsdClient::RaiseRawDataReceived, this, &NetworkManager::OnRawDataReceived);

        connect(&m_xplaneAdapter, &XplaneAdapter::userAircraftDataChanged, this, &NetworkManager::OnUserAircraftDataUpdated);
        connect(&m_xplaneAdapter, &XplaneAdapter::userAircraftConfigDataChanged, this, &NetworkManager::OnUserAircraftConfigDataUpdated);
        connect(&m_xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, &NetworkManager::OnRadioStackStateChanged);
        connect(&m_xplaneAdapter, &XplaneAdapter::requestStationInfo, this, &NetworkManager::OnRequestControllerInfo);
        connect(&m_xplaneAdapter, &XplaneAdapter::radioMessageSent, this, &NetworkManager::sendRadioMessage);
        connect(&m_xplaneAdapter, &XplaneAdapter::privateMessageSent, this, &NetworkManager::sendPrivateMessage);
        connect(&m_xplaneAdapter, &XplaneAdapter::requestMetar, this, &NetworkManager::RequestMetar);
        connect(&m_xplaneAdapter, &XplaneAdapter::forceDisconnect, this, &NetworkManager::OnForceDisconnected);
        connect(&m_xplaneAdapter, &XplaneAdapter::sendWallop, this, &NetworkManager::OnSendWallop);
        connect(&m_xplaneAdapter, &XplaneAdapter::simPausedStateChanged, this, &NetworkManager::OnSimPaused);

        connect(this, &NetworkManager::notificationPosted, this, [&](QString message, MessageType type)
        {
            m_xplaneAdapter.NotificationPosted(message, toColorHex(type));
        });

        connect(&m_slowPositionTimer, &QTimer::timeout, this, &NetworkManager::OnSlowPositionTimerElapsed);
        connect(&m_fastPositionTimer, &QTimer::timeout, this, &NetworkManager::OnFastPositionTimerElapsed);
        m_fastPositionTimer.setInterval(200);
    }

    NetworkManager::~NetworkManager()
    {
        m_networkLog.close();
    }

    void NetworkManager::OnNetworkConnected()
    {
        if(m_connectInfo.ObserverMode) {
            emit notificationPosted("Connected to network in observer mode.", MessageType::Info);
        }
        else if(m_connectInfo.TowerViewMode) {
            emit notificationPosted("Connected to network in tower view mode.", MessageType::Info);
        } else {
            emit notificationPosted("Connected to network.", MessageType::Info);
        }
        emit networkConnected(m_connectInfo.Callsign, !m_connectInfo.TowerViewMode);

        if(m_connectInfo.TowerViewMode) {
            emit towerviewConnected();
        }

        if(!m_connectInfo.TowerViewMode) {
            m_xplaneAdapter.NetworkConnected(m_connectInfo.Callsign, m_connectInfo.SelcalCode, m_connectInfo.ObserverMode);
        }
    }

    void NetworkManager::OnNetworkDisconnected()
    {
        m_fastPositionTimer.stop();
        m_slowPositionTimer.stop();

        if(m_forcedDisconnect) {
            if(!m_forcedDisconnectReason.isEmpty()) {
                emit notificationPosted("Forcibly disconnected from network: " + m_forcedDisconnectReason, MessageType::Error);
            }
            else {
                emit notificationPosted("Forcibly disconnected from network.", MessageType::Error);
            }
        }
        else {
            emit notificationPosted("Disconnected from network.", MessageType::Info);
        }

        m_xplaneAdapter.NetworkDisconnected();
        emit networkDisconnected(m_forcedDisconnect);

        m_intentionalDisconnect = false;
        m_forcedDisconnect = false;
        m_forcedDisconnectReason = "";
    }

    void NetworkManager::OnForceDisconnected(QString reason)
    {
        m_forcedDisconnect = true;
        m_forcedDisconnectReason = reason;

        m_fsd.SendPDU(PDUDeletePilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId));
        m_fsd.Disconnect();
    }

    void NetworkManager::OnServerIdentificationReceived(PDUServerIdentification pdu)
    {
        m_fsd.SendPDU(PDUClientIdentification(m_connectInfo.Callsign, m_clientProperties.ClientID, "xPilot", FSD_VERSION_MAJOR, FSD_VERSION_MINOR,
                                              AppConfig::getInstance()->VatsimId, QSysInfo::machineUniqueId(), ""));

        GetJwtToken().then([&](const QByteArray &response){
            auto json = QJsonDocument::fromJson(response).object();
            if(json.contains("success") && json["success"].toBool()) {
                if(json.contains("token")) {
                    m_jwtToken = json["token"].toString();
                    LoginToNetwork(m_jwtToken);
                }
            }
            else {
                QString jwtError = json["error_msg"].toString();
                emit notificationPosted(QString("Network authentication error: %1").arg(jwtError), MessageType::Error);
                emit networkDisconnected(true);
                m_fsd.Disconnect();
            }
        }).fail([&](const QString &err){
            emit notificationPosted(QString("Network authentication error: %1").arg(err), MessageType::Error);
            emit networkDisconnected(true);
            m_fsd.Disconnect();
        });
    }

    void NetworkManager::LoginToNetwork(QString password)
    {
        if(m_connectInfo.ObserverMode || m_connectInfo.TowerViewMode) {
            m_fsd.SendPDU(PDUAddATC(m_connectInfo.Callsign, AppConfig::getInstance()->Name, AppConfig::getInstance()->VatsimId,
                                    password, NetworkRating::OBS, ProtocolRevision::Vatsim2022));
        }
        else {
            m_fsd.SendPDU(PDUAddPilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId, password,
                                      NetworkRating::OBS, ProtocolRevision::Vatsim2022, SimulatorType::XPlane,
                                      AppConfig::getInstance()->NameWithHomeAirport()));
        }

        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, "SERVER", ClientQueryType::PublicIP));
        SendSlowPositionPacket();
        SendStoppedFastPositionPacket();
        m_slowPositionTimer.setInterval(m_connectInfo.ObserverMode || m_connectInfo.TowerViewMode ? 15000 : 5000);
        m_slowPositionTimer.start();
    }

    QtPromise::QPromise<QString> NetworkManager::GetBestFsdServer()
    {
        return QtPromise::QPromise<QString>{[&](const auto resolve, const auto reject)
        {
            QNetworkRequest request(QUrl("http://fsd.vatsim.net"));
            m_reply = nam->get(request);

            QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
               if(m_reply->error() == QNetworkReply::NoError) {
                   QHostAddress address(m_reply->readAll().trimmed());
                   if(QAbstractSocket::IPv4Protocol == address.protocol()) {
                       resolve(address.toString());
                   }
                   else {
                       reject();
                   }
               }
               else {
                   reject();
               }
               m_reply->deleteLater();
            });
        }};
    }

    void NetworkManager::OnClientQueryReceived(PDUClientQuery pdu)
    {
        switch(pdu.QueryType)
        {
            case ClientQueryType::AircraftConfiguration:
                emit aircraftConfigurationInfoReceived(pdu.From, pdu.Payload.join(":"));
                break;
            case ClientQueryType::Capabilities:
                if(pdu.From.toUpper() != "SERVER")
                {
                    emit capabilitiesRequestReceived(pdu.From);
                }
                SendCapabilities(pdu.From);
                break;
            case ClientQueryType::COM1Freq:
                {
                    QStringList payload;
                    QString freq = QString::number(m_radioStackState.Com1Frequency / 1000.0, 'f', 3);
                    payload.append(freq);
                    m_fsd.SendPDU(PDUClientQueryResponse(m_connectInfo.Callsign, pdu.From, ClientQueryType::COM1Freq, payload));
                }
                break;
            case ClientQueryType::RealName:
                {
                    QStringList realName;
                    realName.append(AppConfig::getInstance()->NameWithHomeAirport());
                    realName.append(m_connectInfo.TowerViewMode ? "xPilot tower view connection" : "");
                    realName.append(QString::number((int)NetworkRating::OBS));
                    m_fsd.SendPDU(PDUClientQueryResponse(m_connectInfo.Callsign, pdu.From, ClientQueryType::RealName, realName));
                }
                break;
            case ClientQueryType::INF:
                QString inf = QString("xPilot %1 PID=%2 (%3) IP=%4 SYS_UID=%5 FS_VER=XPlane LT=%6 LO=%7 AL=%8")
                        .arg(BuildConfig::getVersionString(),
                             AppConfig::getInstance()->VatsimId,
                             AppConfig::getInstance()->NameWithHomeAirport(),
                             m_publicIp,
                             QSysInfo::machineUniqueId(),
                             QString::number(m_userAircraftData.Latitude),
                             QString::number(m_userAircraftData.Longitude),
                             QString::number(m_userAircraftData.AltitudeMslM * 3.28084));
                m_fsd.SendPDU(PDUTextMessage(m_connectInfo.Callsign, pdu.From, inf));
                break;
        }
    }

    void NetworkManager::OnClientQueryResponseReceived(PDUClientQueryResponse pdu)
    {
        switch(pdu.QueryType)
        {
            case ClientQueryType::PublicIP:
                m_publicIp = pdu.Payload.size() > 0 ? pdu.Payload[0] : "";
                break;
            case ClientQueryType::IsValidATC:
                if(pdu.Payload.at(0).toUpper() == "Y") {
                    emit isValidAtcReceived(pdu.Payload.at(1).toUpper());
                }
                break;
            case ClientQueryType::RealName:
                emit realNameReceived(pdu.From, pdu.Payload.at(0));
                break;
            case ClientQueryType::ATIS:
                if(pdu.Payload.at(0) == "E")
                {
                    // atis end
                    if(m_mapAtisMessages.contains(pdu.From.toUpper()))
                    {
                        emit controllerAtisReceived(pdu.From.toUpper(), m_mapAtisMessages[pdu.From.toUpper()]);

                        m_xplaneAdapter.NotificationPosted(pdu.From.toUpper() + " ATIS:", COLOR_ORANGE);
                        for(const auto& line : qAsConst(m_mapAtisMessages[pdu.From.toUpper()])) {
                            m_xplaneAdapter.NotificationPosted(line, COLOR_ORANGE);
                        }

                        m_mapAtisMessages.remove(pdu.From.toUpper());
                    }
                }
                else if(pdu.Payload.at(0) == "Z")
                {
                    // controller info/controller logoff time
                    if(m_mapAtisMessages.contains(pdu.From.toUpper()))
                    {
                        m_mapAtisMessages[pdu.From.toUpper()].push_back(QString("Estimated logoff time: %1").arg(pdu.Payload[1]));
                    }
                }
                else if(pdu.Payload.at(0) == "T")
                {
                    // controller info/controller logoff time
                    if(m_mapAtisMessages.contains(pdu.From.toUpper()))
                    {
                        m_mapAtisMessages[pdu.From.toUpper()].push_back(pdu.Payload[1]);
                    }
                }
                break;
            case ClientQueryType::Capabilities:
                emit capabilitiesResponseReceived(pdu.From, pdu.Payload.join(":"));
                break;
            case ClientQueryType::Unknown:
            case ClientQueryType::COM1Freq:
            case ClientQueryType::Server:
            case ClientQueryType::INF:
            case ClientQueryType::FlightPlan:
            case ClientQueryType::IPC:
            case ClientQueryType::RequestRelief:
            case ClientQueryType::CancelRequestRelief:
            case ClientQueryType::RequestHelp:
            case ClientQueryType::CancelRequestHelp:
            case ClientQueryType::WhoHas:
            case ClientQueryType::InitiateTrack:
            case ClientQueryType::AcceptHandoff:
            case ClientQueryType::DropTrack:
            case ClientQueryType::SetFinalAltitude:
            case ClientQueryType::SetTempAltitude:
            case ClientQueryType::SetBeaconCode:
            case ClientQueryType::SetScratchpad:
            case ClientQueryType::SetVoiceType:
            case ClientQueryType::AircraftConfiguration:
            case ClientQueryType::NewInfo:
            case ClientQueryType::NewATIS:
            case ClientQueryType::Estimate:
            case ClientQueryType::SetGlobalData:
                break;
        }
    }

    void NetworkManager::OnPilotPositionReceived(PDUPilotPosition pdu)
    {
        QRegularExpression re("^"+ QRegularExpression::escape(pdu.From) +"[A-Z]$");

        if(!m_connectInfo.ObserverMode || m_connectInfo.TowerViewMode || !re.match(m_connectInfo.Callsign).hasMatch())
        {
            AircraftVisualState visualState {};
            visualState.Latitude = pdu.Lat;
            visualState.Longitude = pdu.Lon;
            visualState.Altitude = AdjustIncomingAltitude(pdu.TrueAltitude);
            visualState.Pitch = pdu.Pitch;
            visualState.Heading = pdu.Heading;
            visualState.Bank = pdu.Bank;

            emit slowPositionUpdateReceived(pdu.From, visualState, pdu.GroundSpeed);
        }
    }

    void NetworkManager::OnFastPilotPositionReceived(PDUFastPilotPosition pdu)
    {
        AircraftVisualState visualState {};
        visualState.Latitude = pdu.Lat;
        visualState.Longitude = pdu.Lon;
        visualState.Altitude = AdjustIncomingAltitude(pdu.AltitudeTrue);
        visualState.AltitudeAgl = pdu.AltitudeAgl;
        visualState.Pitch = pdu.Pitch;
        visualState.Heading = pdu.Heading;
        visualState.Bank = pdu.Bank;
        visualState.NoseWheelAngle = pdu.NoseGearAngle;

        if(pdu.Type != FastPilotPositionType::Stopped)
        {
            VelocityVector positionalVelocityVector {};
            positionalVelocityVector.X = pdu.VelocityLongitude;
            positionalVelocityVector.Y = pdu.VelocityAltitude;
            positionalVelocityVector.Z = pdu.VelocityLatitude;

            VelocityVector rotationalVelocityVector {};
            rotationalVelocityVector.X = pdu.VelocityPitch;
            rotationalVelocityVector.Y = pdu.VelocityHeading;
            rotationalVelocityVector.Z = pdu.VelocityBank;

            emit fastPositionUpdateReceived(pdu.From, visualState, positionalVelocityVector, rotationalVelocityVector);
        }
        else
        {
            VelocityVector zero{0,0,0};
            emit fastPositionUpdateReceived(pdu.From, visualState, zero, zero);
        }
    }

    void NetworkManager::OnATCPositionReceived(PDUATCPosition pdu)
    {
        emit controllerUpdateReceived(pdu.From, pdu.Frequencies[0], pdu.Lat, pdu.Lon);
    }

    void NetworkManager::OnMetarResponseReceived(PDUMetarResponse pdu)
    {
        emit metarReceived(pdu.From.toUpper(), pdu.Metar);
        m_xplaneAdapter.NotificationPosted(QString("METAR: %1").arg(pdu.Metar), COLOR_ORANGE);
    }

    void NetworkManager::OnDeletePilotReceived(PDUDeletePilot pdu)
    {
        if(pdu.From.toUpper() == m_connectInfo.Callsign.toUpper()) {
            emit disableVoiceTransmit(); // disables AFV voice transmit
        }
        emit pilotDeleted(pdu.From.toUpper());
    }

    void NetworkManager::OnDeleteATCReceived(PDUDeleteATC pdu)
    {
        emit controllerDeleted(pdu.From.toUpper());
    }

    void NetworkManager::OnPingReceived(PDUPing pdu)
    {
        m_fsd.SendPDU(PDUPong(pdu.To, pdu.From, pdu.Timestamp));
    }

    void NetworkManager::OnTextMessageReceived(PDUTextMessage pdu)
    {
        if(pdu.From.toUpper() == "SERVER")
        {
            emit serverMessageReceived(pdu.Message);
            m_xplaneAdapter.NotificationPosted(pdu.Message, pdu.Message.contains("donate.vatsim.net") ? COLOR_MAGENTA : COLOR_GREEN);
        }
        else
        {
            if(m_connectInfo.TowerViewMode)
            {
                m_fsd.SendPDU(PDUTextMessage(m_connectInfo.Callsign, pdu.From.toUpper(), "This is a xPilot tower view connection. The user is unable to respond to this message. Please contact them through their ATC client connection."));
                return;
            }
            emit privateMessageReceived(pdu.From, pdu.Message);
            m_xplaneAdapter.PrivateMessageReceived(pdu.From.toUpper(), pdu.Message);
        }
    }

    void NetworkManager::OnBroadcastMessageReceived(PDUBroadcastMessage pdu)
    {
        emit broadcastMessageReceived(pdu.From.toUpper(), pdu.Message);
        m_xplaneAdapter.NotificationPosted(QString("[BROADCAST] %1: %2").arg(pdu.From.toUpper(), pdu.Message), COLOR_ORANGE);
    }

    void NetworkManager::OnRadioMessageReceived(PDURadioMessage pdu)
    {
        QList<uint> frequencies;

        for(int i = 0; i < pdu.Frequencies.size(); i++) {

            uint frequency = (uint)pdu.Frequencies[i];

            if(m_radioStackState.Com1ReceiveEnabled && FromNetworkFormat(Normalize25KhzFsdFrequency(frequency)) ==
                    Normalize25KhzFsdFrequency(m_radioStackState.Com1Frequency))
            {
                if(!frequencies.contains(frequency))
                {
                    frequencies.push_back(frequency);
                }
            }
            else if(m_radioStackState.Com2ReceiveEnabled && FromNetworkFormat(Normalize25KhzFsdFrequency(frequency)) ==
                    Normalize25KhzFsdFrequency(m_radioStackState.Com2Frequency))
            {
                if(!frequencies.contains(frequency))
                {
                    frequencies.push_back(frequency);
                }
            }
        }

        if(frequencies.size() == 0) return;

        static QRegularExpression re("^SELCAL ([A-Za-z][A-Za-z]-?[A-Za-z][A-Za-z])$");
        QRegularExpressionMatch match = re.match(pdu.Messages);

        if(match.hasMatch())
        {
            QString selcal = QString("SELCAL %1").arg(match.captured(1).replace("-","")).toUpper();
            if(!m_connectInfo.SelcalCode.isEmpty() && selcal == "SELCAL " + m_connectInfo.SelcalCode.toUpper().replace("-",""))
            {
                emit selcalAlertReceived(pdu.From.toUpper(), frequencies);
                m_xplaneAdapter.selcalAlertReceived();
            }
        }
        else
        {
            bool direct = pdu.Messages.toUpper().startsWith(m_connectInfo.Callsign.toUpper());

            RadioMessageReceived args{};
            args.Frequencies = QVariant::fromValue(frequencies);
            args.From = pdu.From.toUpper();
            args.Message = pdu.Messages;
            args.IsDirect = direct;
            args.DualReceiver = m_radioStackState.Com1ReceiveEnabled && m_radioStackState.Com2ReceiveEnabled;

            emit radioMessageReceived(args);
            m_xplaneAdapter.RadioMessageReceived(pdu.From.toUpper(), pdu.Messages, direct);
        }
    }

    void NetworkManager::OnPlaneInfoRequestReceived(PDUPlaneInfoRequest pdu)
    {
        static QRegularExpression re("^([A-Z]{3})\\d+");
        QRegularExpressionMatch match = re.match(m_connectInfo.Callsign);

        m_fsd.SendPDU(PDUPlaneInfoResponse(m_connectInfo.Callsign, pdu.From, m_connectInfo.TypeCode, match.hasMatch() ? match.captured(1)  : "", "", ""));
    }

    void NetworkManager::OnPlaneInfoResponseReceived(PDUPlaneInfoResponse pdu)
    {
        emit aircraftInfoReceived(pdu.From, pdu.Equipment, pdu.Airline);
    }

    void NetworkManager::OnKillRequestReceived(PDUKillRequest pdu)
    {
        m_forcedDisconnect = true;
        m_forcedDisconnectReason = pdu.Reason;
    }

    void NetworkManager::OnSendFastReceived(PDUSendFast pdu)
    {
        if(pdu.To.toUpper() == m_connectInfo.Callsign.toUpper())
        {
            if(pdu.DoSendFast)
            {
                m_fastPositionTimer.start();
            }
            else
            {
                m_fastPositionTimer.stop();
                SendStoppedFastPositionPacket();
            }
        }
    }

    void NetworkManager::OnUserAircraftDataUpdated(UserAircraftData data)
    {
        if(m_userAircraftData != data)
        {
            m_userAircraftData = data;
        }

        if(IsXplane12())
        {
            m_altitudeDelta = m_userAircraftData.AltimeterTemperatureError;
        }
        else
        {
            m_altitudeDelta = 0;
        }
    }

    void NetworkManager::OnUserAircraftConfigDataUpdated(UserAircraftConfigData data)
    {
        if(m_userAircraftConfigData != data)
        {
            m_userAircraftConfigData = data;
        }
    }

    void NetworkManager::OnRadioStackStateChanged(RadioStackState radioStack)
    {
        if(m_radioStackState != radioStack)
        {
            m_radioStackState = radioStack;

            m_transmitFreqs.clear();
            if(m_radioStackState.Com1TransmitEnabled)
            {
                m_transmitFreqs.append(Normalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com1Frequency)));
                if(!m_transmitFreqs.contains(Denormalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com1Frequency))))
                {
                    m_transmitFreqs.append(Denormalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com1Frequency)));
                }
            }
            else if(m_radioStackState.Com2TransmitEnabled)
            {
                m_transmitFreqs.append(Normalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com2Frequency)));
                if(!m_transmitFreqs.contains(Denormalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com2Frequency))))
                {
                    m_transmitFreqs.append(Denormalize25KhzFsdFrequency(MatchFsdFormat(m_radioStackState.Com2Frequency)));
                }
            }
        }
    }

    void NetworkManager::OnRequestControllerInfo(QString callsign)
    {
        requestControllerAtis(callsign);
    }

    void NetworkManager::SendSlowPositionPacket()
    {
        if(m_connectInfo.ObserverMode || m_connectInfo.TowerViewMode) {
            QList<int> freqs = {99998};
            m_fsd.SendPDU(PDUATCPosition(m_connectInfo.Callsign, freqs, NetworkFacility::OBS, 40, NetworkRating::OBS,
                                         m_userAircraftData.Latitude, m_userAircraftData.Longitude));
        }
        else {
            m_fsd.SendPDU(PDUPilotPosition(m_connectInfo.Callsign,
                                           m_radioStackState.TransponderCode,
                                           m_radioStackState.SquawkingModeC,
                                           m_radioStackState.SquawkingIdent,
                                           NetworkRating::OBS,
                                           m_userAircraftData.Latitude,
                                           m_userAircraftData.Longitude,
                                           (m_userAircraftData.AltitudeMslM * 3.28084) + m_altitudeDelta,
                                           GetPressureAltitude(),
                                           m_userAircraftData.GroundSpeed,
                                           m_userAircraftData.Pitch,
                                           m_userAircraftData.Heading,
                                           m_userAircraftData.Bank));
        }
    }

    void NetworkManager::SendFastPositionPacket(bool sendSlowFast)
    {
        if(!m_connectInfo.ObserverMode && !m_connectInfo.TowerViewMode)
        {
            m_fsd.SendPDU(PDUFastPilotPosition(sendSlowFast ? FastPilotPositionType::Slow : FastPilotPositionType::Fast,
                                               m_connectInfo.Callsign,
                                               m_userAircraftData.Latitude,
                                               m_userAircraftData.Longitude,
                                               (m_userAircraftData.AltitudeMslM * 3.28084) + m_altitudeDelta,
                                               m_userAircraftData.AltitudeAglM * 3.28084,
                                               m_userAircraftData.Pitch,
                                               m_userAircraftData.Heading,
                                               m_userAircraftData.Bank,
                                               m_userAircraftData.LongitudeVelocity,
                                               m_userAircraftData.AltitudeVelocity,
                                               m_userAircraftData.LatitudeVelocity,
                                               m_userAircraftData.PitchVelocity,
                                               m_userAircraftData.HeadingVelocity,
                                               m_userAircraftData.BankVelocity,
                                               m_userAircraftData.NoseWheelAngle));
        }
    }

    void NetworkManager::SendZeroVelocityFastPositionPacket()
    {
        if(!m_connectInfo.ObserverMode && !m_connectInfo.ObserverMode) {
            m_fsd.SendPDU(PDUFastPilotPosition(FastPilotPositionType::Fast,
                                               m_connectInfo.Callsign,
                                               m_userAircraftData.Latitude,
                                               m_userAircraftData.Longitude,
                                               (m_userAircraftData.AltitudeMslM * 3.28084) + m_altitudeDelta,
                                               m_userAircraftData.AltitudeAglM * 3.28084,
                                               m_userAircraftData.Pitch,
                                               m_userAircraftData.Heading,
                                               m_userAircraftData.Bank,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0));
        }
    }

    void NetworkManager::SendStoppedFastPositionPacket()
    {
        if(!m_connectInfo.ObserverMode && !m_connectInfo.TowerViewMode)
        {
            m_fsd.SendPDU(PDUFastPilotPosition(FastPilotPositionType::Stopped,
                                               m_connectInfo.Callsign,
                                               m_userAircraftData.Latitude,
                                               m_userAircraftData.Longitude,
                                               (m_userAircraftData.AltitudeMslM * 3.28084) + m_altitudeDelta,
                                               m_userAircraftData.AltitudeAglM * 3.28084,
                                               m_userAircraftData.Pitch,
                                               m_userAircraftData.Heading,
                                               m_userAircraftData.Bank,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0,
                                               0.0));
        }
    }

    void NetworkManager::OnSlowPositionTimerElapsed()
    {
        if(m_simPaused) {
            SendZeroVelocityFastPositionPacket();
        }
        else if(!PositionalVelocityIsZero(m_userAircraftData)) {
            SendFastPositionPacket(true);
        }
        SendSlowPositionPacket();
    }

    void NetworkManager::OnFastPositionTimerElapsed()
    {
        if(m_simPaused) {
            SendZeroVelocityFastPositionPacket();
        }
        else if(!PositionalVelocityIsZero(m_userAircraftData)) {
            SendFastPositionPacket();
        }
        else {
            SendStoppedFastPositionPacket();
        }
    }

    void NetworkManager::SendAircraftConfigurationUpdate(AircraftConfiguration config)
    {
        SendAircraftConfigurationUpdate("@94836", config);
    }

    void NetworkManager::SendAircraftConfigurationUpdate(QString to, AircraftConfiguration config)
    {
        AircraftConfigurationInfo acconfig{};
        acconfig.Config = config;

        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, to, ClientQueryType::AircraftConfiguration, {acconfig.ToJson()}));
    }

    void NetworkManager::SendCapabilities(QString to)
    {
        QStringList caps;
        caps.append("VERSION=1");
        caps.append("ATCINFO=1");
        caps.append("MODELDESC=1");
        caps.append("ACCONFIG=1");
        caps.append("VISUPDATE=1");
        m_fsd.SendPDU(PDUClientQueryResponse(m_connectInfo.Callsign, to, ClientQueryType::Capabilities, caps));
    }

    QtPromise::QPromise<QByteArray> NetworkManager::GetJwtToken()
    {
        return QtPromise::QPromise<QByteArray>{[&](const auto resolve, const auto reject)
            {
                QJsonObject obj;
                obj["cid"] = AppConfig::getInstance()->VatsimId;
                obj["password"] = AppConfig::getInstance()->VatsimPasswordDecrypted;
                QJsonDocument doc(obj);
                QByteArray data = doc.toJson();

                const QUrl url(QStringLiteral("https://auth.vatsim.net/api/fsd-jwt"));
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                m_reply = nam->post(request, data);

                QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
                    if(m_reply->error() == QNetworkReply::NoError) {
                        resolve(m_reply->readAll());
                    }
                    else {
                        reject(QString{m_reply->errorString()});
                    }
                    m_reply->deleteLater();
                });
            }};
    }

    void NetworkManager::RequestMetar(QString station)
    {
        m_fsd.SendPDU(PDUMetarRequest(m_connectInfo.Callsign, station));
    }

    void NetworkManager::requestRealName(QString callsign)
    {
        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, callsign, ClientQueryType::RealName));
    }

    void NetworkManager::requestControllerAtis(QString callsign)
    {
        if(!m_mapAtisMessages.contains(callsign.toUpper()))
        {
            m_mapAtisMessages.insert(callsign.toUpper(), {});
            m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, callsign, ClientQueryType::ATIS));
        }
    }

    void NetworkManager::requestMetar(QString station)
    {
        m_fsd.SendPDU(PDUMetarRequest(m_connectInfo.Callsign, station));
    }

    void NetworkManager::sendWallop(QString message)
    {
        OnSendWallop(message);
    }

    void NetworkManager::RequestIsValidATC(QString callsign)
    {
        QStringList args;
        args.append(callsign);
        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, "SERVER", ClientQueryType::IsValidATC, args));
    }

    void NetworkManager::RequestCapabilities(QString callsign)
    {
        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, callsign, ClientQueryType::Capabilities));
    }

    void NetworkManager::SendAircraftInfoRequest(QString callsign)
    {
        m_fsd.SendPDU(PDUPlaneInfoRequest(m_connectInfo.Callsign, callsign));
    }

    void NetworkManager::SendAircraftConfigurationRequest(QString callsign)
    {
        AircraftConfigurationInfo acconfig;
        acconfig.FullRequest = true;

        QStringList args;
        args.append(acconfig.ToJson());

        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, callsign, ClientQueryType::AircraftConfiguration, args));
    }

    void NetworkManager::connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer)
    {
        if(AppConfig::getInstance()->configRequired()) {
            emit notificationPosted("It looks like this may be the first time you've run xPilot on this computer. Some configuration items are required before you can connect to the network. Open Settings and verify your network credentials are saved.", MessageType::Error);
            return;
        }
        if(!AppConfig::getInstance()->MicrophoneCalibrated)
        {
            emit microphoneCalibrationRequired();
            AppConfig::getInstance()->MicrophoneCalibrated = true;
            AppConfig::getInstance()->saveConfig();
            return;
        }
        if(!callsign.isEmpty() && !typeCode.isEmpty()) {
            ConnectInfo connectInfo{};
            connectInfo.Callsign = callsign;
            connectInfo.TypeCode = typeCode;
            connectInfo.SelcalCode = selcal;
            connectInfo.ObserverMode = observer;
            m_connectInfo = connectInfo;

            m_clientProperties = {"xPilot", FSD_VERSION_MAJOR, FSD_VERSION_MINOR, BuildConfig::VatsimClientId(), BuildConfig::VatsimClientKey()};
            m_fsd.SetClientProperties(m_clientProperties);

            emit notificationPosted("Connecting to network...", MessageType::Info);

            QString serverName = AppConfig::getInstance()->getNetworkServer();
            if(AppConfig::getInstance()->ServerName == "AUTOMATIC") {
                GetBestFsdServer().then([&](const QString& bestServer) {
                    m_fsd.Connect(bestServer, 6809);
                }).fail([&, serverName](){
                    m_fsd.Connect(serverName, 6809);
                });
            }
            else {
                m_fsd.Connect(serverName, 6809);
            }
        }
        else
        {
            emit notificationPosted("Callsign and Type Code are required.", MessageType::Error);
        }
    }

    void NetworkManager::connectTowerView()
    {
        if(AppConfig::getInstance()->configRequired()) {
            emit notificationPosted("It looks like this may be the first time you've run xPilot on this computer. Some configuration items are required before you can connect to the network. Open Settings and verify your network credentials are saved.", MessageType::Error);
            return;
        }

        ConnectInfo connectInfo{};
        connectInfo.Callsign = AppConfig::getInstance()->VatsimId + "_TV";
        connectInfo.TowerViewMode = true;
        m_connectInfo = connectInfo;

        m_clientProperties = {"xPilot", FSD_VERSION_MAJOR, FSD_VERSION_MINOR, BuildConfig::TowerviewClientId(), BuildConfig::VatsimClientKey()};
        m_fsd.SetClientProperties(m_clientProperties);

        emit notificationPosted("Connecting to network...", MessageType::Info);

        QString serverName = AppConfig::getInstance()->getNetworkServer();
        if(AppConfig::getInstance()->ServerName == "AUTOMATIC") {
            GetBestFsdServer().then([&](const QString& bestServer) {
                m_fsd.Connect(bestServer, 6809);
            }).fail([&, serverName](){
                m_fsd.Connect(serverName, 6809);
            });
        }
        else {
            m_fsd.Connect(serverName, 6809);
        }
    }

    void NetworkManager::disconnectFromNetwork()
    {
        if(!m_fsd.IsConnected())
        {
            return;
        }

        m_intentionalDisconnect = true;
        m_fsd.SendPDU(PDUDeletePilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId));
        m_fsd.Disconnect();
    }

    void NetworkManager::sendRadioMessage(QString message)
    {
        if(m_transmitFreqs.size() > 0) {
            m_fsd.SendPDU(PDURadioMessage(m_connectInfo.Callsign, m_transmitFreqs, message));
            m_xplaneAdapter.SendRadioMessage(message);
        }
    }

    void NetworkManager::sendPrivateMessage(QString to, QString message)
    {
        m_fsd.SendPDU(PDUTextMessage(m_connectInfo.Callsign, to.toUpper(), message));
        m_xplaneAdapter.SendPrivateMessage(to, message);
        emit privateMessageSent(to, message);
    }

    void NetworkManager::OnNetworkError(QString error)
    {
        emit notificationPosted(error, MessageType::Error);
    }

    void NetworkManager::OnProtocolErrorReceived(PDUProtocolError error)
    {
        emit notificationPosted(QString("Network Error: %1").arg(error.Message), MessageType::Error);
    }

    void NetworkManager::OnRawDataSent(QString data)
    {
        if(!AppConfig::getInstance()->VatsimPasswordDecrypted.isEmpty())
        {
            data = data.replace(AppConfig::getInstance()->VatsimPasswordDecrypted, "******");
        }
        if(data.startsWith("#AA", Qt::CaseInsensitive) || data.startsWith("#AP", Qt::CaseInsensitive))
        {
            data = data.replace(m_jwtToken, "******");
        }
        m_rawDataStream << QString("[%1] >>> %2").arg(QDateTime::currentDateTimeUtc().toString("HH:mm:ss.zzz"), data);
        m_rawDataStream.flush();
    }

    void NetworkManager::OnRawDataReceived(QString data)
    {
        m_rawDataStream << QString("[%1] <<< %2").arg(QDateTime::currentDateTimeUtc().toString("HH:mm:ss.zzz"), data);
        m_rawDataStream.flush();
    }

    void NetworkManager::OnSendWallop(QString message)
    {
        m_fsd.SendPDU(PDUWallop(m_connectInfo.Callsign, message));
        emit wallopSent(message);
        m_xplaneAdapter.NotificationPosted(QString("[WALLOP] %1").arg(message), COLOR_RED);
    }

    void NetworkManager::OnSimPaused(bool isPaused)
    {
        m_simPaused = isPaused;
    }

    double NetworkManager::CalculatePressureAltitude() const
    {
        const double deltaPressure = (1013.25 - m_userAircraftData.BarometerSeaLevel) * 30.0; // 30ft per mbar
        return (m_userAircraftData.AltitudeMslM * 3.28084) + deltaPressure;
    }

    double NetworkManager::GetPressureAltitude() const
    {
        if(IsXplane12()) {
            return m_userAircraftData.AltitudePressure;
        }
        return CalculatePressureAltitude();
    }

    double NetworkManager::AdjustIncomingAltitude(double altitude)
    {
        if(!IsXplane12()) {
            return altitude;
        }

        // sim/flightmodel/position/elevation is true altitude corrected for temperature
        // for comparison we need to add the error back in 
        double userTrueAltitude = m_userAircraftData.AltitudeMslM * 3.28084 + m_userAircraftData.AltimeterTemperatureError;

        double verticalDistance = std::abs(userTrueAltitude - altitude);
        if(verticalDistance > 6000.0) {
            return altitude;
        }

        double weight = 1.0;
        if(verticalDistance > 3000.0) {
            weight = 1.0 - ((verticalDistance - 3000.0) / 3000.0);
        }

        // Network altitude is uncorrected, we need to substract the error
        return altitude - (m_userAircraftData.AltimeterTemperatureError * weight);
    }
}

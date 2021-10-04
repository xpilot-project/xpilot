#include "networkmanager.h"
#include "networkserverlist.h"
#include "src/appcore.h"
#include "src/appconfig.h"
#include "src/version.h"

namespace xpilot
{
    NetworkManager::NetworkManager(UdpClient &udpClient, QObject *owner) : QObject(owner)
    {
        connect(&m_fsd, &FsdClient::RaiseNetworkError, this, &NetworkManager::OnNetworkError);
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
        connect(&m_fsd, &FsdClient::RaiseMetarRequestReceived, this, &NetworkManager::OnMetarRequestReceived);
        connect(&m_fsd, &FsdClient::RaiseMetarResponseReceived, this, &NetworkManager::OnMetarResponseReceived);
        connect(&m_fsd, &FsdClient::RaisePlaneInfoRequestReceived, this, &NetworkManager::OnPlaneInfoRequestReceived);
        connect(&m_fsd, &FsdClient::RaisePlaneInfoResponseReceived, this, &NetworkManager::OnPlaneInfoResponseReceived);
        connect(&m_fsd, &FsdClient::RaiseKillRequestReceived, this, &NetworkManager::OnKillRequestReceived);

        connect(&udpClient, &UdpClient::userAircraftDataChanged, this, &NetworkManager::OnUserAircraftDataUpdated);
        connect(&udpClient, &UdpClient::userAircraftConfigDataChanged, this, &NetworkManager::OnUserAircraftConfigDataUpdated);
        connect(&udpClient, &UdpClient::radioStackStateChanged, this, &NetworkManager::OnRadioStackStateChanged);

        m_slowPositionTimer = new QTimer(this);
        connect(m_slowPositionTimer, &QTimer::timeout, this, &NetworkManager::OnSlowPositionTimerElapsed);

        m_fastPositionTimer = new QTimer(this);
        m_fastPositionTimer->setInterval(200);
        connect(m_fastPositionTimer, &QTimer::timeout, this, &NetworkManager::OnFastPositionTimerElapsed);
    }

    void NetworkManager::OnNetworkConnected()
    {
        if(m_connectInfo.ObserverMode) {
            emit notificationPosted((int)NotificationType::Info, "Connected to network in observer mode.");
        } else {
            emit notificationPosted((int)NotificationType::Info, "Connected to network.");
        }
        emit networkConnected();
    }

    void NetworkManager::OnNetworkDisconnected()
    {
        m_fastPositionTimer->stop();
        m_slowPositionTimer->stop();

        if(m_forcedDisconnect) {
            if(!m_forcedDisconnectReason.isEmpty()) {
                emit notificationPosted((int)NotificationType::Error, "Forcibly disconnected from network: " + m_forcedDisconnectReason);
            }
            else {
                emit notificationPosted((int)NotificationType::Error, "Forcibly disconnected from network.");
            }
        }
        else {
            emit notificationPosted((int)NotificationType::Info, "Disconnected from network.");
        }

        m_intentionalDisconnect = false;
        m_forcedDisconnect = false;
        m_forcedDisconnectReason = "";

        emit networkDisconnected();
    }

    void NetworkManager::OnServerIdentificationReceived(PDUServerIdentification pdu)
    {
        m_fsd.SendPDU(PDUClientIdentification(m_connectInfo.Callsign, GetClientId(), "xPilot", 1, 2, AppConfig::getInstance()->VatsimId, GetSystemUid(), ""));

        if(m_connectInfo.ObserverMode) {
            m_fsd.SendPDU(PDUAddATC(m_connectInfo.Callsign, AppConfig::getInstance()->Name, AppConfig::getInstance()->VatsimId,
                                    AppConfig::getInstance()->VatsimPasswordDecrypted, NetworkRating::OBS, ProtocolRevision::VatsimAuth));
        }
        else {
            m_fsd.SendPDU(PDUAddPilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId, AppConfig::getInstance()->VatsimPasswordDecrypted,
                                      NetworkRating::OBS, ProtocolRevision::VatsimAuth, SimulatorType::XPlane, AppConfig::getInstance()->Name));
        }

        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, "SERVER", ClientQueryType::PublicIP));
        SendSlowPositionPacket();
        SendEmptyFastPositionPacket();
        m_slowPositionTimer->setInterval(m_connectInfo.ObserverMode ? 15000 : 5000);
        m_slowPositionTimer->start();
        if(m_velocityEnabled && !m_connectInfo.ObserverMode)
        {
            m_fastPositionTimer->start();
        }
    }

    void NetworkManager::OnClientQueryReceived(PDUClientQuery pdu)
    {
        switch(pdu.QueryType)
        {
        case ClientQueryType::AircraftConfiguration:
            emit aircraftConfigurationInfoReceived(pdu.From, pdu.Payload.join(":"));
            break;
        case ClientQueryType::Capabilities:
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
            realName.append(AppConfig::getInstance()->Name);
            realName.append("");
            realName.append("1");
            m_fsd.SendPDU(PDUClientQueryResponse(m_connectInfo.Callsign, pdu.From, ClientQueryType::RealName, realName));
        }
            break;
        case ClientQueryType::INF:
            QString inf = QString("xPilot %1 PID=%2 (%3) IP=%4 SYS_UID=%5 FS_VER=XPlane LT=%6 LO=%7 AL=%8")
                    .arg(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH),
                         AppConfig::getInstance()->VatsimId,
                         AppConfig::getInstance()->Name,
                         m_publicIp,
                         GetSystemUid(),
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
        }
    }

    void NetworkManager::OnPilotPositionReceived(PDUPilotPosition pdu)
    {

    }

    void NetworkManager::OnFastPilotPositionReceived(PDUFastPilotPosition pdu)
    {

    }

    void NetworkManager::OnATCPositionReceived(PDUATCPosition pdu)
    {

    }

    void NetworkManager::OnMetarResponseReceived(PDUMetarResponse pdu)
    {

    }

    void NetworkManager::OnMetarRequestReceived(PDUMetarRequest pdu)
    {

    }

    void NetworkManager::OnDeletePilotReceived(PDUDeletePilot pdu)
    {

    }

    void NetworkManager::OnDeleteATCReceived(PDUDeleteATC pdu)
    {

    }

    void NetworkManager::OnPingReceived(PDUPing pdu)
    {
        m_fsd.SendPDU(PDUPong(pdu.To, pdu.From, pdu.Timestamp));
    }

    void NetworkManager::OnTextMessageReceived(PDUTextMessage pdu)
    {
        if(pdu.From.toUpper() == "SERVER")
        {
            emit notificationPosted((int)NotificationType::ServerMessage, pdu.Message);
        }
        else
        {
            emit privateMessageReceived(pdu.From, pdu.Message);
        }
    }

    void NetworkManager::OnBroadcastMessageReceived(PDUBroadcastMessage pdu)
    {

    }

    void NetworkManager::OnRadioMessageReceived(PDURadioMessage pdu)
    {

    }

    void NetworkManager::OnPlaneInfoRequestReceived(PDUPlaneInfoRequest pdu)
    {

    }

    void NetworkManager::OnPlaneInfoResponseReceived(PDUPlaneInfoResponse pdu)
    {

    }

    void NetworkManager::OnKillRequestReceived(PDUKillRequest pdu)
    {
        m_forcedDisconnect = true;
        m_forcedDisconnectReason = pdu.Reason;
    }

    void NetworkManager::OnUserAircraftDataUpdated(UserAircraftData data)
    {
        if(m_userAircraftData != data)
        {
            m_userAircraftData = data;
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
        }
    }

    void NetworkManager::SendSlowPositionPacket()
    {
        if(m_connectInfo.ObserverMode) {
            m_fsd.SendPDU(PDUATCPosition(m_connectInfo.Callsign,99998, NetworkFacility::OBS, 40, NetworkRating::OBS,
                                         m_userAircraftData.Latitude, m_userAircraftData.Longitude));
        }
        else {
            m_fsd.SendPDU(PDUPilotPosition(m_connectInfo.Callsign, m_radioStackState.TransponderCode, m_radioStackState.SquawkingModeC,
                                           m_radioStackState.SquawkingIdent, NetworkRating::OBS, m_userAircraftData.Latitude,
                                           m_userAircraftData.Longitude, m_userAircraftData.AltitudeMslM * 3.28084,
                                           m_userAircraftData.AltitudeAglM * 3.28084, m_userAircraftData.GroundSpeed,
                                           m_userAircraftData.Pitch, m_userAircraftData.Heading, m_userAircraftData.Bank));
        }
    }

    void NetworkManager::SendFastPositionPacket()
    {
        if(m_velocityEnabled && !m_connectInfo.ObserverMode)
        {
            m_fsd.SendPDU(PDUFastPilotPosition(m_connectInfo.Callsign, m_userAircraftData.Latitude, m_userAircraftData.Longitude,
                                               m_userAircraftData.AltitudeMslM * 3.28084, m_userAircraftData.Pitch, m_userAircraftData.Heading,
                                               m_userAircraftData.Bank, m_userAircraftData.LongitudeVelocity, m_userAircraftData.AltitudeVelocity,
                                               m_userAircraftData.LatitudeVelocity, m_userAircraftData.PitchVelocity, m_userAircraftData.HeadingVelocity,
                                               m_userAircraftData.BankVelocity));
        }
    }

    void NetworkManager::SendEmptyFastPositionPacket()
    {
        if(!m_connectInfo.ObserverMode && m_velocityEnabled)
        {
            m_fsd.SendPDU(PDUFastPilotPosition(m_connectInfo.Callsign, m_userAircraftData.Latitude, m_userAircraftData.Longitude,
                                               m_userAircraftData.AltitudeMslM * 3.28084, m_userAircraftData.Pitch, m_userAircraftData.Heading,
                                               m_userAircraftData.Bank, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
        }
    }

    void NetworkManager::OnSlowPositionTimerElapsed()
    {
        SendSlowPositionPacket();
    }

    void NetworkManager::OnFastPositionTimerElapsed()
    {
        SendFastPositionPacket();
    }

    void NetworkManager::SendAircraftConfigurationUpdate(AircraftConfiguration config)
    {
        SendAircraftConfigurationUpdate("@94836", config);
    }

    void NetworkManager::SendAircraftConfigurationUpdate(QString to, AircraftConfiguration config)
    {
        AircraftConfigurationInfo acconfig{};
        acconfig.Config = config;

        QStringList payload;
        if(config.IsFullData) {
            payload.append(acconfig.ToFullJson());
        }
        else{
            payload.append(acconfig.ToIncrementalJson());
        }

        m_fsd.SendPDU(PDUClientQuery(m_connectInfo.Callsign, to, ClientQueryType::AircraftConfiguration, payload));
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

    void NetworkManager::connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer)
    {
        if(AppConfig::getInstance()->configRequired()) {
            emit notificationPosted((int)NotificationType::Error, "It looks like this may be the first time you've run xPilot on this computer. "
"Some configuration items are required before you can connect to the network. Open Settings and verify your network credentials are saved.");
            return;
        }
        if(!callsign.isEmpty() && !typeCode.isEmpty()) {
            ConnectInfo connectInfo{};
            connectInfo.Callsign = callsign;
            connectInfo.TypeCode = typeCode;
            connectInfo.SelcalCode = selcal;
            connectInfo.ObserverMode = observer;
            m_connectInfo = connectInfo;

            emit notificationPosted((int)NotificationType::Info, "Connecting to network...");
            m_fsd.Connect(AppConfig::getInstance()->getNetworkServer(), 6809);
        }
        else
        {
            emit notificationPosted((int)NotificationType::Error, "Callsign and Type Code are required.");
        }
    }

    void NetworkManager::disconnectFromNetwork()
    {
        m_intentionalDisconnect = true;
        m_fsd.SendPDU(PDUDeletePilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId));
        m_fsd.Disconnect();
    }

    void NetworkManager::OnNetworkError(QString error)
    {
        emit notificationPosted((int)NotificationType::Error, error);
    }
}

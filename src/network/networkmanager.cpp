#include "networkmanager.h"
#include "networkserverlist.h"
#include "../appcore.h"
#include "../appconfig.h"

namespace xpilot
{
    NetworkManager::NetworkManager(QObject *owner) : QObject(owner)
    {
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
    }

    void NetworkManager::connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer)
    {
        if(AppConfig::getInstance()->configRequired()) {
            emit notificationPosted((int)NotificationType::Error, "It looks like this may be the first time you've run xPilot on this computer. Some configuration items are required before you can connect to the network. Open Settings and verify your network credentials are saved.");
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
            m_fsd.Connect("192.168.50.56", 6809);
        }
        else
        {
            emit notificationPosted((int)NotificationType::Error, "Callsign and Type Code are required.");
        }
    }

    void NetworkManager::disconnectFromNetwork()
    {
        m_fsd.Disconnect();
    }

    void NetworkManager::OnNetworkConnected()
    {
        emit notificationPosted((int)NotificationType::Info, "Connected to network.");
        emit networkConnected();
    }

    void NetworkManager::OnNetworkDisconnected()
    {
        emit notificationPosted((int)NotificationType::Info, "Disconnected from network.");
        emit networkDisconnected();
    }

    void NetworkManager::OnServerIdentificationReceived(PDUServerIdentification pdu)
    {
        m_fsd.SendPDU(PDUClientIdentification(m_connectInfo.Callsign, GetClientId(), "xPilot", 1, 0, AppConfig::getInstance()->VatsimId, GetSystemUid(), ""));
        m_fsd.SendPDU(PDUAddPilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId, AppConfig::getInstance()->VatsimPasswordDecrypted, NetworkRating::OBS,
                                  ProtocolRevision::VatsimAuth, SimulatorType::XPlane, AppConfig::getInstance()->Name));
    }

    void NetworkManager::OnClientQueryReceived(PDUClientQuery pdu)
    {

    }

    void NetworkManager::OnClientQueryResponseReceived(PDUClientQueryResponse pdu)
    {

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
        emit notificationPosted((int)NotificationType::TextMessage, pdu.Message);
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

    }

    void NetworkManager::HandleServerListDownloaded()
    {
        qDebug() << "Downloaded";
    }
}

#include "networkmanager.h"
#include "networkserverlist.h"
#include "../appcore.h"
#include "../appconfig.h"

namespace xpilot
{
    NetworkManager::NetworkManager(QObject *owner) : QObject(owner)
    {
        connect(&m_fsd, &FsdClient::RaiseNetworkConnected, this, &NetworkManager::HandleNetworkConnected);
        connect(&m_fsd, &FsdClient::RaiseNetworkDisconnected, this, &NetworkManager::HandleNetworkDisconnected);
        connect(&m_fsd, &FsdClient::RaiseServerIdentificationReceived, this, &NetworkManager::HandleServerIdentificationReceived);
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

    void NetworkManager::HandleNetworkConnected()
    {
        emit notificationPosted((int)NotificationType::Info, "Connected to network.");
        emit networkConnected();
    }

    void NetworkManager::HandleNetworkDisconnected()
    {
        emit notificationPosted((int)NotificationType::Info, "Disconnected from network.");
        emit networkDisconnected();
    }

    void NetworkManager::HandleServerIdentificationReceived(PDUServerIdentification pdu)
    {
        m_fsd.SendPDU(PDUClientIdentification(m_connectInfo.Callsign, GetClientId(), "xPilot", 1, 0, AppConfig::getInstance()->VatsimId, GetSystemUid(), ""));
        m_fsd.SendPDU(PDUAddPilot(m_connectInfo.Callsign, AppConfig::getInstance()->VatsimId, AppConfig::getInstance()->VatsimPasswordDecrypted, NetworkRating::OBS,
                                  ProtocolRevision::VatsimAuth, SimulatorType::XPlane, AppConfig::getInstance()->Name));
    }

    void NetworkManager::HandleServerListDownloaded()
    {
        qDebug() << "Downloaded";
    }
}

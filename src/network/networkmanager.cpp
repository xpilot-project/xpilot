#include "../appcore.h"
#include "networkmanager.h"

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
        if(!callsign.isEmpty() && !typeCode.isEmpty()) {
            ConnectInfo connectInfo;
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
        m_fsd.SendPDU(PDUClientIdentification(m_connectInfo.Callsign, GetClientId(), "xPilot", 1, 0, "1215759", GetSystemUid(), ""));
        m_fsd.SendPDU(PDUAddPilot(m_connectInfo.Callsign, "1215759", "3rBaF46EQ3tRNWe", NetworkRating::OBS, ProtocolRevision::VatsimAuth, SimulatorType::XPlane, "Justin"));
    }
}

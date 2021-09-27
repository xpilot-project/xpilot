#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include "connectinfo.h"
#include "../fsd/fsd_client.h"
#include "../fsd/pdu/pdu_base.h"
#include "../fsd/pdu/pdu_add_pilot.h"

namespace xpilot
{
    class NetworkManager : public QObject
    {
        Q_OBJECT

    public:
        NetworkManager(QObject *owner = nullptr);

    signals:
        void networkConnected();
        void networkDisconnected();
        void notificationPosted(int type, QString message);

    public slots:
        void connectToNetwork(QString callsign, QString typeCode, QString selcal, bool observer);
        void disconnectFromNetwork();

    private:
        FsdClient m_fsd { this };
        ConnectInfo m_connectInfo{};

        void HandleNetworkConnected();
        void HandleNetworkDisconnected();
        void HandleServerIdentificationReceived(PDUServerIdentification pdu);

        void HandleServerListDownloaded();
    };
}

#endif

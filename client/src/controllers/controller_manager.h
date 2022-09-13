#ifndef CONTROLLER_MANAGER_H
#define CONTROLLER_MANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>

#include "controller.h"
#include "src/network/networkmanager.h"
#include "src/simulator/xplane_adapter.h"

namespace xpilot
{
    class ControllerManager : public QObject
    {
        Q_OBJECT

    public:
        ControllerManager(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject* parent = nullptr);
        void ValidateController(Controller& controller);

    signals:
        void controllerAdded(Controller controller);
        void controllerDeleted(Controller controller);
        void controllerUpdated(Controller controller);

    private:
        void OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon);
        void IsValidATCReceived(QString callsign);
        void OnRealNameReceived(QString callsign, QString realName);
        void RefreshController(Controller controller);
        void OnControllerDeleted(QString callsign);
        void OnRadioStackStateChanged(RadioStackState radioStack);
        void UpdateStationCallsigns();

    private:
        NetworkManager &m_networkManager;
        XplaneAdapter &m_xplaneAdapter;
        QList<Controller> m_controllers;
        QTimer m_nearbyAtcTimer;
        RadioStackState m_radioStackState;
    };
}

#endif

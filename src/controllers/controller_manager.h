#ifndef CONTROLLER_MANAGER_H
#define CONTROLLER_MANAGER_H

#include <QObject>
#include <QList>

#include "controller.h"
#include "src/network/networkmanager.h"

namespace xpilot
{
    class ControllerManager : public QObject
    {
        Q_OBJECT

    public:
        ControllerManager(NetworkManager& networkManager, QObject* parent = nullptr);
        void ValidateController(Controller& controller);

    signals:
        void controllerAdded(Controller controller);
        void controllerDeleted(Controller controller);

    private:
        void OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon);
        void IsValidATCReceived(QString callsign);

    private:
        NetworkManager& m_networkManager;
        QList<Controller> m_controllers;
    };
}

#endif
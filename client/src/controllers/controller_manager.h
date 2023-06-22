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

#ifndef CONTROLLER_MANAGER_H
#define CONTROLLER_MANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>

#include "controller.h"
#include "network/networkmanager.h"
#include "simulator/xplane_adapter.h"
#include "qinjection/dependencypointer.h"
#include "common/frequency_utils.h"

namespace xpilot
{
    class ControllerManager : public QObject
    {
        Q_OBJECT

    public:
        ControllerManager(QObject* parent = nullptr);

    signals:
        void controllerAdded(Controller& controller);
        void controllerDeleted(Controller& controller);
        void controllerUpdated(Controller& controller);

    private:
        void OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon);
        void IsValidATCReceived(QString callsign);
        void OnRealNameReceived(QString callsign, QString realName);
        void RefreshController(Controller controller);
        void OnControllerDeleted(QString callsign);
        void OnRadioStackStateChanged(RadioStackState radioStack);
        void UpdateStationCallsigns();

    private:
        NetworkManager& m_networkManager;
        XplaneAdapter& m_xplaneAdapter;
        QList<Controller> m_controllers;
        QTimer m_nearbyAtcTimer;
        RadioStackState m_radioStackState;
    };
}

#endif

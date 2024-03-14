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

#ifndef AIRCRAFT_MANAGER_H
#define AIRCRAFT_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QList>

#include "network_aircraft.h"
#include "velocity_vector.h"
#include "simulator/xplane_adapter.h"
#include "network/networkmanager.h"
#include "qinjection/dependencypointer.h"

namespace xpilot
{
    class AircraftManager : public QObject
    {
        Q_OBJECT

    public:
        AircraftManager(QObject* parent = nullptr);

    private:
        NetworkManager& m_networkManager;
        XplaneAdapter& m_xplaneAdapter;

        static constexpr int StaleAircraftTimeout = 10000;
        static constexpr int SimulatorAircraftSyncInterval = 5000;

        QTimer m_staleAircraftCheckTimer;
        QTimer m_simulatorAircraftSyncTimer;

        QList<NetworkAircraft> m_aircraft;
        QList<QString> m_ignoredAircraft;

        void InitializeTimers();
        void OnNetworkConnected();
        void OnNetworkDisconnected();
        void OnStaleAircraftTimeoutTimeout();
        void OnSimulatorAircraftSyncTimeout();
        void OnCapabilitiessResponseReceived(QString callsign, QString data);
        void OnCapabilitiesRequestReceived(QString callsign);
        void OnSlowPositionUpdateReceived(QString callsign, AircraftVisualState visualState, double groundSpeed);
        void OnFastPositionUpdateReceived(QString callsign, AircraftVisualState visualState, VelocityVector positionalVelocityVector, VelocityVector rotationalVelocityVector);
        void OnPilotDeleted(QString callsign);
        void OnAircraftConfigurationReceived(QString callsign, QString json);
        void OnAircraftInfoReceived(QString callsign, QString typeCode, QString airlineIcao);
        void HandleAircraftConfiguration(NetworkAircraft& aircraft, const AircraftConfiguration& config);
        void DeleteAllPlanes();
        void DeletePlane(const NetworkAircraft& aircraft, QString reason);
        void SetUpNewAircraft(const QString &callsign, const AircraftVisualState& visualState);
        bool IsEligibleToAddToSimulator(const NetworkAircraft& aircraft);
        void SyncSimulatorAircraft();
        void OnIgnoreAircraft(QString callsign);
        void OnUnignoreAircraft(QString callsign);
        void OnAircraftAddedToSim(QString callsign);
        void OnAircraftRemovedFromSim(QString callsign);
    };
}

#endif // AIRCRAFT_MANAGER_H

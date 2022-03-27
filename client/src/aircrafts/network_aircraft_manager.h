#ifndef AIRCRAFT_MANAGER_H
#define AIRCRAFT_MANAGER_H

#include "network_aircraft.h"
#include "velocity_vector.h"
#include "src/simulator/xplane_adapter.h"
#include "src/network/networkmanager.h"

#include <QObject>
#include <QTimer>
#include <QList>

namespace xpilot
{
    class AircraftManager : public QObject
    {
        Q_OBJECT

    public:
        AircraftManager(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject* parent = nullptr);

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

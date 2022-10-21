#include "network_aircraft_manager.h"

namespace xpilot
{
    AircraftManager::AircraftManager(NetworkManager &networkManager, XplaneAdapter &xplaneAdapter, QObject *parent) :
        QObject(parent),
        m_networkManager(networkManager),
        m_xplaneAdapter(xplaneAdapter)
    {
        InitializeTimers();

        connect(&m_networkManager, &NetworkManager::networkConnected, this, &AircraftManager::OnNetworkConnected);
        connect(&m_networkManager, &NetworkManager::networkDisconnected, this, &AircraftManager::OnNetworkDisconnected);
        connect(&m_networkManager, &NetworkManager::capabilitiesResponseReceived, this, &AircraftManager::OnCapabilitiessResponseReceived);
        connect(&m_networkManager, &NetworkManager::capabilitiesRequestReceived, this, &AircraftManager::OnCapabilitiesRequestReceived);
        connect(&m_networkManager, &NetworkManager::slowPositionUpdateReceived, this, &AircraftManager::OnSlowPositionUpdateReceived);
        connect(&m_networkManager, &NetworkManager::fastPositionUpdateReceived, this, &AircraftManager::OnFastPositionUpdateReceived);
        connect(&m_networkManager, &NetworkManager::aircraftConfigurationInfoReceived, this, &AircraftManager::OnAircraftConfigurationReceived);
        connect(&m_networkManager, &NetworkManager::aircraftInfoReceived, this, &AircraftManager::OnAircraftInfoReceived);
        connect(&m_networkManager, &NetworkManager::pilotDeleted, this, &AircraftManager::OnPilotDeleted);
        connect(&m_xplaneAdapter, &XplaneAdapter::aircraftIgnored, this, &AircraftManager::OnIgnoreAircraft);
        connect(&m_xplaneAdapter, &XplaneAdapter::aircraftUnignored, this, &AircraftManager::OnUnignoreAircraft);
        connect(&m_xplaneAdapter, &XplaneAdapter::aircraftAddedToSim, this, &AircraftManager::OnAircraftAddedToSim);
        connect(&m_xplaneAdapter, &XplaneAdapter::aircraftRemovedFromSim, this, &AircraftManager::OnAircraftRemovedFromSim);
        connect(&m_staleAircraftCheckTimer, &QTimer::timeout, this, &AircraftManager::OnStaleAircraftTimeoutTimeout);
        connect(&m_simulatorAircraftSyncTimer, &QTimer::timeout, this, &AircraftManager::OnSimulatorAircraftSyncTimeout);
    }

    void AircraftManager::InitializeTimers()
    {
        m_staleAircraftCheckTimer.setInterval(StaleAircraftTimeout);
        m_simulatorAircraftSyncTimer.setInterval(SimulatorAircraftSyncInterval);
    }

    void AircraftManager::OnNetworkConnected()
    {
        m_staleAircraftCheckTimer.start();
    }

    void AircraftManager::OnNetworkDisconnected()
    {
        DeleteAllPlanes();
        m_staleAircraftCheckTimer.stop();
    }

    void AircraftManager::OnStaleAircraftTimeoutTimeout()
    {
        auto now = QDateTime::currentDateTimeUtc();

        QVector<NetworkAircraft> deleteThese;
        for(auto& aircraft : m_aircraft)
        {
            int timeSinceLastUpdate = aircraft.LastUpdated.msecsTo(now);
            if(timeSinceLastUpdate > 15000)
            {
                deleteThese.append(aircraft);
            }
        }

        for(auto & aircraft : deleteThese)
        {
            DeletePlane(aircraft, "Stale");
        }
    }

    void AircraftManager::OnSimulatorAircraftSyncTimeout()
    {
        SyncSimulatorAircraft();
    }

    void AircraftManager::OnCapabilitiessResponseReceived(QString callsign, QString data)
    {
        if(data.contains("ACCONFIG=1"))
        {
            m_networkManager.SendAircraftConfigurationRequest(callsign);
        }
    }

    void AircraftManager::OnCapabilitiesRequestReceived(QString callsign)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;
        });

        if(aircraft != m_aircraft.end())
        {
            m_networkManager.SendAircraftInfoRequest(callsign);
        }
    }

    void AircraftManager::OnSlowPositionUpdateReceived(QString callsign, AircraftVisualState visualState, double speed)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;
        });

        if(aircraft == m_aircraft.end())
        {
            SetUpNewAircraft(callsign, visualState);
        }
        else
        {
           aircraft->Speed = speed;
           aircraft->LastUpdated = QDateTime::currentDateTimeUtc();
           m_xplaneAdapter.SendHeartbeat(callsign);

           if((aircraft->Status == AircraftStatus::New) && IsEligibleToAddToSimulator(*aircraft))
           {
               SyncSimulatorAircraft();
           }
        }
    }

    void AircraftManager::OnFastPositionUpdateReceived(QString callsign, AircraftVisualState visualState,
                                                       VelocityVector positionalVelocityVector, VelocityVector rotationalVelocityVector)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;
        });

        if(aircraft != m_aircraft.end())
        {
            aircraft->HaveVelocities = true;
            aircraft->LastUpdated = QDateTime::currentDateTimeUtc();
            m_xplaneAdapter.SendFastPositionUpdate(*aircraft, visualState, positionalVelocityVector, rotationalVelocityVector);
        }
    }

    void AircraftManager::OnPilotDeleted(QString callsign)
    {
        for(auto& aircraft : m_aircraft)
        {
            if(aircraft.Callsign == callsign)
            {
                DeletePlane(aircraft, "Deleted");
            }
        }

        SyncSimulatorAircraft();
    }

    void AircraftManager::OnAircraftInfoReceived(QString callsign, QString typeCode, QString airlineIcao)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;
        });

        if(aircraft != m_aircraft.end())
        {
            aircraft->TypeCode = typeCode;
            aircraft->Airline = airlineIcao;
            SyncSimulatorAircraft();
        }
    }

    void AircraftManager::OnAircraftConfigurationReceived(QString callsign, QString json)
    {
        AircraftConfigurationInfo info = AircraftConfigurationInfo::FromJson(json);

        for(auto& plane : m_aircraft)
        {
            if(plane.Callsign == callsign && info.Config.has_value())
            {
                HandleAircraftConfiguration(plane, info.Config.value());
            }
        }
    }

    void AircraftManager::HandleAircraftConfiguration(NetworkAircraft &aircraft, const AircraftConfiguration &config)
    {
        bool isFullData = config.IsFullData.has_value() && config.IsFullData.value();

        // We can just ignore incremental config updates if we haven't received a full config yet.
        if(!isFullData && !aircraft.Configuration.has_value())
        {
            return;
        }

        if(isFullData)
        {
            aircraft.Configuration = config;
        }
        else
        {
            if(config.OnGround.has_value())
            {
                aircraft.Configuration->OnGround = config.OnGround;
            }
            if(config.GearDown.has_value())
            {
                aircraft.Configuration->GearDown = config.GearDown;
            }
            if(config.FlapsPercent.has_value())
            {
                aircraft.Configuration->FlapsPercent = config.FlapsPercent;
            }
            if(config.SpoilersDeployed.has_value())
            {
                aircraft.Configuration->SpoilersDeployed = config.SpoilersDeployed;
            }
            if(config.Lights->HasLights())
            {
                aircraft.Configuration->Lights = config.Lights;
            }
            if(config.Engines->HasEngines())
            {
                aircraft.Configuration->Engines = config.Engines;
            }
        }

        if((aircraft.Status == AircraftStatus::New) && IsEligibleToAddToSimulator(aircraft))
        {
            SyncSimulatorAircraft();
        }
        else
        {
            m_xplaneAdapter.PlaneConfigChanged(aircraft);
        }
    }

    void AircraftManager::DeleteAllPlanes()
    {
        m_xplaneAdapter.DeleteAllAircraft();
        m_aircraft.clear();
    }

    void AircraftManager::DeletePlane(const NetworkAircraft &aircraft, QString reason)
    {
        m_xplaneAdapter.DeleteAircraft(aircraft, reason);
    }

    void AircraftManager::SetUpNewAircraft(const QString &callsign, const AircraftVisualState &visualState)
    {
        NetworkAircraft aircraft{};
        aircraft.Callsign = callsign;
        aircraft.RemoteVisualState = visualState;
        aircraft.LastUpdated = QDateTime::currentDateTimeUtc();
        aircraft.Status = m_ignoredAircraft.contains(callsign) ? AircraftStatus::Ignored : AircraftStatus::New;
        m_aircraft.append(aircraft);

        m_networkManager.RequestCapabilities(callsign);
        m_networkManager.SendCapabilities(callsign);
        m_networkManager.SendAircraftInfoRequest(callsign);
    }

    bool AircraftManager::IsEligibleToAddToSimulator(const NetworkAircraft &aircraft)
    {
        if(!aircraft.Configuration.has_value())
        {
            return false;
        }

        if(aircraft.TypeCode.isEmpty())
        {
            return false;
        }

        if(aircraft.Status == AircraftStatus::Ignored)
        {
            return false;
        }

        if(aircraft.Speed > 0.5 && !aircraft.HaveVelocities)
        {
            return false;
        }

        return true;
    }

    void AircraftManager::SyncSimulatorAircraft()
    {
        auto now = QDateTime::currentDateTimeUtc();

        for(auto& aircraft : m_aircraft)
        {
            if((aircraft.Status == AircraftStatus::New) && IsEligibleToAddToSimulator(aircraft))
            {
                m_xplaneAdapter.AddAircraftToSimulator(aircraft);
                m_xplaneAdapter.PlaneConfigChanged(aircraft);
                aircraft.LastSyncTime = QDateTime::currentDateTimeUtc();
                aircraft.Status = AircraftStatus::Pending;
            }
            else if(aircraft.Status == AircraftStatus::Pending)
            {
                int timeSinceSync = aircraft.LastSyncTime.msecsTo(now);
                if(timeSinceSync > 10000)
                {
                    aircraft.Status = AircraftStatus::New; // try again in 10 seconds if no aircraft instance has been created
                }
            }
        }
    }

    void AircraftManager::OnIgnoreAircraft(QString callsign)
    {
        if(!m_ignoredAircraft.contains(callsign))
        {
            m_ignoredAircraft.push_back(callsign);
        }

        for(auto& aircraft : m_aircraft)
        {
            if(aircraft.Callsign == callsign)
            {
                DeletePlane(aircraft, "Ignore");
            }
        }
    }

    void AircraftManager::OnUnignoreAircraft(QString callsign)
    {
        if(m_ignoredAircraft.contains(callsign))
        {
            m_ignoredAircraft.removeAll(callsign);
        }
    }

    void AircraftManager::OnAircraftAddedToSim(QString callsign)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign;
        });

        if(aircraft != m_aircraft.end())
        {
            aircraft->Status = AircraftStatus::Active;
        }
    }

    void AircraftManager::OnAircraftRemovedFromSim(QString callsign)
    {
        auto aircraft = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){
            return a.Callsign == callsign;
        });

        if(aircraft != m_aircraft.end())
        {
            m_aircraft.removeAll(*aircraft);
        }
    }
}

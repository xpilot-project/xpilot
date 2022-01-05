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
        connect(m_staleAircraftCheckTimer, &QTimer::timeout, this, &AircraftManager::OnStaleAircraftTimeoutTimeout);
        connect(m_simulatorAircraftSyncTimer, &QTimer::timeout, this, &AircraftManager::OnSimulatorAircraftSyncTimeout);
    }

    void AircraftManager::InitializeTimers()
    {
        m_staleAircraftCheckTimer = new QTimer(this);
        m_staleAircraftCheckTimer->setInterval(StaleAircraftTimeout);

        m_simulatorAircraftSyncTimer = new QTimer(this);
        m_simulatorAircraftSyncTimer->setInterval(SimulatorAircraftSyncInterval);
    }

    void AircraftManager::OnNetworkConnected()
    {
        m_staleAircraftCheckTimer->start();
    }

    void AircraftManager::OnNetworkDisconnected()
    {
        DeleteAllPlanes();
        m_staleAircraftCheckTimer->stop();
    }

    void AircraftManager::OnStaleAircraftTimeoutTimeout()
    {
        auto now = QDateTime::currentDateTimeUtc();

        QVector<NetworkAircraft> deleteThese;
        for(auto& aircraft : m_aircraft)
        {
            if(now.secsTo(aircraft.LastSlowPositionUpdateReceived) > 10000)
            {
                deleteThese.append(aircraft);
            }
        }

        for(auto & aircraft : deleteThese)
        {
            DeletePlane(aircraft);
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
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;});

        if(planeIt != m_aircraft.end())
        {
            m_networkManager.SendAircraftInfoRequest(callsign);
        }
    }

    void AircraftManager::OnSlowPositionUpdateReceived(QString callsign, AircraftVisualState visualState, double groundSpeed)
    {
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;});

        if(planeIt != m_aircraft.end())
        {
            UpdateExistingAircraft(*planeIt, visualState, groundSpeed);
        }
        else
        {
            CreateNewAircraft(callsign, visualState);
        }
    }

    void AircraftManager::OnFastPositionUpdateReceived(QString callsign, AircraftVisualState visualState,
                                                       VelocityVector positionalVelocityVector, VelocityVector rotationalVelocityVector)
    {
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;});

        if(planeIt != m_aircraft.end())
        {
            planeIt->HaveVelocities = true;
            m_xplaneAdapter.SendFastPositionUpdate(*planeIt, visualState, positionalVelocityVector, rotationalVelocityVector);
        }
    }

    void AircraftManager::OnPilotDeleted(QString callsign)
    {
        for(auto& aircraft : m_aircraft)
        {
            if(aircraft.Callsign == callsign)
            {
                DeletePlane(aircraft);
            }
        }

        SyncSimulatorAircraft();
    }

    void AircraftManager::OnAircraftInfoReceived(QString callsign, QString typeCode, QString airlineIcao)
    {
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign && a.Status != AircraftStatus::Ignored;});

        if(planeIt != m_aircraft.end())
        {
            if(!planeIt->TypeCode.isEmpty() && planeIt->TypeCode != typeCode)
            {
                planeIt->TypeCode = typeCode;
                m_xplaneAdapter.PlaneModelChanged(*planeIt);
                return;
            }

            planeIt->TypeCode = typeCode;
            planeIt->Airline = airlineIcao;
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

    void AircraftManager::DeletePlane(const NetworkAircraft &aircraft)
    {
        m_xplaneAdapter.DeleteAircraft(aircraft);
        m_aircraft.removeAll(aircraft);
    }

    void AircraftManager::CreateNewAircraft(const QString &callsign, const AircraftVisualState &visualState)
    {
        NetworkAircraft aircraft{};
        aircraft.Callsign = callsign;
        aircraft.RemoteVisualState = visualState;
        aircraft.LastSlowPositionUpdateReceived = QDateTime::currentDateTimeUtc();
        aircraft.Status = m_ignoredAircraft.contains(callsign) ? AircraftStatus::Ignored : AircraftStatus::New;
        m_aircraft.append(aircraft);

        m_networkManager.RequestCapabilities(callsign);
        m_networkManager.SendCapabilities(callsign);
        m_networkManager.SendAircraftInfoRequest(callsign);
    }

    void AircraftManager::UpdateExistingAircraft(NetworkAircraft &aircraft, const AircraftVisualState &visualState, double groundSpeed)
    {
        aircraft.LastSlowPositionUpdateReceived = QDateTime::currentDateTimeUtc();
        m_xplaneAdapter.SendSlowPositionUpdate(aircraft, visualState, groundSpeed);

        if((aircraft.Status == AircraftStatus::New) && IsEligibleToAddToSimulator(aircraft))
        {
            SyncSimulatorAircraft();
        }
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

        return true;
    }

    void AircraftManager::SyncSimulatorAircraft()
    {
        for(auto& aircraft : m_aircraft)
        {
            if((aircraft.Status == AircraftStatus::New) && IsEligibleToAddToSimulator(aircraft))
            {
                m_xplaneAdapter.AddPlaneToSimulator(aircraft);
                m_xplaneAdapter.PlaneConfigChanged(aircraft);
                aircraft.Status = AircraftStatus::Active;
            }
        }
    }

    void AircraftManager::OnIgnoreAircraft(QString callsign)
    {
        if(!m_ignoredAircraft.contains(callsign)) {
            m_ignoredAircraft.push_back(callsign);
        }

        for(auto& aircraft : m_aircraft)
        {
            if(aircraft.Callsign == callsign)
            {
                DeletePlane(aircraft);
            }
        }
    }

    void AircraftManager::OnUnignoreAircraft(QString callsign)
    {
        if(m_ignoredAircraft.contains(callsign)) {
            m_ignoredAircraft.removeAll(callsign);
        }
    }
}

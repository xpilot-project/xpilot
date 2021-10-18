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
        connect(&m_networkManager, &NetworkManager::aircraftConfigurationInfoReceived, this, &AircraftManager::OnAircraftConfigurationReceived);
        connect(&m_networkManager, &NetworkManager::aircraftInfoReceived, this, &AircraftManager::OnAircraftInfoReceived);
        connect(&m_networkManager, &NetworkManager::pilotDeleted, this, &AircraftManager::OnPilotDeleted);
        connect(m_staleAircraftCheckTimer, &QTimer::timeout, this, &AircraftManager::OnStaleAircraftTimeoutTimeout);
        connect(m_simulatorAircraftSyncTimer, &QTimer::timeout, this, &AircraftManager::OnSimulatorAircraftSyncTimeout);

        auto cfg = AircraftConfigurationInfo::FromJson("{\"config\":{\"is_full_data\":true,\"lights\":{\"strobe_on\":true},\"on_ground\":false,\"flaps_pct\":50,\"engines\":{\"1\":{\"on\":false},\"2\":{\"on\":false}}}}");
        qDebug() << "OnGround: " << cfg.Config->OnGround.value_or(true);
        qDebug() << "Flaps: " << cfg.Config->FlapsPercent.value_or(0);
        qDebug() << "StrobeOn: " << cfg.Config->Lights->StrobeOn.value_or(true);
        qDebug() << "AnyEngineRunning: " << cfg.Config->IsAnyEngineRunning();
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
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign;});

        if(planeIt != m_aircraft.end())
        {
            m_networkManager.SendAircraftInfoRequest(callsign);
        }
    }

    void AircraftManager::OnSlowPositionUpdateReceived(QString callsign, AircraftVisualState visualState, double groundSpeed)
    {
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign;});

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
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign;});

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
        auto planeIt = std::find_if(m_aircraft.begin(), m_aircraft.end(), [=](NetworkAircraft a){return a.Callsign == callsign;});

        if(planeIt != m_aircraft.end())
        {
            if(!planeIt->TypeCode.isEmpty() && planeIt->TypeCode != typeCode)
            {
                planeIt->TypeCode = typeCode;
                return;
            }

            planeIt->TypeCode = typeCode;
            planeIt->Airline = airlineIcao;
            SyncSimulatorAircraft();
        }
    }

    void AircraftManager::OnAircraftConfigurationReceived(QString callsign, QString json)
    {
        //        AircraftConfigurationInfo info = AircraftConfigurationInfo::FromJson(json);

        //        for(auto& plane : m_aircraft)
        //        {
        //            if(plane.Callsign == callsign && info.Config.has_value())
        //            {
        //                HandleAircraftConfiguration(plane, info.Config.value());
        //            }
        //        }
    }

    //    void AircraftManager::HandleAircraftConfiguration(NetworkAircraft &aircraft, const AircraftConfiguration &config)
    //    {
    //        bool isFullData = config.IsFullData.has_value() && config.IsFullData.value();

    //        // We can just ignore incremental config updates if we haven't received a full config yet.
    //        if(!isFullData && !aircraft.Configuration.has_value())
    //        {
    //            return;
    //        }

    //        if(isFullData)
    //        {
    //            aircraft.Configuration = config;
    //        }
    //        else
    //        {
    //            AircraftConfiguration::ApplyIncremental(aircraft.Configuration.value(), config);
    //        }

    //        if((aircraft.Status == AircraftStatus::New) && IsEligibleToAddToSimulator(aircraft))
    //        {
    //            SyncSimulatorAircraft();
    //        }
    //        else
    //        {
    //            m_xplaneAdapter.PlaneConfigChanged(aircraft);
    //        }
    //    }

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
        aircraft.Status = AircraftStatus::New;
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
        //        if(!aircraft.Configuration.has_value())
        //        {
        //            return false;
        //        }

        if(aircraft.TypeCode.isEmpty())
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
                aircraft.Status = AircraftStatus::Pending;
            }
        }
    }
}

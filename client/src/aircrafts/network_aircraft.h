#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "aircraft_visual_state.h"
#include "aircraft_configuration.h"

#include <QString>
#include <QDateTime>
#include <optional>

enum class AircraftStatus
{
    New,
    Active,
    Ignored,
    Pending
};

struct NetworkAircraft
{
    QString Callsign;
    QString Airline;
    QString TypeCode;
    AircraftVisualState RemoteVisualState;
    std::optional<AircraftConfiguration> Configuration;
    QDateTime LastUpdated;
    QDateTime LastSyncTime;
    AircraftStatus Status;
    bool HaveVelocities;
    double Speed;

    bool operator==(const NetworkAircraft& rhs) const
    {
         return Callsign == rhs.Callsign
                 && Airline == rhs.Airline
                 && TypeCode == rhs.TypeCode
                 && RemoteVisualState == rhs.RemoteVisualState
                 && LastUpdated == rhs.LastUpdated
                 && Status == rhs.Status
                 && HaveVelocities == rhs.HaveVelocities
                 && Speed == rhs.Speed;
    }
};

#endif

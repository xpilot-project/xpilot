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
//    std::optional<AircraftConfiguration> Configuration;
    QDateTime LastSlowPositionUpdateReceived;
    AircraftStatus Status;
    bool HaveVelocities;

    bool operator==(const NetworkAircraft& rhs) const
    {
         return Callsign == rhs.Callsign
                 && Airline == rhs.Airline
                 && TypeCode == rhs.TypeCode
                 && RemoteVisualState == rhs.RemoteVisualState
                 && LastSlowPositionUpdateReceived == rhs.LastSlowPositionUpdateReceived
                 && Status == rhs.Status
                 && HaveVelocities == rhs.HaveVelocities;
    }
};

#endif

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

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <optional>

#include <QString>
#include <QDateTime>

#include "aircraft_visual_state.h"
#include "aircraft_configuration.h"

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

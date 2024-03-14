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

#ifndef AIRCRAFT_VISUAL_STATE_H
#define AIRCRAFT_VISUAL_STATE_H

struct AircraftVisualState
{
    double Latitude;
    double Longitude;
    double Altitude;
    double AltitudeAgl;
    double Pitch;
    double Heading;
    double Bank;
    double NoseWheelAngle;

    bool operator==(const AircraftVisualState& rhs) const
    {
         return Latitude == rhs.Latitude && Longitude == rhs.Longitude && Altitude == rhs.Altitude && AltitudeAgl == rhs.AltitudeAgl
                 && Pitch == rhs.Pitch && Heading == rhs.Heading && Bank == rhs.Bank && NoseWheelAngle == rhs.NoseWheelAngle;
    }

    bool operator!=(const AircraftVisualState& rhs) const
    {
        return Latitude != rhs.Latitude || Longitude != rhs.Longitude || Altitude != rhs.Altitude || AltitudeAgl != rhs.AltitudeAgl
                || Pitch != rhs.Pitch || Heading != rhs.Heading || Bank != rhs.Bank || NoseWheelAngle != rhs.NoseWheelAngle;
    }
};

#endif

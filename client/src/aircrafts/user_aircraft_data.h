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

#pragma once

#include <QObject>

struct UserAircraftData
{
    Q_GADGET

public:
    double Latitude;
    double Longitude;
    double AltitudeMslM;
    double AltitudeAglM;
    double AltitudePressure;
    double BarometerSeaLevel;
    double AltimeterTemperatureError;
    double GroundSpeed;
    double Pitch;
    double Heading;
    double Bank;
    double LatitudeVelocity;
    double AltitudeVelocity;
    double LongitudeVelocity;
    double PitchVelocity;
    double HeadingVelocity;
    double BankVelocity;
    double NoseWheelAngle;

    bool operator!=(const UserAircraftData& other) const
    {
        return Latitude != other.Latitude ||
                Longitude != other.Longitude ||
                AltitudeMslM != other.AltitudeMslM ||
                AltitudeAglM != other.AltitudeAglM ||
                AltitudePressure != other.AltitudePressure ||
                BarometerSeaLevel != other.BarometerSeaLevel ||
                AltimeterTemperatureError != other.AltimeterTemperatureError ||
                GroundSpeed != other.GroundSpeed ||
                Pitch != other.Pitch ||
                Heading != other.Heading ||
                Bank != other.Bank ||
                LatitudeVelocity != other.LatitudeVelocity ||
                AltitudeVelocity != other.AltitudeVelocity ||
                LongitudeVelocity != other.LongitudeVelocity ||
                PitchVelocity != other.PitchVelocity ||
                HeadingVelocity != other.HeadingVelocity ||
                BankVelocity != other.BankVelocity ||
                NoseWheelAngle != other.NoseWheelAngle;
    }

    bool operator==(const UserAircraftData& other) const
    {
        return Latitude == other.Latitude &&
                Longitude == other.Longitude &&
                AltitudeMslM == other.AltitudeMslM &&
                AltitudeAglM == other.AltitudeAglM &&
                AltitudePressure == other.AltitudePressure &&
                BarometerSeaLevel == other.BarometerSeaLevel &&
                AltimeterTemperatureError == other.AltimeterTemperatureError &&
                GroundSpeed == other.GroundSpeed &&
                Pitch == other.Pitch &&
                Heading == other.Heading &&
                Bank == other.Bank &&
                LatitudeVelocity == other.LatitudeVelocity &&
                AltitudeVelocity == other.AltitudeVelocity &&
                LongitudeVelocity == other.LongitudeVelocity &&
                PitchVelocity == other.PitchVelocity &&
                HeadingVelocity == other.HeadingVelocity &&
                BankVelocity == other.BankVelocity &&
                NoseWheelAngle == other.NoseWheelAngle;
    }
};

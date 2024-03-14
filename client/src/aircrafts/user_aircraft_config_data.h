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

#pragma once

#include <QObject>

struct UserAircraftConfigData
{
    Q_GADGET

public:
    bool StrobesOn;
    bool LandingLightsOn;
    bool TaxiLightsOn;
    bool BeaconOn;
    bool NavLightsOn;
    int EngineCount;
    bool Engine1Running;
    bool Engine2Running;
    bool Engine3Running;
    bool Engine4Running;
    bool Engine1Reversing;
    bool Engine2Reversing;
    bool Engine3Reversing;
    bool Engine4Reversing;
    bool GearDown;
    double FlapsRatio;
    double SpeedbrakeRatio;
    bool OnGround;

    bool operator!=(const UserAircraftConfigData& other) const
    {
        return StrobesOn != other.StrobesOn ||
                LandingLightsOn != other.LandingLightsOn ||
                TaxiLightsOn != other.TaxiLightsOn ||
                BeaconOn != other.BeaconOn ||
                NavLightsOn != other.NavLightsOn ||
                EngineCount != other.EngineCount ||
                Engine1Running != other.Engine1Running ||
                Engine2Running != other.Engine2Running ||
                Engine3Running != other.Engine3Running ||
                Engine4Running != other.Engine4Running ||
                Engine1Reversing != other.Engine1Reversing ||
                Engine2Reversing != other.Engine2Reversing ||
                Engine3Reversing != other.Engine3Reversing ||
                Engine4Reversing != other.Engine4Reversing ||
                GearDown != other.GearDown ||
                FlapsRatio != other.FlapsRatio ||
                SpeedbrakeRatio != other.SpeedbrakeRatio ||
                OnGround != other.OnGround;
    }

    bool operator==(const UserAircraftConfigData& other) const
    {
        return StrobesOn == other.StrobesOn &&
                LandingLightsOn == other.LandingLightsOn &&
                TaxiLightsOn == other.TaxiLightsOn &&
                BeaconOn == other.BeaconOn &&
                NavLightsOn == other.NavLightsOn &&
                EngineCount == other.EngineCount &&
                Engine1Running == other.Engine1Running &&
                Engine2Running == other.Engine2Running &&
                Engine3Running == other.Engine3Running &&
                Engine4Running == other.Engine4Running &&
                Engine1Reversing == other.Engine1Reversing &&
                Engine2Reversing == other.Engine2Reversing &&
                Engine3Reversing == other.Engine3Reversing &&
                Engine4Reversing == other.Engine4Reversing &&
                GearDown == other.GearDown &&
                FlapsRatio == other.FlapsRatio &&
                SpeedbrakeRatio == other.SpeedbrakeRatio &&
                OnGround == other.OnGround;
    }
};

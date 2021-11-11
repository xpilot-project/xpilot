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
                GearDown == other.GearDown &&
                FlapsRatio == other.FlapsRatio &&
                SpeedbrakeRatio == other.SpeedbrakeRatio &&
                OnGround == other.OnGround;
    }
};

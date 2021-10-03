#ifndef USER_AIRCRAFT_CONFIG_DATA_H
#define USER_AIRCRAFT_CONFIG_DATA_H

struct UserAircraftConfigData
{
    bool StrobesOn;
    bool LandingLightsOn;
    bool TaxiLightsOn;
    bool BeaconOn;
    bool NavLightOn;
    int EngineCount;
    bool Engine1Running;
    bool Engine2Running;
    bool Engine3Running;
    bool Engine4Running;
    bool GearDown;
    double FlapsRatio;
    double SpoilersRatio;
    bool OnGround;

    bool operator==(const UserAircraftConfigData& b) const
    {
        return StrobesOn == b.StrobesOn &&
                LandingLightsOn == b.LandingLightsOn &&
                TaxiLightsOn == b.TaxiLightsOn &&
                BeaconOn == b.BeaconOn &&
                NavLightOn == b.NavLightOn &&
                EngineCount == b.EngineCount &&
                Engine1Running == b.Engine1Running &&
                Engine2Running == b.Engine2Running &&
                Engine3Running == b.Engine3Running &&
                Engine4Running == b.Engine4Running &&
                GearDown == b.GearDown &&
                FlapsRatio == b.FlapsRatio &&
                SpoilersRatio == b.SpoilersRatio &&
                OnGround == b.OnGround;
    }
};

#endif

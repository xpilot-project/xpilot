#ifndef USER_AIRCRAFT_DATA_H
#define USER_AIRCRAFT_DATA_H

#include <QObject>

struct UserAircraftData
{
    Q_GADGET

public:
    double Latitude;
    double Longitude;
    double Heading;
    double Bank;
    double Pitch;
    double AltitudeMslM;
    double AltitudeAglM;
    double GroundSpeed;
    bool OnGround;
    float FlapsPercent;
    bool SpeedBrakesDeployed;
    bool GearDown;
    bool StrobeLightsOn;
    bool BeaconLightsOn;
    bool TaxiLightsOn;
    bool LandingLightsOn;
    bool NavLightsOn;
    int EngineCount;
    bool Engine1Running;
    bool Engine2Running;
    bool Engine3Running;
    bool Engine4Running;
    bool ReplayModeEnabled;

    bool operator!=(const UserAircraftData& other) const
    {
        return Latitude != other.Latitude ||
            Longitude != other.Longitude ||
            Heading != other.Heading ||
            Bank != other.Bank ||
            Pitch != other.Pitch ||
            AltitudeMslM != other.AltitudeMslM ||
            AltitudeAglM != other.AltitudeAglM ||
            GroundSpeed != other.GroundSpeed ||
            OnGround != other.OnGround ||
            FlapsPercent != other.FlapsPercent ||
            SpeedBrakesDeployed != other.SpeedBrakesDeployed ||
            GearDown != other.GearDown ||
            StrobeLightsOn != other.StrobeLightsOn ||
            BeaconLightsOn != other.BeaconLightsOn ||
            TaxiLightsOn != other.TaxiLightsOn ||
            LandingLightsOn != other.LandingLightsOn ||
            NavLightsOn != other.NavLightsOn ||
            EngineCount != other.EngineCount ||
            Engine1Running != other.Engine1Running ||
            Engine2Running != other.Engine2Running ||
            Engine3Running != other.Engine3Running ||
            Engine4Running != other.Engine4Running ||
            ReplayModeEnabled != other.ReplayModeEnabled;
    }

    bool operator==(const UserAircraftData& other) const
    {
        return Latitude == other.Latitude &&
            Longitude == other.Longitude &&
            Heading == other.Heading &&
            Bank == other.Bank &&
            Pitch == other.Pitch &&
            AltitudeMslM == other.AltitudeMslM &&
            AltitudeAglM == other.AltitudeAglM &&
            GroundSpeed == other.GroundSpeed &&
            OnGround == other.OnGround &&
            FlapsPercent == other.FlapsPercent &&
            SpeedBrakesDeployed == other.SpeedBrakesDeployed &&
            GearDown == other.GearDown &&
            StrobeLightsOn == other.StrobeLightsOn &&
            BeaconLightsOn == other.BeaconLightsOn &&
            TaxiLightsOn == other.TaxiLightsOn &&
            LandingLightsOn == other.LandingLightsOn &&
            NavLightsOn == other.NavLightsOn &&
            EngineCount == other.EngineCount &&
            Engine1Running == other.Engine1Running &&
            Engine2Running == other.Engine2Running &&
            Engine3Running == other.Engine3Running &&
            Engine4Running == other.Engine4Running &&
            ReplayModeEnabled == other.ReplayModeEnabled;
    }
};

#endif

#ifndef AIRCRAFT_CONFIGURATION_H
#define AIRCRAFT_CONFIGURATION_H

#include <QString>

class AircraftConfigurationLights
{
public:
    bool StrobesOn = false;
    bool LandingOn = false;
    bool TaxiOn = false;
    bool BeaconOn = false;
    bool NavOn = false;

    bool operator==(const AircraftConfigurationLights& b) const
    {
        return StrobesOn == b.StrobesOn && LandingOn == b.LandingOn && TaxiOn == b.TaxiOn && BeaconOn == b.BeaconOn && NavOn == b.NavOn;
    }
};

class AircraftConfigurationEngines
{
public:
    bool Engine1Running;
    bool Engine2Running;
    bool Engine3Running;
    bool Engine4Running;
};

class AircraftConfiguration
{
public:
    bool IsFullData;
    AircraftConfigurationLights Lights;
    AircraftConfigurationEngines Engines;
    bool GearDown;
    bool SpoilersDeployed;
    bool OnGround;
    int FlapsPercent;

    AircraftConfiguration()
    {
        IsFullData = false;
        Lights = {};
        Engines = {};
        GearDown = false;
        SpoilersDeployed = false;
        OnGround = false;
        FlapsPercent = 0;
    }

    QString toJson();
};

class AircraftConfigurationInfo
{
public:
    std::string Request;
    AircraftConfiguration Config;
};

#endif

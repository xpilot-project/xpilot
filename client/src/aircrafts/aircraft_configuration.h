#ifndef AIRCRAFT_CONFIGURATION_H
#define AIRCRAFT_CONFIGURATION_H

#include <QJsonObject>
#include <QJsonDocument>
#include <optional>

#include "user_aircraft_config_data.h"

class AircraftConfigurationEngines
{
public:
    std::optional<bool> Engine1Running;
    std::optional<bool> Engine2Running;
    std::optional<bool> Engine3Running;
    std::optional<bool> Engine4Running;

    std::optional<bool> Engine1Reversing;
    std::optional<bool> Engine2Reversing;
    std::optional<bool> Engine3Reversing;
    std::optional<bool> Engine4Reversing;

    bool HasEngines() const {
        return Engine1Running.has_value() || Engine2Running.has_value() || Engine3Running.has_value() || Engine4Running.has_value() ||
                Engine1Reversing.has_value() || Engine2Reversing.has_value() || Engine3Reversing.has_value() || Engine4Reversing.has_value();
    }

    static AircraftConfigurationEngines FromUserAircraftData(UserAircraftConfigData config);

    AircraftConfigurationEngines CreateIncremental(AircraftConfigurationEngines config);

    bool operator!=(const AircraftConfigurationEngines& rhs) {
        return Engine1Running != rhs.Engine1Running || Engine2Running != rhs.Engine2Running ||
                Engine3Running != rhs.Engine3Running || Engine4Running != rhs.Engine4Running ||
                Engine1Reversing != rhs.Engine1Reversing || Engine2Reversing != rhs.Engine2Reversing ||
                Engine3Reversing != rhs.Engine3Reversing || Engine4Reversing != rhs.Engine4Reversing;
    }

    bool operator==(const AircraftConfigurationEngines& rhs)
    {
        return Engine1Running == rhs.Engine1Running &&
                Engine1Reversing == rhs.Engine1Reversing &&
                Engine2Running == rhs.Engine2Running &&
                Engine2Reversing == rhs.Engine2Reversing &&
                Engine3Running == rhs.Engine3Running &&
                Engine3Reversing == rhs.Engine3Reversing &&
                Engine4Running == rhs.Engine4Running &&
                Engine4Reversing == rhs.Engine4Reversing;
    }
};

class AircraftConfigurationLights
{
public:
    std::optional<bool> StrobeOn;
    std::optional<bool> LandingOn;
    std::optional<bool> TaxiOn;
    std::optional<bool> BeaconOn;
    std::optional<bool> NavOn;
    bool HasLights() const { return StrobeOn.has_value() || LandingOn.has_value() || TaxiOn.has_value() || BeaconOn.has_value() || NavOn.has_value(); }

    static AircraftConfigurationLights FromUserAircraftData(UserAircraftConfigData config);

    AircraftConfigurationLights CreateIncremental(AircraftConfigurationLights config);

    bool operator!=(const AircraftConfigurationLights& rhs)
    {
        return StrobeOn != rhs.StrobeOn || LandingOn != rhs.LandingOn || TaxiOn != rhs.TaxiOn || BeaconOn != rhs.BeaconOn || NavOn != rhs.NavOn;
    }

    bool operator==(const AircraftConfigurationLights& rhs)
    {
        return StrobeOn == rhs.StrobeOn && LandingOn == rhs.LandingOn && TaxiOn == rhs.TaxiOn && BeaconOn == rhs.BeaconOn && NavOn == rhs.NavOn;
    }
};

class AircraftConfiguration
{
public:
    std::optional<bool> IsFullData;
    bool IsIncremental() const { return !IsFullData.has_value() || !IsFullData.value(); }

    std::optional<AircraftConfigurationLights> Lights;
    std::optional<AircraftConfigurationEngines> Engines;
    std::optional<bool> GearDown;
    std::optional<int> FlapsPercent;
    std::optional<bool> SpoilersDeployed;
    std::optional<bool> OnGround;

    bool HasEnginesRunning() const { return Engines.has_value() && (Engines->Engine1Running.has_value() ||
                                                                Engines->Engine2Running.has_value() ||
                                                                Engines->Engine3Running.has_value() ||
                                                                Engines->Engine4Running.has_value()); }

    bool IsAnyEngineRunning() const { return Engines.has_value() && (Engines->Engine1Running.value_or(false) ||
                                                                     Engines->Engine2Running.value_or(false) ||
                                                                     Engines->Engine3Running.value_or(false) ||
                                                                     Engines->Engine4Running.value_or(false)); }

    bool IsAnyEngineReversing() const { return Engines.has_value() && (Engines->Engine1Reversing.value_or(false) ||
                                                                       Engines->Engine2Reversing.value_or(false) ||
                                                                       Engines->Engine3Reversing.value_or(false) ||
                                                                       Engines->Engine4Reversing.value_or(false)); }

    bool HasEnginesReversing() const { return Engines.has_value() && (Engines->Engine1Reversing.has_value() ||
                                                                      Engines->Engine2Reversing.has_value() ||
                                                                      Engines->Engine3Reversing.has_value() ||
                                                                      Engines->Engine4Reversing.has_value()); }

    AircraftConfiguration()
    {
        Lights = AircraftConfigurationLights();
        Engines = AircraftConfigurationEngines();
    }

    QJsonObject ToJson() const;

    static AircraftConfiguration FromUserAircraftData(UserAircraftConfigData config);

    AircraftConfiguration CreateIncremental(AircraftConfiguration config);

    bool operator!=(const AircraftConfiguration& rhs)
    {
        return GearDown != rhs.GearDown ||
                FlapsPercent != rhs.FlapsPercent ||
                SpoilersDeployed != rhs.SpoilersDeployed ||
                OnGround != rhs.OnGround ||
                Lights.value() != rhs.Lights.value() ||
                Engines.value() != rhs.Engines.value();
    }

    bool operator==(const AircraftConfiguration& rhs)
    {
        return GearDown == rhs.GearDown &&
                FlapsPercent == rhs.FlapsPercent &&
                SpoilersDeployed == rhs.SpoilersDeployed &&
                OnGround == rhs.OnGround &&
                Lights.value() == rhs.Lights.value() &&
                Engines.value() == rhs.Engines.value();
    }
};

class AircraftConfigurationInfo
{
public:
    std::optional<bool> FullRequest;
    std::optional<AircraftConfiguration> Config;
    bool HasConfig() const { return Config.has_value(); }

    AircraftConfigurationInfo()
    {
        Config = AircraftConfiguration();
    }

    QString ToJson() const;

    static AircraftConfigurationInfo FromJson(const QString& json);
};

#endif

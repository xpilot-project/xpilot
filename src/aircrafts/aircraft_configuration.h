#ifndef AIRCRAFT_CONFIGURATION_H
#define AIRCRAFT_CONFIGURATION_H

#include <QJsonObject>
#include <QJsonDocument>
#include <optional>

class AircraftConfigurationEngines
{
public:
    std::optional<bool> Engine1Running;
    std::optional<bool> Engine2Running;
    std::optional<bool> Engine3Running;
    std::optional<bool> Engine4Running;

    bool HasEngines() const { return Engine1Running.has_value() || Engine2Running.has_value() || Engine3Running.has_value() || Engine4Running.has_value(); }
};

class AircraftConfigurationLights
{
public:
    std::optional<bool> StrobeOn;
    std::optional<bool> LandingOn;
    std::optional<bool> TaxiOn;
    std::optional<bool> BeaconOn;
    std::optional<bool> NavOn;
    std::optional<bool> LogoOn;

    bool HasLights() const { return StrobeOn.has_value() || LandingOn.has_value() || TaxiOn.has_value() || BeaconOn.has_value() || NavOn.has_value() || LogoOn.has_value(); }
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

    bool IsAnyEngineRunning() const { return Engines.has_value() && (Engines->Engine1Running.value_or(false) || Engines->Engine2Running.value_or(false) ||
                 Engines->Engine3Running.value_or(false) || Engines->Engine4Running.value_or(false)); }

    AircraftConfiguration()
    {
        Lights = AircraftConfigurationLights();
        Engines = AircraftConfigurationEngines();
    }

    QString ToJson() const;
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

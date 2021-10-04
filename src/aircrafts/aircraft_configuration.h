#ifndef AIRCRAFT_CONFIGURATION_H
#define AIRCRAFT_CONFIGURATION_H

#include <optional>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include "user_aircraft_data.h"

class AircraftConfigurationLights
{
public:
    std::optional<bool> StrobesOn;
    std::optional<bool> LandingOn;
    std::optional<bool> TaxiOn;
    std::optional<bool> BeaconOn;
    std::optional<bool> NavOn;

    QJsonObject toJson() const;
    AircraftConfigurationLights Clone();
    void ApplyIncremental(AircraftConfigurationLights& inc);
    AircraftConfigurationLights CreateIncremental(AircraftConfigurationLights& cfg);
    static AircraftConfigurationLights FromUserAircraftData(UserAircraftData& uac);

    bool operator!=(const AircraftConfigurationLights& b) const
    {
        return StrobesOn != b.StrobesOn || LandingOn != b.LandingOn || b.TaxiOn != TaxiOn || b.BeaconOn != BeaconOn || b.NavOn != NavOn;
    }

    bool operator==(const AircraftConfigurationLights& b) const
    {
        return StrobesOn == b.StrobesOn && LandingOn == b.LandingOn && TaxiOn == b.TaxiOn && BeaconOn == b.BeaconOn && NavOn == b.NavOn;
    }
};

class AircraftConfigurationEngine
{
public:
    std::optional<bool> Running;

    QJsonObject toJson() const;
    AircraftConfigurationEngine Clone();
    void ApplyIncremental(AircraftConfigurationEngine& inc);
    AircraftConfigurationEngine CreateIncremental(AircraftConfigurationEngine& cfg);
    static AircraftConfigurationEngine FromUserAircraftData(UserAircraftData& uac, int engineNum);

    bool operator!=(const AircraftConfigurationEngine& b) const
    {
        return Running != b.Running;
    }

    bool operator==(const AircraftConfigurationEngine& b) const
    {
        return Running == b.Running;
    }
};

class AircraftConfigurationEngines
{
public:
    std::optional<AircraftConfigurationEngine> Engine1;
    std::optional<AircraftConfigurationEngine> Engine2;
    std::optional<AircraftConfigurationEngine> Engine3;
    std::optional<AircraftConfigurationEngine> Engine4;

    bool HasEngine1Object() const { return Engine1.has_value(); }
    bool HasEngine2Object() const { return Engine2.has_value(); }
    bool HasEngine3Object() const { return Engine3.has_value(); }
    bool HasEngine4Object() const { return Engine4.has_value(); }
    bool IsAnyEngineRunning() const { return HasEngine1Object() && Engine1->Running.value()
                || HasEngine2Object() && Engine2->Running.value()
                || HasEngine3Object() && Engine3->Running.value()
                || HasEngine4Object() && Engine4->Running.value(); }

    QJsonObject toJson() const;
    AircraftConfigurationEngines Clone();
    void ApplyIncremental(AircraftConfigurationEngines& inc);
    AircraftConfigurationEngines CreateIncremental(AircraftConfigurationEngines& cfg);
    static AircraftConfigurationEngines FromUserAircraftData(UserAircraftData& uac);

    bool operator!=(const AircraftConfigurationEngines& b) const
    {
        return b.Engine1 != Engine1 || b.Engine2 != Engine2 || b.Engine3 != Engine3 || b.Engine4 != Engine4;
    }

    bool operator==(const AircraftConfigurationEngines& b) const
    {
        return b.Engine1 == Engine1 && b.Engine2 == Engine2 && b.Engine3 == Engine3 && b.Engine4 == Engine4;
    }
};

class AircraftConfiguration
{
public:
    std::optional<bool> IsFullData;
    std::optional<AircraftConfigurationLights> Lights;
    std::optional<AircraftConfigurationEngines> Engines;
    std::optional<bool> GearDown;
    std::optional<bool> SpoilersDeployed;
    std::optional<bool> OnGround;
    std::optional<int> FlapsPercent;

    QJsonObject ToJson() const;
    AircraftConfiguration Clone();
    void ApplyIncremental(AircraftConfiguration& inc);
    AircraftConfiguration CreateIncremental(AircraftConfiguration& cfg);
    static AircraftConfiguration FromUserAircraftData(UserAircraftData& uac);

    bool operator!=(const AircraftConfiguration& b) const
    {
        return b.GearDown != GearDown || b.FlapsPercent != FlapsPercent || b.SpoilersDeployed != SpoilersDeployed || b.OnGround != OnGround || b.Lights != Lights;
    }

    bool operator==(const AircraftConfiguration& b) const
    {
        return b.GearDown == GearDown && b.FlapsPercent == FlapsPercent && b.SpoilersDeployed == SpoilersDeployed && b.OnGround == OnGround;
    }

private:
    static int RoundUpToNearest5(double val);
};

class AircraftConfigurationInfo
{
public:
    bool IsFullRequest;
    std::optional<AircraftConfiguration> Config;
    bool HasConfig() const;
    QString ToIncrementalJson() const;
    QString ToFullJson() const;
    QString RequestFullConfig() const;
    static AircraftConfigurationInfo FromJson(QString json);
};

#endif

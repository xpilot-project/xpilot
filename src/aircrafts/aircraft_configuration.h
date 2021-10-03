#ifndef AIRCRAFT_CONFIGURATION_H
#define AIRCRAFT_CONFIGURATION_H

#include <optional>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include "user_aircraft_config_data.h"

class AircraftConfigurationLights
{
public:
    std::optional<bool> StrobesOn;
    std::optional<bool> LandingOn;
    std::optional<bool> TaxiOn;
    std::optional<bool> BeaconOn;
    std::optional<bool> NavOn;

    QJsonObject toJson() const;
    void EnsurePopuplated();
    AircraftConfigurationLights Clone();
    void ApplyIncremental(AircraftConfigurationLights& inc);
    AircraftConfigurationLights CreateIncremental(AircraftConfigurationLights& cfg);
    static AircraftConfigurationLights FromUserAircraftData(UserAircraftConfigData& uac);

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
    void EnsurePopuplated();
    AircraftConfigurationEngine Clone();
    void ApplyIncremental(AircraftConfigurationEngine& inc);
    AircraftConfigurationEngine CreateIncremental(AircraftConfigurationEngine& cfg);
    static AircraftConfigurationEngine FromUserAircraftData(UserAircraftConfigData& uac, int engineNum);

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
    bool IsAnyEngineRunning() const { return
                HasEngine1Object() && Engine1->Running.value() ||
                HasEngine2Object() && Engine2->Running.value() ||
                HasEngine3Object() && Engine3->Running.value() ||
                HasEngine4Object() && Engine4->Running.value(); }

    QJsonObject toJson() const;
    void EnsurePopuplated();
    AircraftConfigurationEngines Clone();
    void ApplyIncremental(AircraftConfigurationEngines& inc);
    AircraftConfigurationEngines CreateIncremental(AircraftConfigurationEngines& cfg);
    static AircraftConfigurationEngines FromUserAircraftData(UserAircraftConfigData& uac);

    bool operator==(const AircraftConfigurationEngines& b) const
    {
        if(!b.HasEngine1Object() && HasEngine1Object()) return false;
        if(b.HasEngine1Object() && !HasEngine1Object()) return false;
        if(!b.HasEngine2Object() && HasEngine2Object()) return false;
        if(b.HasEngine2Object() && !HasEngine2Object()) return false;
        if(!b.HasEngine3Object() && HasEngine3Object()) return false;
        if(b.HasEngine3Object() && !HasEngine3Object()) return false;
        if(!b.HasEngine4Object() && HasEngine4Object()) return false;
        if(b.HasEngine4Object() && !HasEngine4Object()) return false;
        return (
                    (!b.HasEngine1Object() || b.Engine1->operator==(Engine1.value())) &&
                    (!b.HasEngine2Object() || b.Engine2->operator==(Engine2.value())) &&
                    (!b.HasEngine3Object() || b.Engine3->operator==(Engine3.value())) &&
                    (!b.HasEngine4Object() || b.Engine4->operator==(Engine4.value()))
                    );
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

    AircraftConfiguration()
    {
        EnsurePopulated();
    }

    QJsonObject ToJson() const;
    void EnsurePopulated();
    AircraftConfiguration Clone();
    void ApplyIncremental(AircraftConfiguration& inc);
    AircraftConfiguration CreateIncremental(AircraftConfiguration& cfg);
    AircraftConfiguration FromUserAircraftData(UserAircraftConfigData& uac);

    bool operator==(const AircraftConfiguration& b) const
    {
        if((!b.Lights.has_value()) && (Lights.has_value())) return false;
        if((b.Lights.has_value()) && (!Lights.has_value())) return false;
        if((!b.Engines.has_value()) && (Engines.has_value())) return false;
        if((b.Engines.has_value()) && (!Engines.has_value())) return false;
        return (
            ((!b.Lights.has_value() || b.Lights->operator==(Lights.value()))) &&
            ((!b.Engines.has_value() || b.Engines->operator==(Engines.value()))) &&
            b.GearDown == GearDown &&
            b.FlapsPercent == FlapsPercent &&
            b.SpoilersDeployed == SpoilersDeployed &&
            b.OnGround == OnGround
        );
    }

private:
    int RoundUpToNearest5(double val);
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

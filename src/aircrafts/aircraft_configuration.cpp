#include "aircraft_configuration.h"

bool AircraftConfigurationInfo::HasConfig() const
{
    return Config.has_value();
}

QString AircraftConfigurationInfo::ToIncrementalJson() const
{
    QJsonObject obj;
    if(this->Config.has_value() && !this->Config.value().IsFullData) {
        obj["config"] = this->Config.value().ToJson();
    }
    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

QString AircraftConfigurationInfo::ToFullJson() const
{
    QJsonObject root;

    QJsonObject config;
    if(this->Config.has_value())
    {
        config["is_full_data"] = true;
        if(this->Config->Lights.has_value()) {
            config["lights"] = this->Config->Lights.value().toJson();
        }
        if(this->Config->Engines.has_value()) {
            config["engines"] = this->Config->Engines.value().toJson();
        }
        config["flaps_pct"] = this->Config->FlapsPercent.value();
        config["gear_down"] = this->Config->GearDown.value();
        config["on_ground"] = this->Config->OnGround.value();
        config["spoilers_out"] = this->Config->SpoilersDeployed.value();
        root["config"] = config;
    }

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

QString AircraftConfigurationInfo::RequestFullConfig() const
{
    QJsonObject obj;
    obj["request"] = "full";

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

AircraftConfigurationInfo AircraftConfigurationInfo::FromJson(QString json)
{
    AircraftConfigurationInfo cfg;

    if(!json.isEmpty())
    {
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

        if(doc.object().contains("request"))
        {
            if(doc.object()["request"] == "full")
            {
                cfg.IsFullRequest = true;
            }
        }

        else if(doc.object().contains("config"))
        {
            cfg.Config = AircraftConfiguration();

            const QJsonObject json = doc.object()["config"].toObject();

            if(json.contains("is_full_data"))
            {
                cfg.Config->IsFullData = json["is_full_data"].toBool();
            }
            if(json.contains("flaps_pct"))
            {
                cfg.Config->FlapsPercent = json["flaps_pct"].toInt();
            }
            if(json.contains("gear_down"))
            {
                cfg.Config->GearDown = json["gear_down"].toBool();
            }
            if(json.contains("on_ground"))
            {
                cfg.Config->OnGround = json["on_ground"].toBool();
            }
            if(json.contains("spoilers_out"))
            {
                cfg.Config->SpoilersDeployed = json["spoilers_out"].toBool();
            }
        }
    }

    return cfg;
}

QJsonObject AircraftConfiguration::ToJson() const
{
    QJsonObject obj;
    if(IsFullData.has_value()) {
        obj["is_full_data"] = IsFullData.value();
    }
    if(Lights.has_value()) {
        obj["lights"] = Lights.value().toJson();
    }
    if(Engines.has_value()) {
        obj["engines"] = Engines.value().toJson();
    }
    if(GearDown.has_value()) {
        obj["gear_down"] = GearDown.value();
    }
    if(FlapsPercent.has_value()) {
        obj["flaps_pct"] = FlapsPercent.value();
    }
    if(SpoilersDeployed.has_value()) {
        obj["spoilers_out"] = SpoilersDeployed.value();
    }
    if(OnGround.has_value()) {
        obj["on_ground"] = OnGround.value();
    }
    return obj;
}

AircraftConfiguration AircraftConfiguration::Clone()
{
    AircraftConfiguration clone;
    clone.IsFullData = IsFullData;
    if(Lights.has_value()) {
        clone.Lights = Lights.value().Clone();
    }
    if(Engines.has_value()) {
        clone.Engines = Engines.value().Clone();
    }
    clone.GearDown = GearDown;
    clone.FlapsPercent = FlapsPercent;
    clone.SpoilersDeployed = SpoilersDeployed;
    clone.OnGround = OnGround;
    return clone;
}

void AircraftConfiguration::ApplyIncremental(AircraftConfiguration &inc)
{
    if(Lights.has_value()) {
        Lights->ApplyIncremental(inc.Lights.value());
    }
    if(Engines.has_value()) {
        Engines->ApplyIncremental(inc.Engines.value());
    }
    if(inc.GearDown.has_value()) GearDown = inc.GearDown.value();
    if(inc.FlapsPercent.has_value()) FlapsPercent = inc.FlapsPercent.value();
    if(inc.SpoilersDeployed.has_value()) SpoilersDeployed = inc.SpoilersDeployed.value();
    if(inc.OnGround.has_value()) OnGround = inc.OnGround.value();
}

AircraftConfiguration AircraftConfiguration::CreateIncremental(AircraftConfiguration &cfg)
{
    AircraftConfiguration inc{};
    inc.Lights = Lights->CreateIncremental(cfg.Lights.value());
    inc.Engines = Engines->CreateIncremental(cfg.Engines.value());
    if(cfg.GearDown != GearDown) inc.GearDown = cfg.GearDown;
    if(cfg.FlapsPercent != FlapsPercent) inc.FlapsPercent = cfg.FlapsPercent;
    if(cfg.SpoilersDeployed != SpoilersDeployed) inc.SpoilersDeployed = cfg.SpoilersDeployed;
    if(cfg.OnGround != OnGround) inc.OnGround = cfg.OnGround;
    return inc;
}

AircraftConfiguration AircraftConfiguration::FromUserAircraftData(UserAircraftData &uac)
{
    AircraftConfiguration cfg{};
    cfg.Lights = AircraftConfigurationLights::FromUserAircraftData(uac);
    cfg.Engines = AircraftConfigurationEngines::FromUserAircraftData(uac);
    cfg.GearDown = uac.GearDown;
    cfg.FlapsPercent = RoundUpToNearest5(uac.FlapsPercent);
    cfg.SpoilersDeployed = uac.SpeedBrakesDeployed;
    cfg.OnGround = uac.OnGround;
    return cfg;
}

int AircraftConfiguration::RoundUpToNearest5(double val)
{
    return (int)(val * 100.0 / 5.0) * 5;
}

QJsonObject AircraftConfigurationLights::toJson() const
{
    QJsonObject obj;
    if(StrobesOn.has_value()) {
        obj["strobe_on"] = StrobesOn.value();
    }
    if(LandingOn.has_value()) {
        obj["landing_on"] = LandingOn.value();
    }
    if(TaxiOn.has_value()) {
        obj["taxi_on"] = TaxiOn.value();
    }
    if(BeaconOn.has_value()) {
        obj["beacon_on"] = BeaconOn.value();
    }
    if(NavOn.has_value()) {
        obj["nav_on"] = NavOn.value();
    }
    return obj;
}

AircraftConfigurationLights AircraftConfigurationLights::Clone()
{
    AircraftConfigurationLights clone{};
    clone.StrobesOn = StrobesOn;
    clone.LandingOn = LandingOn;
    clone.TaxiOn = TaxiOn;
    clone.BeaconOn = BeaconOn;
    clone.NavOn = NavOn;
    return clone;
}

void AircraftConfigurationLights::ApplyIncremental(AircraftConfigurationLights &inc)
{
    if(inc.StrobesOn.has_value()) StrobesOn = inc.StrobesOn.value();
    if(inc.LandingOn.has_value()) LandingOn = inc.LandingOn.value();
    if(inc.TaxiOn.has_value()) TaxiOn = inc.TaxiOn.value();
    if(inc.BeaconOn.has_value()) BeaconOn = inc.BeaconOn.value();
    if(inc.NavOn.has_value()) NavOn = inc.NavOn.value();
}

AircraftConfigurationLights AircraftConfigurationLights::CreateIncremental(AircraftConfigurationLights& cfg)
{
    if(this == &cfg) return {};
    AircraftConfigurationLights inc{};
    if(cfg.StrobesOn != StrobesOn) inc.StrobesOn = cfg.StrobesOn;
    if(cfg.LandingOn != LandingOn) inc.LandingOn = cfg.LandingOn;
    if(cfg.TaxiOn != TaxiOn) inc.TaxiOn = cfg.TaxiOn;
    if(cfg.BeaconOn != BeaconOn) inc.BeaconOn = cfg.BeaconOn;
    if(cfg.NavOn != NavOn) inc.NavOn = cfg.NavOn;
    return inc;
}

AircraftConfigurationLights AircraftConfigurationLights::FromUserAircraftData(UserAircraftData &uac)
{
    AircraftConfigurationLights acl{};
    acl.StrobesOn = uac.StrobeLightsOn;
    acl.LandingOn = uac.LandingLightsOn;
    acl.TaxiOn = uac.TaxiLightsOn;
    acl.BeaconOn = uac.BeaconLightsOn;
    acl.NavOn = uac.NavLightsOn;
    return acl;
}

QJsonObject AircraftConfigurationEngine::toJson() const
{
    QJsonObject obj;
    if(Running.has_value()) {
        obj["on"] = Running.value();
    }
    return obj;
}

AircraftConfigurationEngine AircraftConfigurationEngine::Clone()
{
    AircraftConfigurationEngine clone{};
    clone.Running = Running;
    return clone;
}

void AircraftConfigurationEngine::ApplyIncremental(AircraftConfigurationEngine& inc)
{
    if(inc.Running.has_value()) Running = inc.Running;
}

AircraftConfigurationEngine AircraftConfigurationEngine::CreateIncremental(AircraftConfigurationEngine &cfg)
{
    if(this == &cfg) return {};
    AircraftConfigurationEngine inc{};
    if(cfg.Running != Running) inc.Running = cfg.Running;
    return inc;
}

AircraftConfigurationEngine AircraftConfigurationEngine::FromUserAircraftData(UserAircraftData &uac, int engineNum)
{
    AircraftConfigurationEngine ace{};
    switch(engineNum) {
    case 1:
        ace.Running = uac.Engine1Running;
        break;
    case 2:
        ace.Running = uac.Engine2Running;
        break;
    case 3:
        ace.Running = uac.Engine3Running;
        break;
    case 4:
        ace.Running = uac.Engine4Running;
        break;
    }
    return ace;
}

QJsonObject AircraftConfigurationEngines::toJson() const
{
    QJsonObject obj;
    if(HasEngine1Object()) {
        obj["1"] = Engine1->Running.value();
    }
    if(HasEngine2Object()) {
        obj["2"] = Engine2->Running.value();
    }
    if(HasEngine3Object()) {
        obj["3"] = Engine3->Running.value();
    }
    if(HasEngine4Object()) {
        obj["4"] = Engine4->Running.value();
    }
    return obj;
}

AircraftConfigurationEngines AircraftConfigurationEngines::Clone()
{
    AircraftConfigurationEngines clone{};
    if(HasEngine1Object()) clone.Engine1 = Engine1->Clone();
    if(HasEngine2Object()) clone.Engine2 = Engine2->Clone();
    if(HasEngine3Object()) clone.Engine3 = Engine3->Clone();
    if(HasEngine4Object()) clone.Engine4 = Engine4->Clone();
    return clone;
}

void AircraftConfigurationEngines::ApplyIncremental(AircraftConfigurationEngines &inc)
{
    if(inc.HasEngine1Object() && HasEngine1Object()) Engine1->ApplyIncremental(inc.Engine1.value());
    if(inc.HasEngine2Object() && HasEngine2Object()) Engine2->ApplyIncremental(inc.Engine2.value());
    if(inc.HasEngine3Object() && HasEngine3Object()) Engine3->ApplyIncremental(inc.Engine3.value());
    if(inc.HasEngine4Object() && HasEngine4Object()) Engine4->ApplyIncremental(inc.Engine4.value());
}

AircraftConfigurationEngines AircraftConfigurationEngines::CreateIncremental(AircraftConfigurationEngines &cfg)
{
    if(this == &cfg) return {};
    AircraftConfigurationEngines inc{};
    if(HasEngine1Object() && cfg.HasEngine1Object()) inc.Engine1 = Engine1->CreateIncremental(cfg.Engine1.value());
    if(HasEngine2Object() && cfg.HasEngine2Object()) inc.Engine2 = Engine2->CreateIncremental(cfg.Engine2.value());
    if(HasEngine3Object() && cfg.HasEngine3Object()) inc.Engine3 = Engine3->CreateIncremental(cfg.Engine3.value());
    if(HasEngine4Object() && cfg.HasEngine4Object()) inc.Engine4 = Engine4->CreateIncremental(cfg.Engine4.value());
    return inc;
}

AircraftConfigurationEngines AircraftConfigurationEngines::FromUserAircraftData(UserAircraftData &uac)
{
    AircraftConfigurationEngines ace{};
    if(uac.EngineCount >= 1) ace.Engine1 = AircraftConfigurationEngine::FromUserAircraftData(uac, 1);
    if(uac.EngineCount >= 2) ace.Engine2 = AircraftConfigurationEngine::FromUserAircraftData(uac, 2);
    if(uac.EngineCount >= 3) ace.Engine3 = AircraftConfigurationEngine::FromUserAircraftData(uac, 3);
    if(uac.EngineCount >= 4) ace.Engine4 = AircraftConfigurationEngine::FromUserAircraftData(uac, 4);
    return ace;
}

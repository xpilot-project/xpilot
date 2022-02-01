#include "aircraft_configuration.h"

QJsonObject AircraftConfiguration::ToJson() const
{
    QJsonObject cfg;

    if(!IsIncremental())
    {
        cfg["is_full_data"] = true;
    }

    if(GearDown.has_value())
    {
        cfg["gear_down"] = GearDown.value();
    }
    if(FlapsPercent.has_value())
    {
        cfg["flaps_pct"] = FlapsPercent.value();
    }
    if(SpoilersDeployed.has_value())
    {
        cfg["spoilers_out"] = SpoilersDeployed.value();
    }
    if(OnGround.has_value())
    {
        cfg["on_ground"] = OnGround.value();
    }

    if(Engines.has_value() && Engines->HasEngines())
    {
        QJsonObject engines;

        if(Engines->Engine1Running.has_value() || Engines->Engine1Reversing.has_value())
        {
            QJsonObject engine1;
            if(Engines->Engine1Running.has_value())
            {
                engine1.insert("on", Engines->Engine1Running.value());
            }
            if(Engines->Engine1Reversing.has_value())
            {
                engine1.insert("is_reversing", Engines->Engine1Reversing.value());
            }
            engines.insert("1", engine1);
        }

        if(Engines->Engine2Running.has_value() || Engines->Engine2Reversing.has_value())
        {
            QJsonObject engine2;
            if(Engines->Engine2Running.has_value())
            {
                engine2.insert("on", Engines->Engine2Running.value());
            }
            if(Engines->Engine2Reversing.has_value())
            {
                engine2.insert("is_reversing", Engines->Engine2Reversing.value());
            }
            engines.insert("2", engine2);
        }

        if(Engines->Engine3Running.has_value() || Engines->Engine3Reversing.has_value())
        {

            QJsonObject engine3;
            if(Engines->Engine3Running.has_value())
            {
                engine3.insert("on", Engines->Engine3Running.value());
            }
            if(Engines->Engine3Reversing.has_value())
            {
                engine3.insert("is_reversing", Engines->Engine3Reversing.value());
            }
            engines.insert("3", engine3);
        }

        if(Engines->Engine4Running.has_value() || Engines->Engine4Reversing.has_value())
        {
            QJsonObject engine4;
            if(Engines->Engine4Running.has_value())
            {
                engine4.insert("on", Engines->Engine4Running.value());
            }
            if(Engines->Engine4Reversing.has_value())
            {
                engine4.insert("is_reversing", Engines->Engine4Reversing.value());
            }
            engines.insert("4", engine4);
        }

        cfg["engines"] = engines;
    }

    if(Lights.has_value() && Lights->HasLights())
    {
        QJsonObject lights;

        if(Lights->StrobeOn.has_value())
        {
            lights["strobe_on"] = Lights->StrobeOn.value();
        }
        if(Lights->LandingOn.has_value())
        {
            lights["landing_on"] = Lights->LandingOn.value();
        }
        if(Lights->TaxiOn.has_value())
        {
            lights["taxi_on"] = Lights->TaxiOn.value();
        }
        if(Lights->BeaconOn.has_value())
        {
            lights["beacon_on"] = Lights->BeaconOn.value();
        }
        if(Lights->NavOn.has_value())
        {
            lights["nav_on"] = Lights->NavOn.value();
        }

        cfg["lights"] = lights;
    }

    return cfg;
}

AircraftConfiguration AircraftConfiguration::FromUserAircraftData(UserAircraftConfigData config)
{
    AircraftConfiguration cfg = AircraftConfiguration();
    cfg.Lights = AircraftConfigurationLights::FromUserAircraftData(config);
    cfg.Engines = AircraftConfigurationEngines::FromUserAircraftData(config);
    cfg.GearDown = config.GearDown;
    cfg.FlapsPercent = (int)(config.FlapsRatio * 100.0 / 5.0) * 5; // round to nearest 5
    cfg.SpoilersDeployed = config.SpeedbrakeRatio > 0;
    cfg.OnGround = config.OnGround;
    return cfg;
}

AircraftConfiguration AircraftConfiguration::CreateIncremental(AircraftConfiguration config)
{
    AircraftConfiguration inc = AircraftConfiguration();
    if(config.Lights.has_value() && config.Lights->HasLights())
    {
        inc.Lights = Lights->CreateIncremental(config.Lights.value());
    }
    if(config.Engines.has_value() && config.Engines->HasEngines())
    {
        inc.Engines = Engines->CreateIncremental(config.Engines.value());
    }
    if(config.GearDown != GearDown) inc.GearDown = config.GearDown;
    if(config.FlapsPercent != FlapsPercent) inc.FlapsPercent = config.FlapsPercent;
    if(config.SpoilersDeployed != SpoilersDeployed) inc.SpoilersDeployed = config.SpoilersDeployed;
    if(config.OnGround != OnGround) inc.OnGround = config.OnGround;
    return inc;
}

QString AircraftConfigurationInfo::ToJson() const
{
    QJsonObject obj;

    if(FullRequest.has_value() && FullRequest.value())
    {
        obj["request"] = "full";
    }

    else if(Config.has_value())
    {
        obj["config"] = Config.value().ToJson();
    }

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

AircraftConfigurationInfo AircraftConfigurationInfo::FromJson(const QString &json)
{
    if(json.isEmpty()) return {};

    AircraftConfigurationInfo info{};

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

    if(doc.object().contains("request"))
    {
        if(doc.object()["request"] == "full")
        {
            info.FullRequest = true;
        }
    }
    else if(doc.object().contains("config"))
    {
        info.Config = AircraftConfiguration();

        const QJsonObject config = doc.object()["config"].toObject();

        if(config.contains("is_full_data"))
        {
            info.Config->IsFullData = config["is_full_data"].toBool();
        }
        if(config.contains("gear_down"))
        {
            info.Config->GearDown = config["gear_down"].toBool();
        }
        if(config.contains("flaps_pct"))
        {
            info.Config->FlapsPercent = config["flaps_pct"].toInt();
        }
        if(config.contains("spoilers_out"))
        {
            info.Config->SpoilersDeployed = config["spoilers_out"].toBool();
        }
        if(config.contains("on_ground"))
        {
            info.Config->OnGround = config["on_ground"].toBool();
        }
        if(config.contains("engines"))
        {
            info.Config->Engines = AircraftConfigurationEngines();

            const QJsonObject engines = config["engines"].toObject();

            if(engines.contains("1"))
            {
                auto engine = engines["1"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine1Running = engine["on"].toBool();
                }
                if(engine.contains("is_reversing"))
                {
                    info.Config->Engines->Engine1Reversing = engine["is_reversing"].toBool();
                }
            }
            if(engines.contains("2"))
            {
                auto engine = engines["2"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine2Running = engine["on"].toBool();
                }
                if(engine.contains("is_reversing"))
                {
                    info.Config->Engines->Engine2Reversing = engine["is_reversing"].toBool();
                }
            }
            if(engines.contains("3"))
            {
                auto engine = engines["3"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine3Running = engine["on"].toBool();
                }
                if(engine.contains("is_reversing"))
                {
                    info.Config->Engines->Engine3Reversing = engine["is_reversing"].toBool();
                }
            }
            if(engines.contains("4"))
            {
                auto engine = engines["4"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine4Running = engine["on"].toBool();
                }
                if(engine.contains("is_reversing"))
                {
                    info.Config->Engines->Engine4Reversing = engine["is_reversing"].toBool();
                }
            }
        }
        if(config.contains("lights"))
        {
            info.Config->Lights = AircraftConfigurationLights();

            const QJsonObject lights = config["lights"].toObject();

            if(lights.contains("strobe_on"))
            {
                info.Config->Lights->StrobeOn = lights["strobe_on"].toBool();
            }
            if(lights.contains("landing_on"))
            {
                info.Config->Lights->LandingOn = lights["landing_on"].toBool();
            }
            if(lights.contains("taxi_on"))
            {
                info.Config->Lights->TaxiOn = lights["taxi_on"].toBool();
            }
            if(lights.contains("beacon_on"))
            {
                info.Config->Lights->BeaconOn = lights["beacon_on"].toBool();
            }
            if(lights.contains("nav_on"))
            {
                info.Config->Lights->NavOn = lights["nav_on"].toBool();
            }
        }
    }

    return info;
}

AircraftConfigurationLights AircraftConfigurationLights::FromUserAircraftData(UserAircraftConfigData config)
{
    AircraftConfigurationLights cfg = AircraftConfigurationLights();
    cfg.LandingOn = config.LandingLightsOn;
    cfg.TaxiOn = config.TaxiLightsOn;
    cfg.BeaconOn = config.BeaconOn;
    cfg.NavOn = config.NavLightsOn;
    cfg.StrobeOn = config.StrobesOn;
    return cfg;
}

AircraftConfigurationLights AircraftConfigurationLights::CreateIncremental(AircraftConfigurationLights config)
{
    AircraftConfigurationLights inc = AircraftConfigurationLights();
    if(config.StrobeOn != StrobeOn) inc.StrobeOn = config.StrobeOn;
    if(config.TaxiOn != TaxiOn) inc.TaxiOn = config.TaxiOn;
    if(config.LandingOn != LandingOn) inc.LandingOn = config.LandingOn;
    if(config.BeaconOn != BeaconOn) inc.BeaconOn = config.BeaconOn;
    if(config.NavOn != NavOn) inc.NavOn = config.NavOn;
    return inc;
}

AircraftConfigurationEngines AircraftConfigurationEngines::FromUserAircraftData(UserAircraftConfigData config)
{
    AircraftConfigurationEngines cfg = AircraftConfigurationEngines();
    if(config.EngineCount >= 1) cfg.Engine1Running = config.Engine1Running;
    if(config.EngineCount >= 2) cfg.Engine2Running = config.Engine2Running;
    if(config.EngineCount >= 3) cfg.Engine3Running = config.Engine3Running;
    if(config.EngineCount >= 4) cfg.Engine4Running = config.Engine4Running;
    if(config.EngineCount >= 1) cfg.Engine1Reversing = config.Engine1Reversing;
    if(config.EngineCount >= 2) cfg.Engine2Reversing = config.Engine2Reversing;
    if(config.EngineCount >= 3) cfg.Engine3Reversing = config.Engine3Reversing;
    if(config.EngineCount >= 4) cfg.Engine4Reversing = config.Engine4Reversing;
    return cfg;
}

AircraftConfigurationEngines AircraftConfigurationEngines::CreateIncremental(AircraftConfigurationEngines config)
{
    AircraftConfigurationEngines inc = AircraftConfigurationEngines();
    if(config.Engine1Running != Engine1Running) inc.Engine1Running = config.Engine1Running;
    if(config.Engine2Running != Engine2Running) inc.Engine2Running = config.Engine2Running;
    if(config.Engine3Running != Engine3Running) inc.Engine3Running = config.Engine3Running;
    if(config.Engine4Running != Engine4Running) inc.Engine4Running = config.Engine4Running;
    if(config.Engine1Reversing != Engine1Reversing) inc.Engine1Reversing = config.Engine1Reversing;
    if(config.Engine2Reversing != Engine2Reversing) inc.Engine2Reversing = config.Engine2Reversing;
    if(config.Engine3Reversing != Engine3Reversing) inc.Engine3Reversing = config.Engine3Reversing;
    if(config.Engine4Reversing != Engine4Reversing) inc.Engine4Reversing = config.Engine4Reversing;
    return inc;
}

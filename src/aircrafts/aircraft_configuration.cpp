#include "aircraft_configuration.h"

QString AircraftConfiguration::ToJson() const
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

        if(Engines->Engine1Running.has_value())
        {
            QJsonObject running;
            running.insert("on", Engines->Engine1Running.value());
            engines.insert("1", running);
        }

        if(Engines->Engine2Running.has_value())
        {
            QJsonObject running;
            running.insert("on", Engines->Engine2Running.value());
            engines.insert("2", running);
        }

        if(Engines->Engine3Running.has_value())
        {
            QJsonObject running;
            running.insert("on", Engines->Engine3Running.value());
            engines.insert("3", running);
        }

        if(Engines->Engine4Running.has_value())
        {
            QJsonObject running;
            running.insert("on", Engines->Engine4Running.value());
            engines.insert("4", running);
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
        if(Lights->LogoOn.has_value())
        {
            lights["logo_on"] = Lights->LogoOn.value();
        }

        cfg["lights"] = lights;
    }

    QJsonDocument doc(cfg);
    return doc.toJson(QJsonDocument::Compact);
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
            }
            if(engines.contains("2"))
            {
                auto engine = engines["2"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine2Running = engine["on"].toBool();
                }
            }
            if(engines.contains("3"))
            {
                auto engine = engines["3"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine3Running = engine["on"].toBool();
                }
            }
            if(engines.contains("4"))
            {
                auto engine = engines["4"].toObject();
                if(engine.contains("on"))
                {
                    info.Config->Engines->Engine4Running = engine["on"].toBool();
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
            if(lights.contains("logo_on"))
            {
                info.Config->Lights->LogoOn = lights["logo_on"].toBool();
            }
        }
    }

    return info;
}

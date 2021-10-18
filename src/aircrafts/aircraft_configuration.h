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

    QString ToJson() const
    {
        QJsonObject obj;
        QJsonDocument doc(obj);
        return doc.toJson(QJsonDocument::Compact);
    }
};

class AircraftConfigurationInfo
{
public:
    std::optional<bool> FullRequest;
    std::optional<AircraftConfiguration> Config;
    bool HasConfig() const { return Config.has_value(); }

    QString ToJson() const
    {
        QJsonObject obj;

        if(FullRequest.has_value() && FullRequest.value())
        {
            obj["request"] = "full";
        }

        if(Config.has_value())
        {
            obj["config"] = Config.value().ToJson();
        }

        QJsonDocument doc(obj);
        return doc.toJson(QJsonDocument::Compact);
    }

    static AircraftConfigurationInfo FromJson(const QString& json)
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
};

#endif

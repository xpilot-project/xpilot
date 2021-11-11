/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2020 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#ifndef NetworkAircraftConfig_h
#define NetworkAircraftConfig_h

#include <string>
#include <optional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace nlohmann
{
    template<typename T>
    struct adl_serializer<std::optional<T>>
    {
        static void to_json(json& j, const std::optional<T>& value)
        {
            if (!value)
            {
                j = nullptr;
            }
            else
            {
                j = *value;
            }
        }

        static void from_json(json const& j, std::optional<T>& value)
        {
            if (j.is_null())
            {
                value.reset();
            }
            else
            {
                value = j.get<T>();
            }
        }
    };
}

namespace xpilot
{
    struct NetworkAircraftConfigLights
    {
        std::optional<bool> strobesOn;
        std::optional<bool> landingOn;
        std::optional<bool> taxiOn;
        std::optional<bool> beaconOn;
        std::optional<bool> navOn;
    };

    struct NetworkAircraftConfigData
    {
        std::string callsign;
        std::optional<NetworkAircraftConfigLights> lights;
        std::optional<bool> enginesRunning;
        std::optional<bool> reverseThrust;
        std::optional<bool> onGround;
        std::optional<bool> spoilersDeployed;
        std::optional<bool> gearDown;
        std::optional<float> flapsPct;
        std::optional<bool> fullConfig;
    };

    struct NetworkAircraftConfig
    {
        std::string type;
        NetworkAircraftConfigData data;
    };

    void from_json(const json& j, NetworkAircraftConfigData& data);
    void from_json(const json& j, NetworkAircraftConfigLights& lights);
    void from_json(const json& j, NetworkAircraftConfig& config);
}

#endif // !NetworkAircraftConfig_h
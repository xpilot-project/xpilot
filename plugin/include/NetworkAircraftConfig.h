/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2022 Justin Shannon
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

#include <nlohmann/json.hpp>

#include <string>
#include <optional>

using json = nlohmann::json;
using namespace std;

namespace nlohmann
{
    template<typename T>
    struct adl_serializer<optional<T>>
    {
        static void to_json(json& j, const optional<T>& value)
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

        static void from_json(json const& j, optional<T>& value)
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
        optional<bool> strobesOn;
        optional<bool> landingOn;
        optional<bool> taxiOn;
        optional<bool> beaconOn;
        optional<bool> navOn;
    };

    struct NetworkAircraftConfigData
    {
        string callsign;
        optional<NetworkAircraftConfigLights> lights;
        optional<bool> enginesRunning;
        optional<bool> enginesReversing;
        optional<bool> onGround;
        optional<bool> spoilersDeployed;
        optional<bool> gearDown;
        optional<float> flapsPct;
        optional<bool> fullConfig;
    };

    struct NetworkAircraftConfig
    {
        string type;
        NetworkAircraftConfigData data;
    };

    void from_json(const json& j, NetworkAircraftConfigData& data);
    void from_json(const json& j, NetworkAircraftConfigLights& lights);
    void from_json(const json& j, NetworkAircraftConfig& config);
}

#endif // !NetworkAircraftConfig_h
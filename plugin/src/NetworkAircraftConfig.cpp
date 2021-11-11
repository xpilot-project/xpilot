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

#include "NetworkAircraftConfig.h"

namespace xpilot
{
	void from_json(const json& j, NetworkAircraftConfigData& data)
	{
		if (j.find("callsign") != j.end())
			j.at("callsign").get_to(data.callsign);

		if (j.find("full_config") != j.end())
			j.at("full_config").get_to(data.fullConfig);

		if (j.find("lights") != j.end())
			j.at("lights").get_to(data.lights);

		if (j.find("gear_down") != j.end())
			j.at("gear_down").get_to(data.gearDown);

		if (j.find("on_ground") != j.end())
			j.at("on_ground").get_to(data.onGround);

		if (j.find("flaps") != j.end())
			j.at("flaps").get_to(data.flapsPct);

		if (j.find("engines_on") != j.end())
			j.at("engines_on").get_to(data.enginesRunning);

		if (j.find("spoilers_deployed") != j.end())
			j.at("spoilers_deployed").get_to(data.spoilersDeployed);

		if (j.find("reverse_thrust") != j.end())
			j.at("reverse_thrust").get_to(data.reverseThrust);
	}

	void from_json(const json& j, NetworkAircraftConfigLights& lights)
	{
		if (j.find("strobe_lights_on") != j.end())
			j.at("strobe_lights_on").get_to(lights.strobesOn);

		if (j.find("landing_lights_on") != j.end())
			j.at("landing_lights_on").get_to(lights.landingOn);

		if (j.find("taxi_lights_on") != j.end())
			j.at("taxi_lights_on").get_to(lights.taxiOn);

		if (j.find("beacon_lights_on") != j.end())
			j.at("beacon_lights_on").get_to(lights.beaconOn);

		if (j.find("nav_lights_on") != j.end())
			j.at("nav_lights_on").get_to(lights.navOn);
	}

	void from_json(const json& j, NetworkAircraftConfig& config)
	{
		j.at("type").get_to(config.type);
		j.at("data").get_to(config.data);
	}
}
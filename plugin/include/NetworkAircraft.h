/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2021 Justin Shannon
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

#ifndef NetworkAircraft_h
#define NetworkAircraft_h

#include <deque>

#include "XPilotAPI.h"
#include "InterpolatedState.h"
#include "TerrainProbe.h"

#include "XPCAircraft.h"
#include "XPMPAircraft.h"
#include "XPMPMultiplayer.h"

#include "Vector3.hpp"
#include <optional>

namespace xpilot
{
    struct AircraftVisualState
    {
        double Lat;
        double Lon;
        double Altitude;
        double Pitch;
        double Heading;
        double Bank;
    };

    class NetworkAircraft : public XPMP2::Aircraft
    {
    public:
        NetworkAircraft(const std::string& _callsign, const AircraftVisualState& _visualState, const std::string& _icaoType, const std::string& _icaoAirline, const std::string& _livery, XPMPPlaneID _modeS_id, const std::string& _modelName);

        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const;
        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const;

        void UpdateErrorVectors(double interval);

        bool on_ground;
        bool gear_down;
        bool engines_running;
        bool reverse_thrust;
        float ground_speed;
        float target_gear_position;
        float target_flaps_position;
        float target_spoiler_position;
        bool spoilers_deployed;
        XPMPPlaneSurfaces_t surfaces;
        std::string origin;
        std::string destination;
        std::chrono::system_clock::time_point prev_surface_update_time;
        int fast_positions_received_count;
        bool first_render_pending;

        XPMPPlanePosition_t position;
        XPMPPlaneRadar_t radar;

        double terrain_offset;
        double previous_terrain_offset;
        std::optional<double> ground_altitude = {};
        std::optional<double> target_terrain_offset = {};
        std::optional<double> adjusted_altitude = {};
        TerrainProbe terrain_probe;

        AircraftVisualState remote_visual_state;
        AircraftVisualState predicted_visual_state;

        Vector3 positional_velocity_vector;
        Vector3 positional_velocity_vector_error;
        Vector3 rotational_velocity_vector;
        Vector3 rotational_velocity_vector_error;

        std::chrono::steady_clock::time_point last_fast_position_timestamp;
        std::chrono::steady_clock::time_point last_slow_position_timestamp;

    protected:
        virtual void UpdatePosition(float, int);
        void Extrapolate(Vector3 velocityVector, Vector3 rotationVector, double interval);
        void AutoLevel(float frameRate);
        static double NormalizeDegrees(double value, double lowerBound, double upperBound);
    };
}

#endif // !NetworkAircraft_h



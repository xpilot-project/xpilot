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
#include <optional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "XPilotAPI.h"
#include "InterpolatedState.h"
#include "TerrainProbe.h"
#include "Utilities.h"
#include "Vector3.hpp"

#include "XPCAircraft.h"
#include "XPMPAircraft.h"
#include "XPMPMultiplayer.h"

#include "AircraftSoundManager.h"
#include "AL/al.h"

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

    enum class EngineClass
    {
        Helicopter,
        PistonProp,
        TurboProp,
        JetEngine,
        Unknown
    };

    enum class EngineState
    {
        Starter,
        Normal
    };

    class NetworkAircraft : public XPMP2::Aircraft
    {
    public:
        NetworkAircraft(const std::string& _callsign, const AircraftVisualState& _visualState, const std::string& _icaoType, const std::string& _icaoAirline, const std::string& _livery, XPMPPlaneID _modeS_id, const std::string& _modelName);
        virtual ~NetworkAircraft();

        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const;
        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const;

        void UpdateErrorVectors(double interval);

        bool on_ground;
        bool gear_down;
        bool engines_running = false;
        bool was_engines_running = false;
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

        ALuint m_soundBuffer = 0;
        float m_pitch = 1.0f;
        float m_gain = 1.0f;
        bool m_loopSound = true;
        bool m_soundLoaded = false;
        bool m_soundsPlaying = false;

        bool firstSoundInitialized = false;

        ALuint m_soundSources[8];

        void audioLoop();
        void startSoundThread();
        void stopSoundThread();
        void setEngineState(EngineState state);
        void stopSounds();
        std::unique_ptr<std::thread> m_soundThread;
        std::atomic_bool m_shouldEndSoundThread = { false };
        std::chrono::system_clock::time_point prev_update_time;
        std::chrono::system_clock::time_point starterBegun;
        float currentGain = 0.0f;

        const float PistonStarterTime = 2.0f;
        const float JetStarterTime = 19.0f;
        const float TurboStarterTime = 13.0f;

        EngineClass m_engineClass;
        EngineState m_engineState;
        int m_engineCount;
        vect m_velocity;
        vect m_position;
    };
}

#endif // !NetworkAircraft_h



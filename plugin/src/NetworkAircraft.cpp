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
#include "NetworkAircraft.h"
#include "Utilities.h"
#include "Config.h"
#include "GeoCalc.hpp"
#include "Quaternion.hpp"
#include <alext.h>

namespace xpilot
{
    const double TERRAIN_HEIGHT_TOLERANCE = 200.0;

#define CHECKERR(msg) { ALuint e = alGetError(); if (e != AL_NO_ERROR) LOG_MSG(logERROR, msg); }

    double CalculateNormalizedDelta(double start, double end, double lowerBound, double upperBound)
    {
        double range = upperBound - lowerBound;
        double halfRange = range / 2.0;

        if (abs(end - start) > halfRange)
        {
            end += (end > start ? -range : range);
        }

        return end - start;
    }

    double HeadingDiff(double head1, double head2)
    {
        if (std::abs(head2 - head1) > 180)
        {
            if (head1 < head2)
            {
                head1 += 360;
            }
            else
            {
                head2 += 360;
            }
        }
        return head2 - head1;
    }

    inline float RpmToDegree(float rpm, double s)
    {
        return rpm / 60.0f * float(s) * 360.0f;
    }

    inline double LinearInterpolate(double y1, double y2, double mu)
    {
        return y1 + (mu * (y2 - y1));
    }

    inline double SmoothStepInterpolate(double y1, double y2, double mu)
    {
        double mu2 = std::pow(mu, 2) * (3 - (2 * mu));
        return LinearInterpolate(y1, y2, mu2);
    }

    NetworkAircraft::NetworkAircraft(
        const std::string& _callsign,
        const AircraftVisualState& _visualState,
        const std::string& _icaoType, 
        const std::string& _icaoAirline, 
        const std::string& _livery,
        XPMPPlaneID _modeS_id = 0,
        const std::string& _modelName = "") :
        XPMP2::Aircraft(_icaoType, _icaoAirline, _livery, _modeS_id, _modelName),
        engines_running(false),
        gear_down(false),
        on_ground(false),
        fast_positions_received_count(0),
        reverse_thrust(false),
        spoilers_deployed(false),
        target_flaps_position(0.0f),
        target_gear_position(0.0f),
        target_spoiler_position(0.0f),
        ground_speed(0.0),
        first_render_pending(true)
    {
        label = _callsign;
        strScpy(acInfoTexts.tailNum, _callsign.c_str(), sizeof(acInfoTexts.tailNum));
        strScpy(acInfoTexts.icaoAcType, acIcaoType.c_str(), sizeof(acInfoTexts.icaoAcType));
        strScpy(acInfoTexts.icaoAirline, acIcaoAirline.c_str(), sizeof(acInfoTexts.icaoAirline));

        SetLocation(_visualState.Lat, _visualState.Lon, _visualState.Altitude);
        SetHeading(_visualState.Heading);
        SetPitch(_visualState.Pitch);
        SetRoll(_visualState.Bank);

        last_slow_position_timestamp = std::chrono::steady_clock::now();
        predicted_visual_state = _visualState;
        remote_visual_state = _visualState;
        positional_velocity_vector = Vector3::Zero();
        rotational_velocity_vector = Vector3::Zero();

        SetVisible(false);

        float zero[3] = { 0,0,0 };
        alGenSources(1, &m_soundSource);
        CHECKERR("Can't create sound source");

        auto model = GetModelInfo();

        if (model.doc8643Classification.size() > 0) {
            // helicoptor or gyrocopter
            if (model.doc8643Classification[0] == 'H' || model.doc8643Classification[0] == 'G') {
                m_engineClass = EngineClass::Helicopter;
                ALuint sound = AircraftSoundManager::get()->helicopter();
                alSourcei(m_soundSource, AL_BUFFER, sound);
                CHECKERR("Can't buffer sound data.");
            }
            else {
                // jet
                if (model.doc8643Classification[2] == 'J') {
                    m_engineClass = EngineClass::JetEngine;
                    ALuint sound = AircraftSoundManager::get()->jetEngine();
                    alSourcei(m_soundSource, AL_BUFFER, sound);
                    CHECKERR("Can't buffer sound data.");
                }
                // turboprop
                else if (model.doc8643Classification[2] == 'T') {
                    m_engineClass = EngineClass::TurboProp;
                    ALuint sound = AircraftSoundManager::get()->turboProp();
                    alSourcei(m_soundSource, AL_BUFFER, sound);
                    CHECKERR("Can't buffer sound data.");
                }
                // prop
                else if (model.doc8643Classification[3] == 'P') {
                    m_engineClass = EngineClass::PistonProp;
                    ALuint sound = AircraftSoundManager::get()->pistonProp();
                    alSourcei(m_soundSource, AL_BUFFER, sound);
                    CHECKERR("Can't buffer sound data.");
                }
                else {
                    // don't know... default to jet
                    m_engineClass = EngineClass::JetEngine;
                    ALuint sound = AircraftSoundManager::get()->jetEngine();
                    alSourcei(m_soundSource, AL_BUFFER, sound);
                    CHECKERR("Can't buffer sound data.");
                }
            }
        }

        alSourcef(m_soundSource, AL_PITCH, m_pitch);
        alSourcef(m_soundSource, AL_GAIN, m_gain);
        alSourcei(m_soundSource, AL_LOOPING, m_loopSound);
        alSourcei(m_soundSource, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcefv(m_soundSource, AL_VELOCITY, zero);
        zero[2] = 5.0f;
        alSourcefv(m_soundSource, AL_POSITION, zero);
        CHECKERR("Can't set sound source");

        // initialize
        m_velocity = vect(0, 0, 0);
        m_position = vect(0, 0, 0);
        m_dist = 3000.0f;
    }

    NetworkAircraft::~NetworkAircraft()
    {
        stopSoundThread();
        alDeleteSources(1, &m_soundSource);
    }

    void NetworkAircraft::Extrapolate(
        Vector3 velocityVector,
        Vector3 rotationVector,
        double interval)
    {
        double lat_change = MetersToDegrees(velocityVector.Z * interval);
        double new_lat = NormalizeDegrees(predicted_visual_state.Lat + lat_change, -90.0, 90.0);

        double lon_change = MetersToDegrees(velocityVector.X * interval / LongitudeScalingFactor(predicted_visual_state.Lat));
        double new_lon = NormalizeDegrees(predicted_visual_state.Lon + lon_change, -180.0, 180.0);

        double alt_change = velocityVector.Y * interval * 3.28084;
        double new_alt = predicted_visual_state.Altitude + alt_change;

        AutoLevel(1.0 / interval);
        SetLocation(new_lat, new_lon, adjusted_altitude.has_value() ? adjusted_altitude.value() : new_alt);

        Quaternion current_orientation = Quaternion::FromEuler(
            DegreesToRadians(GetPitch()),
            DegreesToRadians(GetHeading()),
            DegreesToRadians(GetRoll())
        );

        Quaternion rotation = Quaternion::FromEuler(
            rotationVector.X,
            rotationVector.Y,
            rotationVector.Z
        );

        Quaternion slerp = Quaternion::Slerp(
            Quaternion::Identity(),
            rotation,
            interval > 1.0 ? 1.0 : interval
        );

        Quaternion result = current_orientation * slerp;

        Vector3 new_orientation = Quaternion::ToEuler(result);

        double new_pitch = RadiansToDegrees(new_orientation.X);
        double new_heading = RadiansToDegrees(new_orientation.Y);
        double new_roll = RadiansToDegrees(new_orientation.Z);

        predicted_visual_state.Lat = new_lat;
        predicted_visual_state.Lon = new_lon;
        predicted_visual_state.Altitude = new_alt;
        predicted_visual_state.Pitch = new_pitch;
        predicted_visual_state.Bank = new_roll;
        predicted_visual_state.Heading = new_heading;

        SetPitch(new_pitch);
        SetHeading(new_heading);
        SetRoll(new_roll);
    }

    void NetworkAircraft::AutoLevel(float frameRate)
    {
        ground_altitude = {};
        if (predicted_visual_state.Altitude < 18000.0)
        {
            ground_altitude = terrain_probe.getTerrainElevation(
                predicted_visual_state.Lat,
                predicted_visual_state.Lon
            );
        }

        if (!ground_altitude.has_value())
        {
            return;
        }

        // Clear the target terrain offset if the aircraft no longer appears to be on the ground.
        // This will cause the autoleveler to smoothly take out the terrain offset.
        if (target_terrain_offset.has_value() && !on_ground)
        {
            previous_terrain_offset = terrain_offset;
            target_terrain_offset = {};
        }

        double smoothedTerrainOffset = 0.0;

        if (!target_terrain_offset.has_value() && on_ground)
        {
            // The aircraft just touched down. Calculate the necessary terrain offset.
            double agl = remote_visual_state.Altitude - ground_altitude.value();
            if (std::abs(agl) < TERRAIN_HEIGHT_TOLERANCE)
            {
                target_terrain_offset = ground_altitude.value() - remote_visual_state.Altitude;
                terrain_offset = first_render_pending ? target_terrain_offset.value() : 0.0;
                smoothedTerrainOffset = terrain_offset;
            }
        }
        else if (target_terrain_offset.has_value())
        {
            // Update the target terrain offset as the aircraft moves around on the ground, 
            // since it may have uneven terrain.
            if (on_ground)
            {
                target_terrain_offset = ground_altitude.value() - remote_visual_state.Altitude;
            }

            // Aircraft is transitioning to local terrain height. 
            // Interpolate the terrain offset so that the altitude doesn't jump.
            if (terrain_offset != target_terrain_offset.value())
            {
                if (target_terrain_offset.value() == 0.0)
                {
                    terrain_offset = 0.0;
                    smoothedTerrainOffset = 0.0;
                }
                else
                {
                    terrain_offset += target_terrain_offset.value() / (frameRate * 5.0);
                    if (std::abs(terrain_offset) > std::abs(target_terrain_offset.value()))
                    {
                        terrain_offset = target_terrain_offset.value();
                    }
                    smoothedTerrainOffset = SmoothStepInterpolate(0.0, target_terrain_offset.value(), terrain_offset / target_terrain_offset.value());
                }
            }
            else
            {
                smoothedTerrainOffset = terrain_offset;
            }
        }
        else if (!target_terrain_offset.has_value() && (terrain_offset != 0.0))
        {
            // Aircraft is climbing out. Slowly interpolate the terrain offset back to zero.
            terrain_offset -= previous_terrain_offset / (frameRate * 5.0);
            if (previous_terrain_offset > 0.0)
            {
                if (terrain_offset < 0.0)
                {
                    terrain_offset = 0.0;
                }
            }
            else
            {
                if (terrain_offset > 0.0)
                {
                    terrain_offset = 0.0;
                }
            }
            smoothedTerrainOffset = terrain_offset;
        }

        adjusted_altitude = predicted_visual_state.Altitude + smoothedTerrainOffset;

        if (adjusted_altitude < ground_altitude.value())
        {
            adjusted_altitude = ground_altitude.value();
        }
    }

    void NetworkAircraft::UpdateErrorVectors(double interval)
    {
        if (positional_velocity_vector == Vector3::Zero())
        {
            positional_velocity_vector_error = Vector3::Zero();
            rotational_velocity_vector_error = Vector3::Zero();
            return;
        }

        double latDelta = DegreesToMeters(CalculateNormalizedDelta(
            predicted_visual_state.Lat,
            remote_visual_state.Lat,
            -90.0,
            90.0
        ));

        double lonDelta = DegreesToMeters(CalculateNormalizedDelta(
            predicted_visual_state.Lon,
            remote_visual_state.Lon,
            -180.0,
            180.0
        ));
        lonDelta *= LongitudeScalingFactor(remote_visual_state.Lat);

        double altDelta = (remote_visual_state.Altitude - predicted_visual_state.Altitude) * 0.3048;

        positional_velocity_vector_error = Vector3(
            lonDelta / interval,
            altDelta / interval,
            latDelta / interval
        );

        Quaternion currentOrientation = Quaternion::FromEuler(
            DegreesToRadians(GetPitch()),
            DegreesToRadians(GetHeading()),
            DegreesToRadians(GetRoll())
        );

        Quaternion targetOrientation = Quaternion::FromEuler(
            DegreesToRadians(remote_visual_state.Pitch),
            DegreesToRadians(remote_visual_state.Heading),
            DegreesToRadians(remote_visual_state.Bank)
        );

        Quaternion delta = Quaternion::Inverse(currentOrientation) * targetOrientation;

        Vector3 result = Quaternion::ToEuler(delta);

        rotational_velocity_vector_error = Vector3(result.X / interval, result.Y / interval, result.Z / interval);
    }

    double NetworkAircraft::NormalizeDegrees(double value, double lowerBound, double upperBound)
    {
        double range = upperBound - lowerBound;
        if (value < lowerBound)
        {
            return value + range;
        }
        if (value > upperBound)
        {
            return value - range;
        }
        return value;
    }

    void NetworkAircraft::startSoundThread()
    {
        if (m_soundLoaded) return;

        m_soundThread = std::make_unique<std::thread>([&]()
            {
                audioLoop();
            });

        m_soundLoaded = true;
    }

    void NetworkAircraft::stopSoundThread()
    {
        if (m_soundThread) {
            m_soundThread->join();
        }
    }

    void NetworkAircraft::audioLoop()
    {
        alSourcePlay(m_soundSource);
    }

    void NetworkAircraft::UpdatePosition(float _elapsedSinceLastCall, int)
    {
        Extrapolate(
            positional_velocity_vector + positional_velocity_vector_error,
            rotational_velocity_vector + rotational_velocity_vector_error,
            _elapsedSinceLastCall
        );

        if (on_ground)
        {
            double rpm = (60 / (2 * M_PI * 3.2)) * positional_velocity_vector.X * -1;
            double rpmDeg = RpmToDegree(GetTireRotRpm(), _elapsedSinceLastCall);

            SetTireRotRpm(rpm);
            SetTireRotAngle(GetTireRotAngle() + rpmDeg);
            while (GetTireRotAngle() >= 360.0f)
            {
                SetTireRotAngle(GetTireRotAngle() - 360.0f);
            }
        }

        const auto now = std::chrono::system_clock::now();
        static const float epsilon = std::numeric_limits<float>::epsilon();
        const auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev_surface_update_time);

        target_gear_position = gear_down ? 1.0f : 0.0f;
        target_spoiler_position = spoilers_deployed ? 1.0f : 0.0f;

        if (fast_positions_received_count <= 2)
        {
            // we don't want to wait for the animation on first load...
            // looks particular funny with the gear extending after the aircraft
            // loads on the ground
            surfaces.gearPosition = target_gear_position;
            surfaces.flapRatio = target_flaps_position;
            surfaces.spoilerRatio = target_spoiler_position;
        }
        else
        {
            const float f = surfaces.gearPosition - target_gear_position;
            if (std::abs(f) > epsilon)
            {
                // interpolate gear position
                constexpr float gearMoveTimeMs = 10000;
                const auto gearPositionDiffRemaining = target_gear_position - surfaces.gearPosition;

                const auto gearPositionDiffThisFrame = (diffMs.count()) / gearMoveTimeMs;
                surfaces.gearPosition += std::copysign(gearPositionDiffThisFrame, gearPositionDiffRemaining);
                surfaces.gearPosition = (std::max)(0.0f, (std::min)(surfaces.gearPosition, 1.0f));
            }

            const float f2 = surfaces.flapRatio - target_flaps_position;
            if (std::abs(f2) > epsilon)
            {
                // interpolate flap position
                constexpr float flapMoveTimeMs = 10000;
                const auto flapPositionDiffRemaining = target_flaps_position - surfaces.flapRatio;

                const auto flapPositionDiffThisFrame = (diffMs.count()) / flapMoveTimeMs;
                surfaces.flapRatio += std::copysign(flapPositionDiffThisFrame, flapPositionDiffRemaining);
                surfaces.flapRatio = (std::max)(0.0f, (std::min)(surfaces.flapRatio, 1.0f));
            }

            const float f3 = surfaces.spoilerRatio - target_spoiler_position;
            if (std::abs(f3) > epsilon)
            {
                // interpolate spoiler position
                constexpr float spoilerMoveTimeMs = 2000;
                const auto spoilerPositionDiffRemaining = target_spoiler_position - surfaces.spoilerRatio;

                const auto spoilerPositionDiffThisFrame = (diffMs.count()) / spoilerMoveTimeMs;
                surfaces.spoilerRatio += std::copysign(spoilerPositionDiffThisFrame, spoilerPositionDiffRemaining);
                surfaces.spoilerRatio = (std::max)(0.0f, (std::min)(surfaces.spoilerRatio, 1.0f));
            }

            prev_surface_update_time = now;
        }

        SetGearRatio(surfaces.gearPosition);
        SetFlapRatio(surfaces.flapRatio);
        SetSlatRatio(GetFlapRatio());
        SetSpoilerRatio(surfaces.spoilerRatio);
        SetSpeedbrakeRatio(surfaces.spoilerRatio);

        if (engines_running)
        {
            SetEngineRotRpm(1200);
            SetPropRotRpm(GetEngineRotRpm());
            SetEngineRotAngle(GetEngineRotAngle() + RpmToDegree(GetEngineRotRpm(), _elapsedSinceLastCall));
            while (GetEngineRotAngle() >= 360.0f)
            {
                SetEngineRotAngle(GetEngineRotAngle() - 360.0f);
            }
            SetPropRotAngle(GetEngineRotAngle());
        }
        else
        {
            SetEngineRotRpm(0.0f);
            SetPropRotRpm(0.0f);
            SetEngineRotAngle(0.0f);
            SetPropRotAngle(0.0f);
        }

        SetLightsTaxi(surfaces.lights.taxiLights);
        SetLightsLanding(surfaces.lights.landLights);
        SetLightsBeacon(surfaces.lights.bcnLights);
        SetLightsStrobe(surfaces.lights.strbLights);
        SetLightsNav(surfaces.lights.navLights);

        HexToRgb(Config::Instance().getAircraftLabelColor(), colLabel);

        first_render_pending = false;

        XPLMCameraPosition_t camera;
        XPLMReadCameraPosition(&camera);

        auto& pos = GetLocation();

        vect apos(pos.z, pos.y, pos.x);
        vect user(camera.z, camera.y, camera.x);

        vect diff = apos - user;

        float dist = (diff / diff);
        if (!engines_running) {
            dist = 6000.0f;
        }

        m_position = diff;
        const float networkTime = GetNetworkTime();
        const float d_ts = networkTime - prev_ts;
        m_velocity = vect((pos.z - prev_z) / d_ts, (pos.y - prev_y) / d_ts, (pos.x - prev_x) / d_ts);
        m_dist = dist;

        if (m_engineClass == EngineClass::JetEngine && m_dist < 3000.0f) {
            ALfloat pos[3] = { m_position.x / 50.0f, m_position.y / 50.0f, m_position.z / 50.0f };
            ALfloat vel[3] = { m_velocity.x / 50.0f, m_velocity.y / 50.0f, m_velocity.z / 50.0f };

            alSourcefv(m_soundSource, AL_POSITION, pos);
            alSourcefv(m_soundSource, AL_VELOCITY, vel);

            if ((m_velocity / m_velocity) < 0.0f) {
                alSourcef(m_soundSource, AL_PITCH, 0.8f);
            }

            if (IsVisible()) {
                startSoundThread();
            }
        }
        else if ((m_engineClass == EngineClass::PistonProp || m_engineClass == EngineClass::TurboProp) && m_dist < 3000.0f) {
            ALfloat pos[3] = { m_position.x / 50.0f, m_position.y / 50.0f, m_position.z / 50.0f };
            ALfloat vel[3] = { m_velocity.x / 50.0f, m_velocity.y / 50.0f, m_velocity.z / 50.0f };

            alSourcefv(m_soundSource, AL_POSITION, pos);
            alSourcefv(m_soundSource, AL_VELOCITY, vel);

            if ((m_velocity / m_velocity) < 0.1f) {
                alSourcef(m_soundSource, AL_PITCH, 0.7f);
            }

            if (IsVisible()) {
                startSoundThread();
            }
        }
        else if (m_engineClass == EngineClass::Helicopter && m_dist < 3000.0f) {
            ALfloat pos[3] = { m_position.x / 50.0f, m_position.y / 50.0f, m_position.z / 50.0f };
            ALfloat vel[3] = { m_velocity.x / 50.0f, m_velocity.y / 50.0f, m_velocity.z / 50.0f };

            alSourcefv(m_soundSource, AL_POSITION, pos);
            alSourcefv(m_soundSource, AL_VELOCITY, vel);

            if ((m_velocity / m_velocity) < 0.1f) {
                alSourcef(m_soundSource, AL_PITCH, 0.80f);
            }
            else {
                alSourcef(m_soundSource, AL_PITCH, 0.85f);
            }

            if (IsVisible()) {
                startSoundThread();
            }
        }
        else {
            alSourceStop(m_soundSource);
            alSourceRewind(m_soundSource);
        }
    }

    void NetworkAircraft::copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const
    {
        double lat, lon, alt;
        GetLocation(lat, lon, alt);

        pOut->keyNum = modeS_id;
        pOut->lat = lat;
        pOut->lon = lon;
        pOut->alt_ft = alt;
        pOut->pitch = GetPitch();
        pOut->roll = GetRoll();
        pOut->terrainAlt_ft = (float)ground_altitude.value_or(0.0f);
        pOut->speed_kt = (float)ground_speed;
        pOut->heading = GetHeading();
        pOut->flaps = (float)surfaces.flapRatio;
        pOut->gear = (float)surfaces.gearPosition;
        pOut->bearing = GetCameraBearing();
        pOut->dist_nm = GetCameraDist();
        pOut->bits.taxi = GetLightsTaxi();
        pOut->bits.land = GetLightsLanding();
        pOut->bits.bcn = GetLightsBeacon();
        pOut->bits.strb = GetLightsStrobe();
        pOut->bits.nav = GetLightsNav();
        pOut->bits.onGnd = on_ground;
        pOut->bits.filler1 = 0;
        pOut->bits.multiIdx = GetTcasTargetIdx();
        pOut->bits.filler2 = 0;
        pOut->bits.filler3 = 0;
    }

    void NetworkAircraft::copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const
    {
        pOut->keyNum = modeS_id;
        STRCPY_ATMOST(pOut->callSign, label);
        STRCPY_ATMOST(pOut->modelIcao, acIcaoType);
        STRCPY_ATMOST(pOut->cslModel, GetModelName());
        STRCPY_ATMOST(pOut->acClass, GetModelInfo().doc8643Classification);
        STRCPY_ATMOST(pOut->wtc, GetModelInfo().doc8643WTC);
        if (radar.code > 0 || radar.code <= 9999)
        {
            char s[10];
            snprintf(s, sizeof(s), "%04ld", radar.code);
            STRCPY_ATMOST(pOut->squawk, std::string(s));
        }
        STRCPY_ATMOST(pOut->origin, origin);
        STRCPY_ATMOST(pOut->destination, destination);
    }
}
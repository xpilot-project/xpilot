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
#include <chrono>

namespace xpilot
{
    constexpr double MIN_AGL_FOR_CLIMBOUT = 50.0;
    constexpr double TERRAIN_OFFSET_WINDOW_LANDING = 2.0;
    constexpr double TERRAIN_OFFSET_WINDOW_CLIMBOUT = 10.0;
    constexpr double MIN_TERRAIN_OFFSET_MAGNITUDE = 0.1;

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

    NetworkAircraft::NetworkAircraft(
        const std::string& _callsign,
        const AircraftVisualState& _visualState,
        const std::string& _icaoType,
        const std::string& _icaoAirline,
        const std::string& _livery,
        XPMPPlaneID _modeS_id = 0,
        const std::string& _modelName = "") :
        XPMP2::Aircraft(_icaoType, _icaoAirline, _livery, _modeS_id, _modelName)
    {
        label = _callsign;
        strScpy(acInfoTexts.tailNum, _callsign.c_str(), sizeof(acInfoTexts.tailNum));
        strScpy(acInfoTexts.icaoAcType, acIcaoType.c_str(), sizeof(acInfoTexts.icaoAcType));
        strScpy(acInfoTexts.icaoAirline, acIcaoAirline.c_str(), sizeof(acInfoTexts.icaoAirline));

        SetLocation(_visualState.Lat, _visualState.Lon, _visualState.AltitudeTrue);
        SetHeading(_visualState.Heading);
        SetPitch(_visualState.Pitch);
        SetRoll(_visualState.Bank);

        LastSlowPositionTimestamp = std::chrono::steady_clock::now();
        PredictedVisualState = _visualState;
        RemoteVisualState = _visualState;
        PositionalVelocityVector = Vector3::Zero();
        RotationalVelocityVector = Vector3::Zero();

        auto model = GetModelInfo();
        m_engineClass = EngineClass::JetEngine;
        m_engineCount = 2;

        if (model.doc8643Classification.size() > 0) {

            std::string engineCount(1, model.doc8643Classification[1]);
            try {
                m_engineCount = std::min(std::stoi(engineCount), 2); // limit to 2 engines
            }
            catch (...) { /* catch the exception and default to 2 engines */ }

            // helicopter or gyrocopter
            if (model.doc8643Classification[0] == 'H' || model.doc8643Classification[0] == 'G') {
                m_engineClass = EngineClass::Helicopter;
            }
            // jet
            else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'J') {
                m_engineClass = EngineClass::JetEngine;
            }
            // piston prop
            else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'P') {
                m_engineClass = EngineClass::PistonProp;
            }
            // turbo prop
            else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'T') {
                m_engineClass = EngineClass::TurboProp;
            }
        }

        setEngineState(EngineState::Normal);

        m_velocity = vect(0, 0, 0);
        m_position = vect(0, 0, 0);
    }

    NetworkAircraft::~NetworkAircraft()
    {
        stopSoundThread();

        for (int i = 0; i < m_engineCount; i++) {
            alDeleteSources(1, &m_soundSources[i]);
        }
    }

    void NetworkAircraft::Extrapolate(
        Vector3 velocityVector,
        Vector3 rotationVector,
        double interval)
    {
        double lat_change = MetersToDegrees(velocityVector.Z * interval);
        double new_lat = NormalizeDegrees(PredictedVisualState.Lat + lat_change, -90.0, 90.0);

        double lon_change = MetersToDegrees(velocityVector.X * interval / LongitudeScalingFactor(PredictedVisualState.Lat));
        double new_lon = NormalizeDegrees(PredictedVisualState.Lon + lon_change, -180.0, 180.0);

        double alt_change = velocityVector.Y * interval * 3.28084;
        double new_alt = PredictedVisualState.AltitudeTrue + alt_change;

        AutoLevel(1.0 / interval);
        SetLocation(new_lat, new_lon, AdjustedAltitude.has_value() ? AdjustedAltitude.value() : new_alt);

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

        PredictedVisualState.Lat = new_lat;
        PredictedVisualState.Lon = new_lon;
        PredictedVisualState.AltitudeTrue = new_alt;
        PredictedVisualState.Pitch = new_pitch;
        PredictedVisualState.Bank = new_roll;
        PredictedVisualState.Heading = new_heading;

        SetPitch(new_pitch);
        SetHeading(new_heading);
        SetRoll(new_roll);
    }

    void NetworkAircraft::AutoLevel(float frameRate)
    {
        LocalTerrainElevation = {};
        if (PredictedVisualState.AltitudeTrue < 18000.0)
        {
            LocalTerrainElevation = TerrainProbe.getTerrainElevation(
                PredictedVisualState.Lat,
                PredictedVisualState.Lon
            );
        }

        if (!LocalTerrainElevation.has_value())
        {
            AdjustedAltitude = {};
            return;
        }

        if (!RemoteVisualState.AltitudeAgl.has_value()) {
            if (IsReportedOnGround && (RemoteVisualState.AltitudeTrue - LocalTerrainElevation.value() > LocalTerrainElevation.value())) {
                AdjustedAltitude = LocalTerrainElevation.value() - PredictedVisualState.AltitudeTrue;
                EnsureAboveGround();
                return;
            }
            AdjustedAltitude = {};
            return;
        }

        // Check if we can bail out early.
        if (!HasUsableTerrainElevationData
            && !IsReportedOnGround
            && (TargetTerrainOffset == 0.0)
            && (TerrainOffset == 0.0))
        {
            AdjustedAltitude = PredictedVisualState.AltitudeTrue;
            EnsureAboveGround();
            return;
        }

        double newTargetOffset;
        if (HasUsableTerrainElevationData || IsReportedOnGround) {
            double remoteTerrainElevation = RemoteVisualState.AltitudeTrue - RemoteVisualState.AltitudeAgl.value();
            newTargetOffset = LocalTerrainElevation.value() - remoteTerrainElevation;

            if (RemoteVisualState.AltitudeTrue + newTargetOffset > LocalTerrainElevation.value()) {
                // correct for terrain elevation differences in X-Plane
                newTargetOffset += -LocalTerrainElevation.value() + newTargetOffset;
            }
        }
        else {
            newTargetOffset = 0.0;
        }

        if (newTargetOffset != TargetTerrainOffset) {
            TargetTerrainOffset = newTargetOffset;
            TerrainOffsetMagnitude = abs(TargetTerrainOffset - TerrainOffset);
            TerrainOffsetMagnitude = std::max(TerrainOffsetMagnitude, MIN_TERRAIN_OFFSET_MAGNITUDE);
        }

        if (TerrainOffset != TargetTerrainOffset) {
            if (IsFirstRenderPending) {
                TerrainOffset = TargetTerrainOffset;
            }
            else {
                double window = !IsReportedOnGround
                    && (RemoteVisualState.AltitudeAgl.value() >= MIN_AGL_FOR_CLIMBOUT)
                    && (TargetTerrainOffset == 0.0) ? TERRAIN_OFFSET_WINDOW_CLIMBOUT : TERRAIN_OFFSET_WINDOW_LANDING;
                double step = TerrainOffsetMagnitude / (frameRate * window);
                if (step >= abs(TargetTerrainOffset - TerrainOffset)) {
                    TerrainOffset = TargetTerrainOffset;
                }
                else {
                    TerrainOffset += TargetTerrainOffset > TerrainOffset ? step : -step;
                }
            }
        }

        AdjustedAltitude = PredictedVisualState.AltitudeTrue + TerrainOffset;
        EnsureAboveGround();
    }

    void NetworkAircraft::EnsureAboveGround()
    {
        if (AdjustedAltitude < LocalTerrainElevation.value()) {
            AdjustedAltitude = LocalTerrainElevation.value();
        }
    }

    void NetworkAircraft::UpdateErrorVectors(double interval)
    {
        if (PositionalVelocityVector == Vector3::Zero())
        {
            PositionalVelocityVectorError = Vector3::Zero();
            RotationalVelocityVectorError = Vector3::Zero();
            return;
        }

        double latDelta = DegreesToMeters(CalculateNormalizedDelta(
            PredictedVisualState.Lat,
            RemoteVisualState.Lat,
            -90.0,
            90.0
        ));

        double lonDelta = DegreesToMeters(CalculateNormalizedDelta(
            PredictedVisualState.Lon,
            RemoteVisualState.Lon,
            -180.0,
            180.0
        ));
        lonDelta *= LongitudeScalingFactor(RemoteVisualState.Lat);

        double altDelta = (RemoteVisualState.AltitudeTrue - PredictedVisualState.AltitudeTrue) * 0.3048;

        PositionalVelocityVectorError = Vector3(
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
            DegreesToRadians(RemoteVisualState.Pitch),
            DegreesToRadians(RemoteVisualState.Heading),
            DegreesToRadians(RemoteVisualState.Bank)
        );

        Quaternion delta = Quaternion::Inverse(currentOrientation) * targetOrientation;

        Vector3 result = Quaternion::ToEuler(delta);

        RotationalVelocityVectorError = Vector3(result.X / interval, result.Y / interval, result.Z / interval);
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
        if (m_soundLoaded) {
            return;
        }

        stopSoundThread();

        m_soundThread = std::make_unique<std::thread>([&]()
            {
                audioLoop();
            });
    }

    void NetworkAircraft::stopSoundThread()
    {
        if (m_soundThread) {
            m_soundThread->join();
            m_soundThread.reset();
        }
    }

    void NetworkAircraft::setEngineState(EngineState state)
    {
        ALuint normalSound;
        ALuint starterSound;

        m_currentGain = 0.0f;

        if (m_soundsInitialized) {
            alDeleteSources(m_engineCount, m_soundSources);
        }

        alGenSources(m_engineCount, m_soundSources);

        float zero[3] = { 0,0,0 };
        for (int i = 0; i < m_engineCount; i++) {
            alSourcef(m_soundSources[i], AL_PITCH, m_pitch);
            alSourcei(m_soundSources[i], AL_LOOPING, (state == EngineState::Starter) ? AL_FALSE : AL_TRUE);
            alSourcei(m_soundSources[i], AL_SOURCE_RELATIVE, AL_FALSE);
            alSourcefv(m_soundSources[i], AL_VELOCITY, zero);
            zero[2] = 5.0f;
            alSourcefv(m_soundSources[i], AL_POSITION, zero);
        }

        switch (m_engineClass) {
        case EngineClass::Helicopter:
            normalSound = AircraftSoundManager::get()->helicopter();
            starterSound = AircraftSoundManager::get()->helicopter();
            break;
        case EngineClass::PistonProp:
            normalSound = AircraftSoundManager::get()->pistonProp();
            starterSound = AircraftSoundManager::get()->pistonStarter();
            break;
        case EngineClass::TurboProp:
            normalSound = AircraftSoundManager::get()->turboProp();
            starterSound = AircraftSoundManager::get()->turboStarter();
            break;
        case EngineClass::JetEngine:
        default:
            normalSound = AircraftSoundManager::get()->jetEngine();
            starterSound = AircraftSoundManager::get()->jetStarter();
            break;
        }

        for (int i = 0; i < m_engineCount; i++) {
            if (state == EngineState::Starter && m_engineClass != EngineClass::Helicopter) {
                alSourcei(m_soundSources[i], AL_BUFFER, starterSound);
                m_starterSoundBegan = std::chrono::system_clock::now();
            }
            else {
                alSourcei(m_soundSources[i], AL_BUFFER, normalSound);
            }
        }

        m_soundLoaded = false;
        m_engineState = state;
        m_soundsInitialized = true;
    }

    void NetworkAircraft::stopSounds()
    {
        m_soundLoaded = false;

        for (int i = 0; i < m_engineCount; i++) {
            alSourceStop(m_soundSources[i]);
            alSourceRewind(m_soundSources[i]);
        }
    }

    void NetworkAircraft::audioLoop()
    {
        for (int i = 0; i < m_engineCount; i++) {
            alSourcePlay(m_soundSources[i]);
        }

        m_soundLoaded = true;
        m_soundsPlaying = true;
    }

    void NetworkAircraft::UpdatePosition(float _elapsedSinceLastCall, int)
    {
        Extrapolate(
            PositionalVelocityVector + PositionalVelocityVectorError,
            RotationalVelocityVector + RotationalVelocityVectorError,
            _elapsedSinceLastCall
        );

        const auto now = std::chrono::system_clock::now();
        static const float epsilon = std::numeric_limits<float>::epsilon();
        const auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - PreviousSurfaceUpdateTime);

        TargetGearPosition = IsGearDown || IsReportedOnGround ? 1.0f : 0.0f;
        TargetSpoilerPosition = IsSpoilersDeployed ? 1.0f : 0.0f;
        TargetReverserPosition = IsEnginesReversing ? 1.0f : 0.0f;

        if (FastPositionsReceivedCount == 0)
        {
            // we don't want to wait for the animation on first load...
            // looks particular funny with the gear extending after the aircraft
            // loads on the ground
            Surfaces.gearPosition = TargetGearPosition;
            Surfaces.flapRatio = TargetFlapsPosition;
            Surfaces.spoilerRatio = TargetSpoilerPosition;
        }
        else if (FastPositionsReceivedCount > 1) {
            IsFirstRenderPending = false;
        }

        const float f = Surfaces.gearPosition - TargetGearPosition;
        if (std::abs(f) > epsilon)
        {
            // interpolate gear position
            constexpr float gearMoveTimeMs = 10000;
            const auto gearPositionDiffRemaining = TargetGearPosition - Surfaces.gearPosition;

            const auto gearPositionDiffThisFrame = (diffMs.count()) / gearMoveTimeMs;
            Surfaces.gearPosition += std::copysign(gearPositionDiffThisFrame, gearPositionDiffRemaining);
            Surfaces.gearPosition = (std::max)(0.0f, (std::min)(Surfaces.gearPosition, 1.0f));
        }

        const float f2 = Surfaces.flapRatio - TargetFlapsPosition;
        if (std::abs(f2) > epsilon)
        {
            // interpolate flap position
            constexpr float flapMoveTimeMs = 10000;
            const auto flapPositionDiffRemaining = TargetFlapsPosition - Surfaces.flapRatio;

            const auto flapPositionDiffThisFrame = (diffMs.count()) / flapMoveTimeMs;
            Surfaces.flapRatio += std::copysign(flapPositionDiffThisFrame, flapPositionDiffRemaining);
            Surfaces.flapRatio = (std::max)(0.0f, (std::min)(Surfaces.flapRatio, 1.0f));
        }

        const float f3 = Surfaces.spoilerRatio - TargetSpoilerPosition;
        if (std::abs(f3) > epsilon)
        {
            // interpolate spoiler position
            constexpr float spoilerMoveTimeMs = 2000;
            const auto spoilerPositionDiffRemaining = TargetSpoilerPosition - Surfaces.spoilerRatio;

            const auto spoilerPositionDiffThisFrame = (diffMs.count()) / spoilerMoveTimeMs;
            Surfaces.spoilerRatio += std::copysign(spoilerPositionDiffThisFrame, spoilerPositionDiffRemaining);
            Surfaces.spoilerRatio = (std::max)(0.0f, (std::min)(Surfaces.spoilerRatio, 1.0f));
        }

        const float f4 = Surfaces.tireDeflect - TargetGearDeflection;
        if (std::abs(f4) > epsilon)
        {
            // interpolate gear position
            constexpr float moveTimeMs = 2000;
            const auto diffRemaining = TargetGearDeflection - Surfaces.tireDeflect;

            const auto diffThisFrame = (diffMs.count()) / moveTimeMs;
            Surfaces.tireDeflect += std::copysign(diffThisFrame, diffRemaining);
            Surfaces.tireDeflect = (std::max)(0.0f, (std::min)(Surfaces.tireDeflect, 1.0f));
        }

        const float f5 = Surfaces.reversRatio - TargetReverserPosition;
        if (std::abs(f5) > epsilon)
        {
            // interpolate gear position
            constexpr float moveTimeMs = 3000;
            const auto diffRemaining = TargetReverserPosition - Surfaces.reversRatio;

            const auto diffThisFrame = (diffMs.count()) / moveTimeMs;
            Surfaces.reversRatio += std::copysign(diffThisFrame, diffRemaining);
            Surfaces.reversRatio = (std::max)(0.0f, (std::min)(Surfaces.reversRatio, 1.0f));
        }

        PreviousSurfaceUpdateTime = now;

        SetGearRatio(Surfaces.gearPosition);
        SetFlapRatio(Surfaces.flapRatio);
        SetSlatRatio(GetFlapRatio());
        SetSpoilerRatio(Surfaces.spoilerRatio);
        SetSpeedbrakeRatio(Surfaces.spoilerRatio);
        SetNoseWheelAngle(RemoteVisualState.NoseWheelAngle);

        if (IsReportedOnGround && !WasReportedOnGround) {
            WasReportedOnGround = true;
        }

        if (abs(TerrainOffset) > 0.0f) {
            double v = TerrainOffset / TargetTerrainOffset;
            if (v >= 0.95f) {
                TerrainOffsetFinished = true; // terrain offset is nearly finished; this boolean is used trigger when the aircraft wheels can begin spinning
            }
        }
        else
        {
            TerrainOffsetFinished = false;
        }

        //SetTireDeflection(0.60f);
        SetReversDeployRatio(Surfaces.reversRatio);

        if (IsReportedOnGround || TerrainOffsetFinished)
        {
            double rpm = (60 / (2 * M_PI * 3.2)) * PositionalVelocityVector.X * -1;
            double rpmDeg = RpmToDegree(GetTireRotRpm(), _elapsedSinceLastCall);

            SetTireRotRpm(rpm);
            SetTireRotAngle(GetTireRotAngle() + rpmDeg);
            while (GetTireRotAngle() >= 360.0f)
            {
                SetTireRotAngle(GetTireRotAngle() - 360.0f);
            }
        }

        if (IsEnginesRunning)
        {
            SetEngineRotRpm(1200);
            SetPropRotRpm(GetEngineRotRpm());
            SetEngineRotAngle(GetEngineRotAngle() + RpmToDegree(GetEngineRotRpm(), _elapsedSinceLastCall));
            while (GetEngineRotAngle() >= 360.0f)
            {
                SetEngineRotAngle(GetEngineRotAngle() - 360.0f);
            }
            SetPropRotAngle(GetEngineRotAngle());
            SetThrustRatio(1.0f);
        }
        else
        {
            SetEngineRotRpm(0.0f);
            SetPropRotRpm(0.0f);
            SetEngineRotAngle(0.0f);
            SetPropRotAngle(0.0f);
            SetThrustRatio(0.0f);
        }

        SetLightsTaxi(Surfaces.lights.taxiLights);
        SetLightsLanding(Surfaces.lights.landLights);
        SetLightsBeacon(Surfaces.lights.bcnLights);
        SetLightsStrobe(Surfaces.lights.strbLights);
        SetLightsNav(Surfaces.lights.navLights);
        SetWeightOnWheels(IsReportedOnGround);

        HexToRgb(Config::Instance().getAircraftLabelColor(), colLabel);

        // Sounds

        XPLMCameraPosition_t camera;
        XPLMReadCameraPosition(&camera);

        auto& pos = GetLocation();

        vect apos(pos.x, pos.y, pos.z);
        vect user(camera.x, camera.y, camera.z);

        vect diff = apos - user;

        float dist = (diff / diff);

        m_position = diff;
        const float networkTime = GetNetworkTime();
        const float d_ts = networkTime - prev_ts;
        m_velocity = vect((pos.x - prev_x) / d_ts, (pos.y - prev_y) / d_ts, (pos.z - prev_z) / d_ts);

        constexpr float minDistance = 3000.0f;
        constexpr float positionAdj = 25.0f;

        ALfloat soundPos[3] = { m_position.x / positionAdj, m_position.y / positionAdj, m_position.z / positionAdj };
        ALfloat soundVel[3] = { m_velocity.x / positionAdj, m_velocity.y / positionAdj, m_velocity.z / positionAdj };

        if (!IsEnginesRunning || dist > minDistance) {
            if (m_soundsPlaying) {
                // fade out engine sound when stopping sound
                if (m_currentGain > 0.0f) {
                    auto now = std::chrono::system_clock::now();
                    const auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_previousGainUpdateTime);

                    m_currentGain -= 0.01f;

                    for (int i = 0; i < m_engineCount; i++) {
                        if (m_soundSources[i]) {
                            alSourcef(m_soundSources[i], AL_GAIN, std::max(m_currentGain, 0.0f));
                        }
                    }

                    m_previousGainUpdateTime = now;
                }
                else {
                    stopSounds();
                    m_soundsPlaying = false;
                }
            }
        }
        else {
            if (IsFirstRenderPending) {
                setEngineState(EngineState::Normal);
            }
            else {
                if (IsEnginesRunning != WasEnginesRunning) {
                    if (!WasEnginesRunning && IsEnginesRunning) {
                        setEngineState(EngineState::Starter);
                    }
                    else {
                        setEngineState(EngineState::Normal);
                    }
                }
            }

            float idleGain = 0.80f;
            float normalGain = 1.0f;
            float idlePitch = 0.75f;
            float normalPitch = 1.0f;
            bool isIdle = (m_velocity / m_velocity) < 0.1f;
            float targetGain = isIdle ? idleGain : normalGain;
            float targetPitch = isIdle ? idlePitch : normalPitch;
            m_currentGain = targetGain;

            for (int i = 0; i < m_engineCount; i++) {
                if (m_soundSources[i]) {
                    alSourcefv(m_soundSources[i], AL_POSITION, soundPos);
                    alSourcefv(m_soundSources[i], AL_VELOCITY, soundVel);
                    alSourcef(m_soundSources[i], AL_GAIN, targetGain);
                    alSourcef(m_soundSources[i], AL_PITCH, targetPitch);
                }
            }

            if (m_engineState == EngineState::Starter) {
                const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_starterSoundBegan);
                float starterTime = 0.0f;

                switch (m_engineClass) {
                case EngineClass::JetEngine:
                    starterTime = JetStarterTime;
                    break;
                case EngineClass::PistonProp:
                    starterTime = PistonStarterTime;
                    break;
                case EngineClass::TurboProp:
                    starterTime = TurboStarterTime;
                    break;
                }

                if (elapsed.count() > starterTime) {
                    setEngineState(EngineState::Normal);
                }
            }

            if (IsRendered() && Config::Instance().getEnableAircraftSounds()) {
                startSoundThread();
            }
            else {
                stopSounds();
            }
        }

        WasEnginesRunning = IsEnginesRunning;
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
        pOut->terrainAlt_ft = (float)LocalTerrainElevation.value_or(0.0f);
        pOut->speed_kt = (float)GroundSpeed;
        pOut->heading = GetHeading();
        pOut->flaps = (float)Surfaces.flapRatio;
        pOut->gear = (float)Surfaces.gearPosition;
        pOut->bearing = GetCameraBearing();
        pOut->dist_nm = GetCameraDist();
        pOut->bits.taxi = GetLightsTaxi();
        pOut->bits.land = GetLightsLanding();
        pOut->bits.bcn = GetLightsBeacon();
        pOut->bits.strb = GetLightsStrobe();
        pOut->bits.nav = GetLightsNav();
        pOut->bits.onGnd = IsReportedOnGround;
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
        if (Radar.code > 0 || Radar.code <= 9999)
        {
            char s[10];
            snprintf(s, sizeof(s), "%04ld", Radar.code);
            STRCPY_ATMOST(pOut->squawk, std::string(s));
        }
        STRCPY_ATMOST(pOut->origin, Origin);
        STRCPY_ATMOST(pOut->destination, Destination);
    }
}
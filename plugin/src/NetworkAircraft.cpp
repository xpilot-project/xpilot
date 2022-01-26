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
#include <regex>

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

    inline float RpmToDegree(float rpm, double s)
    {
        return rpm / 60.0f * float(s) * 360.0f;
    }

    void Interpolate(float& value, float& target, float _diffMs, float _moveTime)
    {
        const float f = value - target;
        if (std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            const auto diffRemaining = target - value;
            const auto diffThisFrame = _diffMs / _moveTime;
            value += std::copysign(diffThisFrame, diffRemaining);
            value = (std::max)(0.0f, (std::min)(value, 1.0f));
        }
    }

    template<typename T>
    void InterpolateSurface(T& surface, const float& target, float _diffMs, float _moveTime, float _max = 1.0f)
    {
        const float f = surface - target;
        if (std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            const auto diffRemaining = target - surface;
            const auto diffThisFrame = _diffMs / _moveTime;
            surface += std::copysign(diffThisFrame, diffRemaining);
            surface = (std::max)(0.0f, (std::min)(surface, _max));
        }
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
        pMdl = GetFlightModel(model);

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

        GroundClamping(1.0 / interval);
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

    void NetworkAircraft::GroundClamping(float frameRate)
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

            // correct for terrain elevation differences in X-Plane
            if (RemoteVisualState.AltitudeTrue + newTargetOffset > LocalTerrainElevation.value()) {
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

    FlightModel NetworkAircraft::GetFlightModel(const XPMP2::CSLModelInfo_t model)
    {
        std::string classification = string_format("%s;%s;%s;", model.doc8643WTC, model.doc8643Classification, model.icaoType);
        std::string category = "MediumJets";

        for (const auto& mapIt : FlightModel::modelMatches) {
            std::smatch m;
            std::regex re(mapIt.regex.c_str());
            std::regex_search(classification, m, re);
            if (m.size() > 0) {
                category = mapIt.category;
                break;
            }
        }

        FlightModel flightModel = {};
        flightModel.modelCategory = category;

        if (category == "HugeJets") {
            flightModel.FLAPS_DURATION = 10000;
            flightModel.GEAR_DURATION = 10000;
            flightModel.GEAR_DEFLECTION = 1.4;
        }
        else if (category == "BizJet") {
            flightModel.FLAPS_DURATION = 5000;
            flightModel.GEAR_DURATION = 0.25;
            flightModel.GEAR_DEFLECTION = 0.5;
        }
        else if (category == "GA") {
            flightModel.FLAPS_DURATION = 5000;
            flightModel.GEAR_DURATION = 10000;
            flightModel.GEAR_DEFLECTION = 0.25;
        }
        else if (category == "LightAC") {
            flightModel.FLAPS_DURATION = 5000;
            flightModel.GEAR_DURATION = 10000;
            flightModel.GEAR_DEFLECTION = 0.25;
        }
        else if (category == "Heli") {
            flightModel.FLAPS_DURATION = 5000;
            flightModel.GEAR_DURATION = 10000;
            flightModel.GEAR_DEFLECTION = 0.25;
        }
        else {
            flightModel.FLAPS_DURATION = 5000;
            flightModel.GEAR_DURATION = 10000;
            flightModel.GEAR_DEFLECTION = 0.5;
        }

        return flightModel;
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
            Surfaces.tireDeflect = IsReportedOnGround ? pMdl.GEAR_DEFLECTION / 2.0f : pMdl.GEAR_DEFLECTION;
        }
        else if (FastPositionsReceivedCount > 1) {
            IsFirstRenderPending = false;
        }

        if (IsReportedOnGround) {
            TargetGearDeflection = pMdl.GEAR_DEFLECTION / 2.0f;
        }
        else if (abs(TerrainOffset) > 0.0f) {
            double v = TerrainOffset / TargetTerrainOffset;
            if (v >= 0.95f) {
                TargetGearDeflection = pMdl.GEAR_DEFLECTION / 2.0f;
                TerrainOffsetFinished = true; // terrain offset is nearly finished; this boolean is used trigger when the aircraft wheels can begin spinning
            }
        }
        else {
            TerrainOffsetFinished = false;
        }

        SetLightsTaxi(Surfaces.lights.taxiLights);
        SetLightsLanding(Surfaces.lights.landLights);
        SetLightsBeacon(Surfaces.lights.bcnLights);
        SetLightsStrobe(Surfaces.lights.strbLights);
        SetLightsNav(Surfaces.lights.navLights);

        InterpolateSurface(Surfaces.gearPosition, TargetGearPosition, diffMs.count(), pMdl.GEAR_DURATION);
        InterpolateSurface(Surfaces.flapRatio, TargetFlapsPosition, diffMs.count(), pMdl.FLAPS_DURATION);
        InterpolateSurface(Surfaces.spoilerRatio, TargetSpoilerPosition, diffMs.count(), pMdl.FLAPS_DURATION);
        InterpolateSurface(Surfaces.tireDeflect, TargetGearDeflection, diffMs.count(), 2000, 1.5f);
        InterpolateSurface(Surfaces.reversRatio, TargetReverserPosition, diffMs.count(), 3000);
        PreviousSurfaceUpdateTime = now;

        SetGearRatio(Surfaces.gearPosition);
        SetFlapRatio(Surfaces.flapRatio);
        SetSlatRatio(GetFlapRatio());
        SetSpoilerRatio(Surfaces.spoilerRatio);
        SetSpeedbrakeRatio(Surfaces.spoilerRatio);
        SetTireDeflection(Surfaces.tireDeflect);
        SetReversDeployRatio(Surfaces.reversRatio);
        SetNoseWheelAngle(RemoteVisualState.NoseWheelAngle);
        SetWeightOnWheels(IsReportedOnGround);

        if (IsReportedOnGround || TerrainOffsetFinished)
        {
            double rpm = (60 / (2 * M_PI * 3.2)) * PositionalVelocityVector.X * -1;
            double rpmDeg = RpmToDegree(GetTireRotRpm(), _elapsedSinceLastCall);
            SetTireRotRpm(rpm);
            SetTireRotAngle(GetTireRotAngle() + rpmDeg);
            while (GetTireRotAngle() >= 360.0f)
                SetTireRotAngle(GetTireRotAngle() - 360.0f);
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

            float idleGain = 0.70f;
            float normalGain = 1.0f;
            float idlePitch = 0.70f;
            float normalPitch = 1.0f;
            bool isIdle = (m_velocity / m_velocity) < 1.0f;
            float targetGain = isIdle ? idleGain : normalGain;
            float targetPitch = isIdle ? idlePitch : normalPitch;
            
            Interpolate(m_currentGain, targetGain, diffMs.count(), isIdle ? 5000 : 10000);
            Interpolate(m_currentPitch, targetPitch, diffMs.count(), isIdle ? 5000 : 10000);

            for (int i = 0; i < m_engineCount; i++) {
                if (m_soundSources[i]) {
                    alSourcefv(m_soundSources[i], AL_POSITION, soundPos);
                    alSourcefv(m_soundSources[i], AL_VELOCITY, soundVel);
                    alSourcef(m_soundSources[i], AL_GAIN, m_currentGain);
                    alSourcef(m_soundSources[i], AL_PITCH, m_currentPitch);
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

    void FlightModel::InitializeModels()
    {
        // Huge Jets
        modelMatches.push_back({ "HugeJets","^(H|J);L\\dJ;" });
        modelMatches.push_back({ "HugeJets","^M;L4J;" });

        // Biz Jets
        modelMatches.push_back({ "BizJet","^M;L\\dJ;*BEECH*" });     // Beech, Beechcraft
        modelMatches.push_back({ "BizJet","^M;L\\dJ;GLF" });         // Grumman Gulfstream
        modelMatches.push_back({ "BizJet","^M;L\\dJ;LJ" });          // Learjet
        modelMatches.push_back({ "BizJet","^M;L\\dJ;*BEECH*" });     // Beech, Beechcraft
        modelMatches.push_back({ "BizJet","^M;L\\dJ;.*;CESSNA" });   // Cessna
        modelMatches.push_back({ "BizJet","^M;L\\dJ;.*;DASSAULT" }); // Dassault (Falcon)

        // Medium Jets
        modelMatches.push_back({ "MediumJets","^M;L\\dJ;" });
        modelMatches.push_back({ "MediumProps","^M;L\\dT;" });
        modelMatches.push_back({ "BizJet","^L;L\\dJ;" });
        modelMatches.push_back({ "Glider",";(GLID|A20J|A33P|A33E|A34E|ARCE|ARCP|AS14|AS16|AS20|AS21|AS22|AS24|AS25|AS26|AS28|AS29|AS30|AS31|DG1T|DG40|DG50|DG60|DG80|DIMO|DISC|DUOD|G103|G109|HU1|HU2|JANU|L13M|LAE1|LK17|LK19|LK20|LS8|LS9|NIMB|PISI|PITE|PITA|PIT4|PK15|PK20|S10S|S32M|S32E|SF24|SF25|SF27|SF28|SF31|SZ45|SZ9M|TS1J|VENT);" });
        modelMatches.push_back({ "LightAC", "^-" });
        modelMatches.push_back({ "TurboProps", "^L;L\\dT;" });
        modelMatches.push_back({ "GA", "^L;L\\dP;" });
        modelMatches.push_back({ "Heli", "^.;H" });

        // Fallback
        modelMatches.push_back({ "MediumJets", ".*" });
    }

    std::vector<FlightModelInfo> FlightModel::modelMatches;
}
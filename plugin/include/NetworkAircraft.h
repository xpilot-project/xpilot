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
        double AltitudeTrue;
        std::optional<double> AltitudeAgl;
        double Pitch;
        double Heading;
        double Bank;
        double NoseWheelAngle;
    };

    struct WorldPoint
    {
        double Latitude;
        double Longitude;
    };

    struct TerrainElevationData
    {
        std::chrono::steady_clock::time_point Timestamp;
        WorldPoint Location;
        double RemoteValue;
        double LocalValue;
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

    struct FlightModelInfo
    {
        std::string category;
        std::string regex;
    };

    class FlightModel
    {
    public:
        std::string modelCategory;
        double GEAR_DURATION = 10000;       // [ms] time for gear up/down
        double GEAR_DEFLECTION = 0.5;       // [m]  main gear deflection on meters during touchdown
        double FLAPS_DURATION = 5000;       // [ms] time for full flaps extension from 0% to 100%

    public:
        static void InitializeModels();
        static std::vector<FlightModelInfo> modelMatches;
    };

    class NetworkAircraft : public XPMP2::Aircraft
    {
    public:
        NetworkAircraft(const std::string& _callsign, const AircraftVisualState& _visualState, const std::string& _icaoType, const std::string& _icaoAirline, const std::string& _livery, XPMPPlaneID _modeS_id, const std::string& _modelName);
        virtual ~NetworkAircraft();

        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const;
        void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const;

        void UpdateErrorVectors(double interval);

        FlightModel GetFlightModel(const XPMP2::CSLModelInfo_t model);

        int FastPositionsReceivedCount = 0;
        bool IsFirstRenderPending = true;
        bool IsReportedOnGround = false;
        bool TerrainOffsetFinished = false;
        bool IsGearDown = false;
        bool IsEnginesRunning = false;
        bool WasEnginesRunning = false;
        bool IsEnginesReversing = false;
        bool IsSpoilersDeployed = false;
        float GroundSpeed = 0.0f;
        float TargetReverserPosition = 0.0f;
        float TargetGearPosition = 0.0f;
        float TargetFlapsPosition = 0.0f;
        float TargetSpoilerPosition = 0.0f;
        float TargetGearDeflection = 0.0f;
        std::string Origin;
        std::string Destination;
        std::chrono::system_clock::time_point PreviousSurfaceUpdateTime;
        XPMPPlaneSurfaces_t Surfaces;
        XPMPPlaneRadar_t Radar;

        FlightModel pMdl;

        TerrainProbe LocalTerrainProbe;
        std::optional<double> LocalTerrainElevation = {};
        std::optional<double> AdjustedAltitude = {};
        double TargetTerrainOffset = 0.0;
        double TerrainOffset = 0.0;
        double TerrainOffsetMagnitude = 0.0;
        std::list<TerrainElevationData> TerrainElevationHistory;
        bool HasUsableTerrainElevationData;

        AircraftVisualState RemoteVisualState;
        AircraftVisualState PredictedVisualState;

        Vector3 PositionalVelocityVector;
        Vector3 PositionalVelocityVectorError;
        Vector3 RotationalVelocityVector;
        Vector3 RotationalVelocityVectorError;

        std::chrono::steady_clock::time_point LastFastPositionTimestamp;
        std::chrono::steady_clock::time_point LastSlowPositionTimestamp;

        void stopSounds();

    protected:
        virtual void UpdatePosition(float, int);
        void Extrapolate(Vector3 velocityVector, Vector3 rotationVector, double interval);
        void GroundClamping(float frameRate);
        void EnsureAboveGround();
        static double NormalizeDegrees(double value, double lowerBound, double upperBound);

        ALuint m_soundBuffer = 0;
        ALuint m_soundSources[2];

        float m_pitch = 1.0f;
        float m_gain = 1.0f;
        float m_currentGain = 0.0f;
        float m_currentPitch = 0.0f;
        bool m_soundLoaded = false;
        bool m_soundsPlaying = false;
        bool m_soundsInitialized = false;

        void audioLoop();
        void startSoundThread();
        void stopSoundThread();
        void setEngineState(EngineState state);
        std::unique_ptr<std::thread> m_soundThread;
        std::chrono::system_clock::time_point m_previousGainUpdateTime;
        std::chrono::system_clock::time_point m_starterSoundBegan;

        // length (in seconds) of engine starter sounds, used for transition between starter and engine sound
        const float PistonStarterTime = 1.5f;
        const float JetStarterTime = 18.5f;
        const float TurboStarterTime = 12.5f;

        EngineClass m_engineClass;
        EngineState m_engineState;
        int m_engineCount;
        vect m_velocity;
        vect m_position;
    };
}

#endif // !NetworkAircraft_h



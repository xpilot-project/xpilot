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

        FlightModel flightModel;

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

        int soundChannelId;

        vect SoundVelocity() const
        {
            return mSoundVelocity;
        }

        vect SoundPosition() const
        {
            return mSoundPosition;
        }

        EngineClass GetEngineClass() const
        {
            return mEngineClass;
        }

    protected:
        virtual void UpdatePosition(float, int);
        void Extrapolate(Vector3 velocityVector, Vector3 rotationVector, double interval);
        void GroundClamping(float frameRate);
        void EnsureAboveGround();

        EngineClass mEngineClass;
        vect mSoundVelocity;
        vect mSoundPosition;
    };
}

#endif // !NetworkAircraft_h



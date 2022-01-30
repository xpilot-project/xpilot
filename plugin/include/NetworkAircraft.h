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

#ifndef NetworkAircraft_h
#define NetworkAircraft_h

#include "XPilotAPI.h"
#include "TerrainProbe.h"
#include "Utilities.h"
#include "Vector3.hpp"
#include "XPCAircraft.h"
#include "XPMPAircraft.h"
#include "XPMPMultiplayer.h"

#include <deque>
#include <optional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

using namespace std;

namespace xpilot
{
    struct AircraftVisualState
    {
        double Lat;
        double Lon;
        double AltitudeTrue;
        optional<double> AltitudeAgl;
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
        chrono::steady_clock::time_point Timestamp;
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

    struct FlightModelInfo
    {
        string category;
        string regex;
    };

    class FlightModel
    {
    public:
        string modelCategory;
        double GEAR_DURATION = 10000;       // [ms] time for gear up/down
        double GEAR_DEFLECTION = 0.5;       // [m]  main gear deflection on meters during touchdown
        double FLAPS_DURATION = 5000;       // [ms] time for full flaps extension from 0% to 100%

    public:
        static void InitializeModels();
        static vector<FlightModelInfo> modelMatches;
    };

    class NetworkAircraft : public XPMP2::Aircraft
    {
    public:
        NetworkAircraft(const string& _callsign, const AircraftVisualState& _visualState, const string& _icaoType, const string& _icaoAirline, const string& _livery, XPMPPlaneID _modeS_id, const string& _modelName);
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
        string Origin;
        string Destination;
        chrono::system_clock::time_point PreviousSurfaceUpdateTime;
        XPMPPlaneSurfaces_t Surfaces;
        XPMPPlaneRadar_t Radar;

        FlightModel flightModel;

        TerrainProbe LocalTerrainProbe;
        optional<double> LocalTerrainElevation = {};
        optional<double> AdjustedAltitude = {};
        double TargetTerrainOffset = 0.0;
        double TerrainOffset = 0.0;
        double TerrainOffsetMagnitude = 0.0;
        list<TerrainElevationData> TerrainElevationHistory;
        bool HasUsableTerrainElevationData;

        AircraftVisualState RemoteVisualState;
        AircraftVisualState PredictedVisualState;

        Vector3 PositionalVelocityVector;
        Vector3 PositionalVelocityVectorError;
        Vector3 RotationalVelocityVector;
        Vector3 RotationalVelocityVectorError;

        chrono::steady_clock::time_point LastFastPositionTimestamp;
        chrono::steady_clock::time_point LastSlowPositionTimestamp;

        int SoundChannelId;

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



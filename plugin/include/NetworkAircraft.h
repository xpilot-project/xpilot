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

namespace xpilot {
	struct AircraftVisualState
	{
		double Lat;
		double Lon;
		double AltitudeTrue;
		std::optional<double> AltitudeAgl = {};
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
		int64_t Timestamp;
		WorldPoint Location;
		double RemoteValue;
		double LocalValue;
	};

	enum class EngineClassType
	{
		Helicopter,
		PistonProp,
		TurboProp,
		JetEngine,
		Unknown
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

	constexpr long TERRAIN_ELEVATION_DATA_USABLE_AGE = 2000;
	constexpr double MAX_USABLE_ALTITUDE_AGL = 100.0;
	constexpr double TERRAIN_ELEVATION_MAX_SLOPE = 3.0;

	class NetworkAircraft : public XPMP2::Aircraft
	{
	public:
		NetworkAircraft(const std::string& _callsign, const AircraftVisualState& _visualState, const std::string& _icaoType,
			const std::string& _icaoAirline, const std::string& _livery, XPMPPlaneID _modeS_id, const std::string& _modelName);

		void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const;
		void copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const;

		void UpdateErrorVectors(double currentTimestamp);
		void RecordTerrainElevationHistory(double currentTimestamp);
		void UpdateVelocityVectors();

		float GetLift() const override;

		FlightModel GetFlightModel(const XPMP2::CSLModelInfo_t model);

		bool IsFirstRenderPending = true;
		bool IsReportedOnGround = false;
		bool IsGearDown = false;
		bool IsEnginesRunning = false;
		bool IsEnginesReversing = false;
		bool IsSpoilersDeployed = false;
		float GroundSpeed = 0.0f;
		float TargetReverserPosition = 0.0f;
		float TargetGearPosition = 0.0f;
		float TargetFlapsPosition = 0.0f;
		float TargetSpoilerPosition = 0.0f;
		std::string Origin;
		std::string Destination;
		int64_t PreviousSurfaceUpdateTime;
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

		int64_t LastUpdated;
		AircraftVisualState VisualState;
		AircraftVisualState PredictedVisualState;

		Vector3 PositionalVelocities;
		Vector3 PositionalErrorVelocities;
		Vector3 RotationalVelocities;
		Vector3 RotationalErrorVelocities;
		int64_t ApplyErrorVelocitiesUntil;
		int64_t LastVelocityUpdate;

		int SoundChannelId;
		EngineClassType EngineClass;

	protected:
		virtual void UpdatePosition(float, int) override;
		AircraftVisualState ExtrapolatePosition(Vector3 velocityVector, Vector3 rotationVector, double interval);
		void PerformGroundClamping(float frameRate);
		void EnsureAboveGround();
		void ClearRotationalVelocities();
	};
}

#endif // !NetworkAircraft_h



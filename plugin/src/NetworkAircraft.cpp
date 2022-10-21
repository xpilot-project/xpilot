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

#include "NetworkAircraft.h"
#include "Utilities.h"
#include "Config.h"
#include "GeoCalc.hpp"
#include "Quaternion.hpp"
#include <chrono>
#include <regex>
#include <cassert>

namespace xpilot {
	constexpr double MIN_AGL_FOR_CLIMBOUT = 50.0;
	constexpr double TERRAIN_OFFSET_WINDOW_LANDING = 2.0;
	constexpr double TERRAIN_OFFSET_WINDOW_CLIMBOUT = 10.0;
	constexpr double MIN_TERRAIN_OFFSET_MAGNITUDE = 0.1;

	double CalculateNormalizedDelta(double start, double end, double lowerBound, double upperBound) {
		double range = upperBound - lowerBound;
		double halfRange = range / 2.0;

		if (abs(end - start) > halfRange) {
			end += (end > start ? -range : range);
		}

		return end - start;
	}

	double NormalizeDegrees(double value, double lowerBound, double upperBound) {
		double range = upperBound - lowerBound;
		if (value < lowerBound) {
			return value + range;
		}
		if (value > upperBound) {
			return value - range;
		}
		return value;
	}

	float RpmToDegree(float rpm, double s) {
		return rpm / 60.0f * float(s) * 360.0f;
	}

	template<typename T>
	void Interpolate(T& surface, const float& target, float _diffMs, float _moveTime, float _max = 1.0f) {
		const float f = surface - target;
		if (abs(f) > std::numeric_limits<float>::epsilon()) {
			const auto diffRemaining = target - surface;
			const auto diffThisFrame = _diffMs / _moveTime;
			surface += copysign(diffThisFrame, diffRemaining);
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
		XPMP2::Aircraft(_callsign, _icaoType, _icaoAirline, _livery, _modeS_id, _modelName) {
		strScpy(acInfoTexts.tailNum, _callsign.c_str(), sizeof(acInfoTexts.tailNum));
		strScpy(acInfoTexts.icaoAcType, acIcaoType.c_str(), sizeof(acInfoTexts.icaoAcType));
		strScpy(acInfoTexts.icaoAirline, acIcaoAirline.c_str(), sizeof(acInfoTexts.icaoAirline));

		SetVisible(true);
		SetLocation(_visualState.Lat, _visualState.Lon, _visualState.AltitudeTrue);
		SetHeading(_visualState.Heading);
		SetPitch(_visualState.Pitch);
		SetRoll(_visualState.Bank);

		IsFirstRenderPending = true;
		LastVelocityUpdate = PrecisionTimestamp();
		LastUpdated = PrecisionTimestamp();
		PredictedVisualState = _visualState;
		VisualState = _visualState;
		PositionalVelocities = Vector3::Zero();
		RotationalVelocities = Vector3::Zero();

		auto model = GetModelInfo();
		flightModel = GetFlightModel(model);

		EngineClass = EngineClassType::JetEngine;
		if (model.doc8643Classification.size() > 0) {
			// helicopter or gyrocopter
			if (model.doc8643Classification[0] == 'H' || model.doc8643Classification[0] == 'G') {
				EngineClass = EngineClassType::Helicopter;
			}
			// jet
			else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'J') {
				EngineClass = EngineClassType::JetEngine;
			}
			// piston prop
			else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'P') {
				EngineClass = EngineClassType::PistonProp;
			}
			// turbo prop
			else if (model.doc8643Classification.size() >= 2 && model.doc8643Classification[2] == 'T') {
				EngineClass = EngineClassType::TurboProp;
			}
		}
	}

	AircraftVisualState NetworkAircraft::ExtrapolatePosition(
		Vector3 velocityVector,
		Vector3 rotationVector,
		double interval) {
		double lat_change = MetersToDegrees(velocityVector.Z * interval);
		double new_lat = NormalizeDegrees(PredictedVisualState.Lat + lat_change, -90.0, 90.0);

		double lon_change = MetersToDegrees(velocityVector.X * interval / LongitudeScalingFactor(PredictedVisualState.Lat));
		double new_lon = NormalizeDegrees(PredictedVisualState.Lon + lon_change, -180.0, 180.0);

		double alt_change = velocityVector.Y * interval * 3.28084;
		double new_alt = PredictedVisualState.AltitudeTrue + alt_change;

		double pitch;
		double bank;
		double heading;

		if (rotationVector == Vector3::Zero()) {
			pitch = PredictedVisualState.Pitch;
			bank = PredictedVisualState.Bank;
			heading = PredictedVisualState.Heading;
		} else {
			Quaternion current_orientation = Quaternion::FromEuler(
				DegreesToRadians(PredictedVisualState.Pitch),
				DegreesToRadians(PredictedVisualState.Heading),
				DegreesToRadians(PredictedVisualState.Bank)
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

			pitch = RadiansToDegrees(new_orientation.X);
			heading = RadiansToDegrees(new_orientation.Y);
			bank = RadiansToDegrees(new_orientation.Z);
		}

		AircraftVisualState predictedVisualState{};
		predictedVisualState.Lat = new_lat;
		predictedVisualState.Lon = new_lon;
		predictedVisualState.AltitudeTrue = new_alt;
		predictedVisualState.Pitch = pitch;
		predictedVisualState.Bank = bank;
		predictedVisualState.Heading = heading;

		return predictedVisualState;
	}

	void NetworkAircraft::RecordTerrainElevationHistory(double currentTimestamp) {
		if (!LocalTerrainElevation.has_value())
			return;

		HasUsableTerrainElevationData = false;

		TerrainElevationHistory.remove_if([&](TerrainElevationData& meta) {
			return meta.Timestamp < (currentTimestamp - TERRAIN_ELEVATION_DATA_USABLE_AGE + 250);
		});

		if (VisualState.AltitudeAgl.has_value() && (VisualState.AltitudeAgl.value() <= MAX_USABLE_ALTITUDE_AGL)) {
			TerrainElevationData data{};
			data.Timestamp = currentTimestamp;
			data.Location.Latitude = VisualState.Lat;
			data.Location.Longitude = VisualState.Lon;
			data.LocalValue = LocalTerrainElevation.value();
			TerrainElevationHistory.push_back(data);
		} else {
			return;
		}

		if (TerrainElevationHistory.size() < 2) {
			return;
		}

		auto startSample = TerrainElevationHistory.front();
		auto endSample = TerrainElevationHistory.back();
		if ((endSample.Timestamp - startSample.Timestamp) < TERRAIN_ELEVATION_DATA_USABLE_AGE) {
			return;
		}

		double distance = DegreesToFeet(GreatCircleDistance(
			startSample.Location.Longitude, startSample.Location.Latitude,
			endSample.Location.Longitude, endSample.Location.Latitude));
		double remoteElevationDelta = abs(startSample.RemoteValue - endSample.RemoteValue);
		double localElevationDelta = abs(startSample.LocalValue - endSample.LocalValue);
		double remoteSlope = RadiansToDegrees(atan(remoteElevationDelta / distance));
		if (remoteSlope > TERRAIN_ELEVATION_MAX_SLOPE) {
			return;
		}
		double localSlope = RadiansToDegrees(atan(localElevationDelta / distance));
		if (localSlope > TERRAIN_ELEVATION_MAX_SLOPE) {
			return;
		}

		HasUsableTerrainElevationData = true;
	}

	void NetworkAircraft::UpdateVelocityVectors() {
		auto currentTimestamp = PrecisionTimestamp();
		if (currentTimestamp - LastVelocityUpdate > 500) {
			ClearRotationalVelocities();
		}
		LastVelocityUpdate = currentTimestamp;
		RecordTerrainElevationHistory(currentTimestamp);
		UpdateErrorVectors(currentTimestamp);
	}

	void NetworkAircraft::PerformGroundClamping(float frameRate) {
		LocalTerrainElevation = {};
		if (PredictedVisualState.AltitudeTrue < 18000.0) {
			LocalTerrainElevation = LocalTerrainProbe.GetTerrainElevation(
				PredictedVisualState.Lat,
				PredictedVisualState.Lon
			);
		}

		AdjustedAltitude = {};
		if (!LocalTerrainElevation.has_value())
			return;

		double agl;
		if (VisualState.AltitudeAgl.has_value()) {
			agl = VisualState.AltitudeAgl.value();
		} else {
			agl = PredictedVisualState.AltitudeTrue - LocalTerrainElevation.value();
		}

		// Check if we can bail out early.
		if (!HasUsableTerrainElevationData
			&& !IsReportedOnGround
			&& (TargetTerrainOffset == 0.0)
			&& (TerrainOffset == 0.0)) {
			AdjustedAltitude = PredictedVisualState.AltitudeTrue;
			EnsureAboveGround();
			return;
		}

		double newTargetOffset;
		if (HasUsableTerrainElevationData || IsReportedOnGround) {
			double remoteTerrainElevation = VisualState.AltitudeTrue - agl;
			newTargetOffset = Round(LocalTerrainElevation.value() - remoteTerrainElevation, 2);

			// correct for terrain elevation differences in X-Plane
			if (IsReportedOnGround && (VisualState.AltitudeTrue + newTargetOffset > LocalTerrainElevation.value())) {
				double adj = LocalTerrainElevation.value() - (VisualState.AltitudeTrue + newTargetOffset);
				newTargetOffset += adj;
			}
		} else {
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
			} else {
				double window = !IsReportedOnGround
					&& (agl >= MIN_AGL_FOR_CLIMBOUT)
					&& (TargetTerrainOffset == 0.0) ? TERRAIN_OFFSET_WINDOW_CLIMBOUT : TERRAIN_OFFSET_WINDOW_LANDING;
				double step = TerrainOffsetMagnitude / (frameRate * window);
				if (step >= abs(TargetTerrainOffset - TerrainOffset)) {
					TerrainOffset = TargetTerrainOffset;
				} else {
					TerrainOffset += TargetTerrainOffset > TerrainOffset ? step : -step;
				}
			}
		}

		AdjustedAltitude = PredictedVisualState.AltitudeTrue + TerrainOffset;
		EnsureAboveGround();
	}

	void NetworkAircraft::EnsureAboveGround() {
		if (AdjustedAltitude < LocalTerrainElevation.value()) {
			AdjustedAltitude = LocalTerrainElevation.value();
		}
	}

	void NetworkAircraft::ClearRotationalVelocities() {
		RotationalVelocities = Vector3::Zero();
		RotationalErrorVelocities = Vector3::Zero();
		PredictedVisualState.Pitch = VisualState.Pitch;
		PredictedVisualState.Bank = VisualState.Bank;
		PredictedVisualState.Heading = VisualState.Heading;
	}

	void NetworkAircraft::UpdateErrorVectors(double currentTimestamp) {
		double latDelta = DegreesToMeters(CalculateNormalizedDelta(
			PredictedVisualState.Lat,
			VisualState.Lat,
			-90.0,
			90.0
		));

		double lonDelta = DegreesToMeters(CalculateNormalizedDelta(
			PredictedVisualState.Lon,
			VisualState.Lon,
			-180.0,
			180.0
		));
		lonDelta *= LongitudeScalingFactor(VisualState.Lat);

		double altDelta = (VisualState.AltitudeTrue - PredictedVisualState.AltitudeTrue) * 0.3048;

		PositionalErrorVelocities = Vector3(
			lonDelta / 2.0,
			altDelta / 2.0,
			latDelta / 2.0
		);

		if (PredictedVisualState.Pitch == VisualState.Pitch &&
			PredictedVisualState.Heading == VisualState.Heading &&
			PredictedVisualState.Bank == VisualState.Bank) {
			RotationalErrorVelocities = Vector3::Zero();
		} else {
			Quaternion currentOrientation = Quaternion::FromEuler(
				DegreesToRadians(PredictedVisualState.Pitch),
				DegreesToRadians(PredictedVisualState.Heading),
				DegreesToRadians(PredictedVisualState.Bank)
			);

			Quaternion targetOrientation = Quaternion::FromEuler(
				DegreesToRadians(VisualState.Pitch),
				DegreesToRadians(VisualState.Heading),
				DegreesToRadians(VisualState.Bank)
			);

			Quaternion delta = Quaternion::Inverse(currentOrientation) * targetOrientation;

			Vector3 result = Quaternion::ToEuler(delta);

			RotationalErrorVelocities = Vector3(result.X / 2.0, result.Y / 2.0, result.Z / 2.0);
		}

		ApplyErrorVelocitiesUntil = currentTimestamp + 2000;
	}

	void NetworkAircraft::UpdatePosition(float _frameRatePeriod, int) {
		auto currentTimestamp = PrecisionTimestamp();

		if (IsFirstRenderPending) {
			PredictedVisualState = VisualState;
			PositionalErrorVelocities = Vector3::Zero();
			RotationalErrorVelocities = Vector3::Zero();
		} else {
			if (currentTimestamp - LastVelocityUpdate > 500) {
				ClearRotationalVelocities();
			}

			Vector3 positionalVelocities = PositionalVelocities;
			Vector3 rotationalVelocities = RotationalVelocities;

			if (currentTimestamp <= ApplyErrorVelocitiesUntil) {
				positionalVelocities += PositionalErrorVelocities;
				rotationalVelocities += RotationalErrorVelocities;
			}

			PredictedVisualState = ExtrapolatePosition(positionalVelocities, rotationalVelocities, _frameRatePeriod);
		}

		PerformGroundClamping(1.0 / _frameRatePeriod);

		SetLocation(PredictedVisualState.Lat, PredictedVisualState.Lon, AdjustedAltitude.has_value() ? AdjustedAltitude.value() : PredictedVisualState.AltitudeTrue);
		SetPitch(PredictedVisualState.Pitch);
		SetRoll(PredictedVisualState.Bank);
		SetHeading(PredictedVisualState.Heading);

		TargetGearPosition = IsGearDown || IsReportedOnGround ? 1.0f : 0.0f;
		TargetSpoilerPosition = IsSpoilersDeployed ? 1.0f : 0.0f;
		TargetReverserPosition = IsEnginesReversing ? 1.0f : 0.0f;

		if (IsFirstRenderPending) {
			// Set initial aircraft surfaces
			Surfaces.gearPosition = TargetGearPosition;
			Surfaces.flapRatio = TargetFlapsPosition;
			Surfaces.spoilerRatio = TargetSpoilerPosition;
		} else {
			const auto diffMs = currentTimestamp - PreviousSurfaceUpdateTime;
			Interpolate(Surfaces.gearPosition, TargetGearPosition, diffMs, flightModel.GEAR_DURATION);
			Interpolate(Surfaces.flapRatio, TargetFlapsPosition, diffMs, flightModel.FLAPS_DURATION);
			Interpolate(Surfaces.spoilerRatio, TargetSpoilerPosition, diffMs, flightModel.FLAPS_DURATION);
			Interpolate(Surfaces.reversRatio, TargetReverserPosition, diffMs, 1500);
			PreviousSurfaceUpdateTime = currentTimestamp;
		}

		SetLightsTaxi(Surfaces.lights.taxiLights);
		SetLightsLanding(Surfaces.lights.landLights);
		SetLightsBeacon(Surfaces.lights.bcnLights);
		SetLightsStrobe(Surfaces.lights.strbLights);
		SetLightsNav(Surfaces.lights.navLights);

		SetGearRatio(Surfaces.gearPosition);
		SetFlapRatio(Surfaces.flapRatio);
		SetSlatRatio(GetFlapRatio());
		SetSpoilerRatio(Surfaces.spoilerRatio);
		SetSpeedbrakeRatio(Surfaces.spoilerRatio);
		SetTireDeflection(flightModel.GEAR_DEFLECTION / 2.0f);
		SetReversDeployRatio(Surfaces.reversRatio);
		SetNoseWheelAngle(VisualState.NoseWheelAngle);
		SetWeightOnWheels(IsReportedOnGround);

		if (IsReportedOnGround) {
			double rpm = (60 / (2 * M_PI * 3.2)) * abs(PositionalVelocities.X);
			double rpmDeg = RpmToDegree(GetTireRotRpm(), _frameRatePeriod);
			SetTireRotRpm(rpm);
			SetTireRotAngle(GetTireRotAngle() + rpmDeg);
			while (GetTireRotAngle() >= 360.0f)
				SetTireRotAngle(GetTireRotAngle() - 360.0f);
		}

		if (IsEnginesRunning) {
			SetEngineRotRpm(1200);
			SetPropRotRpm(GetEngineRotRpm());
			SetEngineRotAngle(GetEngineRotAngle() + RpmToDegree(GetEngineRotRpm(), _frameRatePeriod));
			while (GetEngineRotAngle() >= 360.0f) {
				SetEngineRotAngle(GetEngineRotAngle() - 360.0f);
			}
			SetPropRotAngle(GetEngineRotAngle());
			SetThrustRatio(1.0f);
		} else {
			SetEngineRotRpm(0.0f);
			SetPropRotRpm(0.0f);
			SetEngineRotAngle(0.0f);
			SetPropRotAngle(0.0f);
			SetThrustRatio(0.0f);
		}

		HexToRgb(Config::GetInstance().GetAircraftLabelColor(), colLabel);

		IsFirstRenderPending = false;
	}

	void NetworkAircraft::copyBulkData(XPilotAPIAircraft::XPilotAPIBulkData* pOut, size_t size) const {
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

	void NetworkAircraft::copyBulkData(XPilotAPIAircraft::XPilotAPIBulkInfoTexts* pOut, size_t size) const {
		pOut->keyNum = modeS_id;
		STRCPY_ATMOST(pOut->callSign, label);
		STRCPY_ATMOST(pOut->modelIcao, acIcaoType);
		STRCPY_ATMOST(pOut->cslModel, GetModelName());
		STRCPY_ATMOST(pOut->acClass, GetModelInfo().doc8643Classification);
		STRCPY_ATMOST(pOut->wtc, GetModelInfo().doc8643WTC);
		if (Radar.code > 0 || Radar.code <= 9999) {
			char s[10];
			snprintf(s, sizeof(s), "%04ld", Radar.code);
			STRCPY_ATMOST(pOut->squawk, std::string(s));
		}
		STRCPY_ATMOST(pOut->origin, Origin);
		STRCPY_ATMOST(pOut->destination, Destination);
	}

	void FlightModel::InitializeModels() {
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

	float NetworkAircraft::GetLift() const
	{
		// No wake if the aircraft is reported on the ground
		return IsReportedOnGround ? 0.0f : GetMass() * XPMP2::G_EARTH;
	}

	FlightModel NetworkAircraft::GetFlightModel(const XPMP2::CSLModelInfo_t model) {
		std::string classification = string_format("%s;%s;%s;", model.doc8643WTC.c_str(), model.doc8643Classification.c_str(), model.icaoType.c_str());
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
		} else if (category == "BizJet") {
			flightModel.FLAPS_DURATION = 5000;
			flightModel.GEAR_DURATION = 0.25;
			flightModel.GEAR_DEFLECTION = 0.5;
		} else if (category == "GA") {
			flightModel.FLAPS_DURATION = 5000;
			flightModel.GEAR_DURATION = 10000;
			flightModel.GEAR_DEFLECTION = 0.25;
		} else if (category == "LightAC") {
			flightModel.FLAPS_DURATION = 5000;
			flightModel.GEAR_DURATION = 10000;
			flightModel.GEAR_DEFLECTION = 0.25;
		} else if (category == "Heli") {
			flightModel.FLAPS_DURATION = 5000;
			flightModel.GEAR_DURATION = 10000;
			flightModel.GEAR_DEFLECTION = 0.25;
		} else {
			flightModel.FLAPS_DURATION = 5000;
			flightModel.GEAR_DURATION = 10000;
			flightModel.GEAR_DEFLECTION = 0.5;
		}

		return flightModel;
	}

	std::vector<FlightModelInfo> FlightModel::modelMatches;
}
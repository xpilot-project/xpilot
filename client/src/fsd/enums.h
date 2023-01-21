/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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

#ifndef ENUMS_H
#define ENUMS_H

enum class NetworkFacility
{
    OBS,
    FSS,
    DEL,
    GND,
    TWR,
    APP,
    CTR
};

enum class NetworkRating
{
    OBS = 1,
    S1,
    S2,
    S3,
    C1,
    C2,
    C3,
    I1,
    I2,
    I3,
    SUP,
    ADM
};

enum class SimulatorType
{
    Unknown,
    MSFS95,
    MSFS98,
    MSCFS,
    AS2,
    PS1,
    XPlane
};

enum class ProtocolRevision
{
    Unknown = 0,
    Classic = 9,
    VatsimNoAuth = 10,
    VatsimAuth = 100,
    Vatsim2022 = 101
};

enum class ClientQueryType
{
    Unknown,
    IsValidATC,
    Capabilities,
    COM1Freq,
    RealName,
    Server,
    ATIS,
    PublicIP,
    INF,
    FlightPlan,
    IPC,
    RequestRelief,
    CancelRequestRelief,
    RequestHelp,
    CancelRequestHelp,
    WhoHas,
    InitiateTrack,
    AcceptHandoff,
    DropTrack,
    SetFinalAltitude,
    SetTempAltitude,
    SetBeaconCode,
    SetScratchpad,
    SetVoiceType,
    AircraftConfiguration,
    NewInfo,
    NewATIS,
    Estimate,
    SetGlobalData
};

enum class FlightRules
{
    Unknown,
    IFR,
    VFR,
    DVFR,
    SVFR
};

enum class NetworkError
{
    Ok,
    CallsignInUse,
    CallsignInvalid,
    AlreadyRegistered,
    SyntaxError,
    PDUSourceInvalid,
    InvalidLogon,
    NoSuchCallsign,
    NoFlightPlan,
    NoWeatherProfile,
    InvalidProtocolRevision,
    RequestedLevelTooHigh,
    ServerFull,
    CertificateSuspended,
    InvalidControl,
    InvalidPositionForRating,
    UnauthorizedSoftware
};

#endif

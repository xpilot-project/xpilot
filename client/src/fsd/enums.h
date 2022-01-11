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

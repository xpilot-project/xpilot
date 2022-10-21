#include "serializer.h"

template<>
QString toQString(const NetworkRating &value)
{
    switch(value)
    {
        case NetworkRating::OBS:
            return "1";
        case NetworkRating::S1:
            return "2";
        case NetworkRating::S2:
            return "3";
        case NetworkRating::S3:
            return "4";
        case NetworkRating::C1:
            return "5";
        case NetworkRating::C2:
            return "6";
        case NetworkRating::C3:
            return "7";
        case NetworkRating::I1:
            return "8";
        case NetworkRating::I2:
            return "9";
        case NetworkRating::I3:
            return "10";
        case NetworkRating::SUP:
            return "11";
        case NetworkRating::ADM:
            return "12";
        default:
            return "1";
    }
}

template<>
NetworkRating fromQString(const QString &str)
{
    if(str.isEmpty()) return NetworkRating::OBS;

    if(str == "1")
    {
        return NetworkRating::OBS;
    }
    else if(str == "2")
    {
        return NetworkRating::S1;
    }
    else if(str == "3")
    {
        return NetworkRating::S2;
    }
    else if(str == "4")
    {
        return NetworkRating::S3;
    }
    else if(str == "5")
    {
        return NetworkRating::C1;
    }
    else if(str == "6")
    {
        return NetworkRating::C2;
    }
    else if(str == "7")
    {
        return NetworkRating::C3;
    }
    else if(str == "8")
    {
        return NetworkRating::I1;
    }
    else if(str == "9")
    {
        return NetworkRating::I2;
    }
    else if(str == "10")
    {
        return NetworkRating::I3;
    }
    else if(str == "11")
    {
        return NetworkRating::SUP;
    }
    else if(str == "12")
    {
        return NetworkRating::ADM;
    }

    return NetworkRating::OBS;
}

template<>
QString toQString(const NetworkFacility &value)
{
    switch(value)
    {
        case NetworkFacility::OBS:
            return "0";
        case NetworkFacility::FSS:
            return "1";
        case NetworkFacility::DEL:
            return "2";
        case NetworkFacility::GND:
            return "3";
        case NetworkFacility::TWR:
            return "4";
        case NetworkFacility::APP:
            return "5";
        case NetworkFacility::CTR:
            return "6";
        default:
            return "0";
    }
}

template<>
NetworkFacility fromQString(const QString &str)
{
    if(str.isEmpty()) return NetworkFacility::OBS;

    if(str == "0")
    {
        return NetworkFacility::OBS;
    }
    else if(str == "1")
    {
        return NetworkFacility::FSS;
    }
    else if(str == "2")
    {
        return NetworkFacility::DEL;
    }
    else if(str == "3")
    {
        return NetworkFacility::GND;
    }
    else if(str == "4")
    {
        return NetworkFacility::TWR;
    }
    else if(str == "5")
    {
        return NetworkFacility::APP;
    }
    else if(str == "6")
    {
        return NetworkFacility::CTR;
    }

    return NetworkFacility::OBS;
}

template<>
QString toQString(const ProtocolRevision &value)
{
    switch(value)
    {
        case ProtocolRevision::Classic:
            return "9";
        case ProtocolRevision::VatsimNoAuth:
            return "10";
        case ProtocolRevision::VatsimAuth:
            return "100";
        case ProtocolRevision::Vatsim2022:
            return "101";
        default:
            return "0";
    }
}

template<>
ProtocolRevision fromQString(const QString &str)
{
    if(str.isEmpty()) return ProtocolRevision::Unknown;

    if(str == "0")
    {
        return ProtocolRevision::Unknown;
    }
    else if(str == "9")
    {
        return ProtocolRevision::Classic;
    }
    else if(str == "10")
    {
        return ProtocolRevision::VatsimNoAuth;
    }
    else if(str == "100")
    {
        return ProtocolRevision::VatsimAuth;
    }
    else
    {
        return ProtocolRevision::Unknown;
    }
}

template<>
QString toQString(const SimulatorType &value)
{
    switch(value)
    {
        case SimulatorType::Unknown:
            return "0";
        case SimulatorType::MSFS95:
            return "1";
        case SimulatorType::MSFS98:
            return "2";
        case SimulatorType::MSCFS:
            return "3";
        case SimulatorType::AS2:
            return "4";
        case SimulatorType::PS1:
            return "5";
        case SimulatorType::XPlane:
            return "6";
        default:
            return "0";
    }
}

template<>
SimulatorType fromQString(const QString &str)
{
    if(str.isEmpty()) return SimulatorType::Unknown;

    if(str == "0")
    {
        return SimulatorType::Unknown;
    }
    else if(str == "1")
    {
        return SimulatorType::MSFS95;
    }
    else if(str == "2")
    {
        return SimulatorType::MSFS98;
    }
    else if(str == "3")
    {
        return SimulatorType::MSCFS;
    }
    else if(str == "4")
    {
        return SimulatorType::AS2;
    }
    else if(str == "5")
    {
        return SimulatorType::PS1;
    }
    else if(str == "6")
    {
        return SimulatorType::XPlane;
    }
    else
    {
        return SimulatorType::Unknown;
    }
}

template<>
QString toQString(const ClientQueryType &value)
{
    switch(value)
    {
        case ClientQueryType::IsValidATC:
            return "ATC";
        case ClientQueryType::Capabilities:
            return "CAPS";
        case ClientQueryType::COM1Freq:
            return "C?";
        case ClientQueryType::RealName:
            return "RN";
        case ClientQueryType::Server:
            return "SV";
        case ClientQueryType::ATIS:
            return "ATIS";
        case ClientQueryType::PublicIP:
            return "IP";
        case ClientQueryType::INF:
            return "INF";
        case ClientQueryType::FlightPlan:
            return "FP";
        case ClientQueryType::IPC:
            return "IPC";
        case ClientQueryType::RequestRelief:
            return "BY";
        case ClientQueryType::CancelRequestRelief:
            return "HI";
        case ClientQueryType::RequestHelp:
            return "HLP";
        case ClientQueryType::CancelRequestHelp:
            return "NOHLP";
        case ClientQueryType::WhoHas:
            return "WH";
        case ClientQueryType::InitiateTrack:
            return "IT";
        case ClientQueryType::AcceptHandoff:
            return "HT";
        case ClientQueryType::DropTrack:
            return "DR";
        case ClientQueryType::SetFinalAltitude:
            return "FA";
        case ClientQueryType::SetTempAltitude:
            return "TA";
        case ClientQueryType::SetBeaconCode:
            return "BC";
        case ClientQueryType::SetScratchpad:
            return "SC";
        case ClientQueryType::SetVoiceType:
            return "VT";
        case ClientQueryType::AircraftConfiguration:
            return "ACC";
        case ClientQueryType::NewInfo:
            return "NEWINFO";
        case ClientQueryType::NewATIS:
            return "NEWATIS";
        case ClientQueryType::Estimate:
            return "EST";
        case ClientQueryType::SetGlobalData:
            return "GD";
        default:
            return "";
    }
}

template<>
ClientQueryType fromQString(const QString &str)
{
    if(str.isEmpty()) return ClientQueryType::Unknown;

    if(str == "ATC") {
        return ClientQueryType::IsValidATC;
    }
    else if(str == "CAPS") {
        return ClientQueryType::Capabilities;
    }
    else if(str == "C?") {
        return ClientQueryType::COM1Freq;
    }
    else if(str == "RN") {
        return ClientQueryType::RealName;
    }
    else if(str == "SV") {
        return ClientQueryType::Server;
    }
    else if(str == "ATIS") {
        return ClientQueryType::ATIS;
    }
    else if(str == "IP") {
        return ClientQueryType::PublicIP;
    }
    else if(str == "INF") {
        return ClientQueryType::INF;
    }
    else if(str == "FP") {
        return ClientQueryType::FlightPlan;
    }
    else if(str == "IPC") {
        return ClientQueryType::IPC;
    }
    else if(str == "BY") {
        return ClientQueryType::RequestRelief;
    }
    else if(str == "HI") {
        return ClientQueryType::CancelRequestRelief;
    }
    else if(str == "HLP") {
        return ClientQueryType::RequestHelp;
    }
    else if(str == "NOHLP") {
        return ClientQueryType::CancelRequestHelp;
    }
    else if(str == "WH") {
        return ClientQueryType::WhoHas;
    }
    else if(str == "IT") {
        return ClientQueryType::InitiateTrack;
    }
    else if(str == "HT") {
        return ClientQueryType::AcceptHandoff;
    }
    else if(str == "DR") {
        return ClientQueryType::DropTrack;
    }
    else if(str == "FA") {
        return ClientQueryType::SetFinalAltitude;
    }
    else if(str == "TA") {
        return ClientQueryType::SetTempAltitude;
    }
    else if(str == "BC") {
        return ClientQueryType::SetBeaconCode;
    }
    else if(str == "SC") {
        return ClientQueryType::SetScratchpad;
    }
    else if(str == "VT") {
        return ClientQueryType::SetVoiceType;
    }
    else if(str == "ACC") {
        return ClientQueryType::AircraftConfiguration;
    }
    else if(str == "NEWINFO") {
        return ClientQueryType::NewInfo;
    }
    else if(str == "NEWATIS") {
        return ClientQueryType::NewATIS;
    }
    else if(str == "EST") {
        return ClientQueryType::Estimate;
    }
    else if(str == "GD") {
        return ClientQueryType::SetGlobalData;
    }
    else {
        return ClientQueryType::Unknown;
    }
}

template<>
QString toQString(const FlightRules &value)
{
    switch(value)
    {
        case FlightRules::IFR:
            return "I";
        case FlightRules::VFR:
        case FlightRules::DVFR:
        case FlightRules::SVFR:
            return "V";
        default:
            return "";
    }
}

template<>
FlightRules fromQString(const QString &str)
{
    if(str.isEmpty()) return FlightRules::Unknown;

    if(str == "I" || str == "IFR") {
        return FlightRules::IFR;
    }
    else if(str == "V" || str == "VFR") {
        return FlightRules::VFR;
    }
    else if(str == "D" || str == "DVFR") {
        return FlightRules::DVFR;
    }
    else if(str == "S" || str == "SVFR") {
        return FlightRules::SVFR;
    }
    else {
        return FlightRules::Unknown;
    }
}

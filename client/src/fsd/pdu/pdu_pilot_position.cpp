#include "pdu_pilot_position.h"

PDUPilotPosition::PDUPilotPosition() : PDUBase() {}

PDUPilotPosition::PDUPilotPosition(QString from, int txCode, bool squawkingModeC, bool identing, NetworkRating rating, double lat, double lon, int trueAlt, int pressureAlt, int gs, double pitch, double heading, double bank) :
    PDUBase(from, "")
{
    SquawkCode = txCode;
    SquawkingModeC = squawkingModeC;
    Identing = identing;
    Rating = rating;
    Lat = lat;
    Lon = lon;
    TrueAltitude = trueAlt;
    PressureAltitude = pressureAlt;
    GroundSpeed = gs;
    Pitch = pitch;
    Heading = heading;
    Bank = bank;
}

QStringList PDUPilotPosition::toTokens() const
{
    QStringList tokens;
    tokens.append(Identing ? "Y" : (SquawkingModeC ? "N" : "S"));
    tokens.append(From);
    tokens.append(QString::number(SquawkCode));
    tokens.append(toQString(Rating));
    tokens.append(QString::number(Lat, 'f', 6));
    tokens.append(QString::number(Lon, 'f', 6));
    tokens.append(QString::number(TrueAltitude));
    tokens.append(QString::number(GroundSpeed));
    tokens.append(QString::number(PackPitchBankHeading(Pitch, Bank, Heading)));
    tokens.append(QString::number(PressureAltitude - TrueAltitude));
    return tokens;
}

PDUPilotPosition PDUPilotPosition::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 10) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    double pitch;
    double bank;
    double heading;
    UnpackPitchBankHeading(tokens[8].toUInt(), pitch, bank, heading);

    bool identing = false;
    bool charlie = false;
    if(tokens[0] == "N") {
        charlie = true;
    }
    else if(tokens[0] == "Y") {
        charlie = true;
        identing = true;
    }

    return PDUPilotPosition(tokens[1], tokens[2].toInt(), charlie, identing, fromQString<NetworkRating>(tokens[3]),
            tokens[4].toDouble(), tokens[5].toDouble(), tokens[6].toInt(),
            tokens[6].toInt() + tokens[9].toInt(), tokens[7].toInt(), pitch, heading, bank);
}

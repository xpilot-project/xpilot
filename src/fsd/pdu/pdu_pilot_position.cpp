#include "pdu_pilot_position.h"

PDUPilotPosition::PDUPilotPosition(QString from, int txCode, bool squawkingModeC, bool identing, NetworkRating rating, double lat, double lon, int trueAlt, int pressureAlt, int gs, double pitch, double heading, double bank) :
    PDUBase(from, "")
{
    if(isnan(lat)) {

    }

    if(isnan(lon)) {

    }

    SquawkCode = txCode;
    IsSquawkingModeC = squawkingModeC;
    IsIdenting = identing;
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

QString PDUPilotPosition::Serialize()
{
    QStringList tokens;

    tokens.append("@");
    tokens.append(IsIdenting ? "Y" : (IsSquawkingModeC ? "N" : "S"));
    tokens.append(From);
    tokens.append(QString::number(SquawkCode));
    tokens.append(toQString(Rating));
    tokens.append(QString::number(Lat, 'f', 6));
    tokens.append(QString::number(Lon, 'f', 6));
    tokens.append(QString::number(TrueAltitude));
    tokens.append(QString::number(GroundSpeed));
    tokens.append(QString::number(PackPitchBankHeading(Pitch, Bank, Heading)));
    tokens.append(QString::number(PressureAltitude - TrueAltitude));

    return tokens.join(Delimeter);
}

PDUPilotPosition PDUPilotPosition::Parse(QStringList fields)
{
    if(fields.length() < 10) {

    }

    double pitch;
    double bank;
    double heading;
    UnpackPitchBankHeading(fields[8].toUInt(), pitch, bank, heading);

    bool identing = false;
    bool charlie = false;
    if(fields[0] == "N") {
        charlie = true;
    }
    else if(fields[0] == "Y") {
        charlie = true;
        identing = true;
    }

    return PDUPilotPosition(fields[1], fields[2].toInt(), charlie, identing, fromQString<NetworkRating>(fields[3]),
            fields[4].toDouble(), fields[5].toDouble(), fields[6].toInt(),
            fields[6].toInt() + fields[9].toInt(), fields[7].toInt(), pitch, heading, bank);
}

#include "pdu_fast_pilot_position.h"

PDUFastPilotPosition::PDUFastPilotPosition(QString from, double lat, double lon, double alt, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank) :
    PDUBase(from, "")
{
    if(isnan(lat)) {

    }

    if(isnan(lon)) {

    }

    Lat = lat;
    Lon = lon;
    Altitude = alt;
    Pitch = pitch;
    Heading = heading;
    Bank = bank;
    VelocityLongitude = velocityLongitude;
    VelocityAltitude = velocityAltitude;
    VelocityLatitude = velocityLatitude;
    VelocityPitch = velocityPitch;
    VelocityHeading = velocityHeading;
    VelocityBank = velocityBank;
}

QString PDUFastPilotPosition::Serialize()
{
    QStringList tokens;

    tokens.append("^");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(QString::number(Lat, 'f', 6));
    tokens.append(Delimeter);
    tokens.append(QString::number(Lon, 'f', 6));
    tokens.append(Delimeter);
    tokens.append(QString::number(Altitude, 'f', 2));
    tokens.append(Delimeter);
    tokens.append(QString::number(PackPitchBankHeading(Pitch, Bank, Heading)));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityLongitude, 'f', 4));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityAltitude, 'f', 4));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityLatitude, 'f', 4));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityPitch, 'f', 4));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityHeading, 'f', 4));
    tokens.append(Delimeter);
    tokens.append(QString::number(VelocityBank, 'f', 4));

    return tokens.join("");
}

PDUFastPilotPosition PDUFastPilotPosition::Parse(QStringList fields)
{
    if(fields.length() < 10) {

    }

    double pitch;
    double bank;
    double heading;
    UnpackPitchBankHeading(fields[4].toUInt(), pitch, bank, heading);

    return PDUFastPilotPosition(fields[0], fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble(),
            pitch, heading, bank, fields[5].toDouble(), fields[6].toDouble(), fields[7].toDouble(),
            fields[8].toDouble(), fields[9].toDouble(), fields[10].toDouble());
}

#include "pdu_fast_pilot_position.h"

PDUFastPilotPosition::PDUFastPilotPosition() : PDUBase() {}

PDUFastPilotPosition::PDUFastPilotPosition(QString from, double lat, double lon, double alt, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank) :
    PDUBase(from, "")
{
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

QStringList PDUFastPilotPosition::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(QString::number(Lat, 'f', 6));
    tokens.append(QString::number(Lon, 'f', 6));
    tokens.append(QString::number(Altitude, 'f', 2));
    tokens.append(QString::number(PackPitchBankHeading(Pitch, Bank, Heading)));
    tokens.append(QString::number(VelocityLongitude, 'f', 4));
    tokens.append(QString::number(VelocityAltitude, 'f', 4));
    tokens.append(QString::number(VelocityLatitude, 'f', 4));
    tokens.append(QString::number(VelocityPitch, 'f', 4));
    tokens.append(QString::number(VelocityHeading, 'f', 4));
    tokens.append(QString::number(VelocityBank, 'f', 4));
    return tokens;
}

PDUFastPilotPosition PDUFastPilotPosition::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 10) {
        return {};
    }

    double pitch;
    double bank;
    double heading;
    UnpackPitchBankHeading(tokens[4].toUInt(), pitch, bank, heading);

    return PDUFastPilotPosition(tokens[0], tokens[1].toDouble(), tokens[2].toDouble(), tokens[3].toDouble(),
            pitch, heading, bank, tokens[5].toDouble(), tokens[6].toDouble(), tokens[7].toDouble(),
            tokens[8].toDouble(), tokens[9].toDouble(), tokens[10].toDouble());
}

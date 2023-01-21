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

#include "pdu_fast_pilot_position.h"

PDUFastPilotPosition::PDUFastPilotPosition() : PDUBase() {}

PDUFastPilotPosition::PDUFastPilotPosition(FastPilotPositionType type, QString from, double lat, double lon, double altTrue, double altAgl, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank, double noseGearAngle) :
    PDUBase(from, "")
{
    Type = type;
    Lat = lat;
    Lon = lon;
    AltitudeTrue = altTrue;
    AltitudeAgl = altAgl;
    Pitch = pitch;
    Heading = heading;
    Bank = bank;
    VelocityLongitude = velocityLongitude;
    VelocityAltitude = velocityAltitude;
    VelocityLatitude = velocityLatitude;
    VelocityPitch = velocityPitch;
    VelocityHeading = velocityHeading;
    VelocityBank = velocityBank;
    NoseGearAngle = noseGearAngle;
}

QStringList PDUFastPilotPosition::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(QString::number(Lat, 'f', 6));
    tokens.append(QString::number(Lon, 'f', 6));
    tokens.append(QString::number(AltitudeTrue, 'f', 2));
    tokens.append(QString::number(AltitudeAgl, 'f', 2));
    tokens.append(QString::number(PackPitchBankHeading(Pitch, Bank, Heading)));
    if(Type != FastPilotPositionType::Stopped)
    {
        tokens.append(QString::number(VelocityLongitude, 'f', 4));
        tokens.append(QString::number(VelocityAltitude, 'f', 4));
        tokens.append(QString::number(VelocityLatitude, 'f', 4));
        tokens.append(QString::number(VelocityPitch, 'f', 4));
        tokens.append(QString::number(VelocityHeading, 'f', 4));
        tokens.append(QString::number(VelocityBank, 'f', 4));
    }
    tokens.append(QString::number(NoseGearAngle, 'f', 2));
    return tokens;
}

PDUFastPilotPosition PDUFastPilotPosition::fromTokens(FastPilotPositionType type, const QStringList &tokens)
{
    int fieldCount = 12;
    if(type == FastPilotPositionType::Stopped) {
        fieldCount = 6;
    }

    if(tokens.length() < fieldCount) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    double pitch;
    double bank;
    double heading;
    UnpackPitchBankHeading(tokens[5].toUInt(), pitch, bank, heading);

    QString from = tokens[0];
    double lat = tokens[1].toDouble();
    double lon = tokens[2].toDouble();
    double altTrue = tokens[3].toDouble();
    double altAgl = tokens[4].toDouble();
    double velLon = 0.0;
    double velAlt = 0.0;
    double velLat = 0.0;
    double velPitch = 0.0;
    double velHeading = 0.0;
    double velBank = 0.0;
    double noseGearAngle = 0.0;

    if(type != FastPilotPositionType::Stopped) {
        velLon = tokens[6].toDouble();
        velAlt = tokens[7].toDouble();
        velLat = tokens[8].toDouble();
        velPitch = tokens[9].toDouble();
        velHeading = tokens[10].toDouble();
        velBank = tokens[11].toDouble();
        noseGearAngle = tokens.length() >= 13 ? tokens[12].toDouble() : 0.0;
    }
    else {
        noseGearAngle = tokens.length() >= 7 ? tokens[6].toDouble() : 0.0;
    }

    return PDUFastPilotPosition(type, from, lat, lon, altTrue, altAgl, pitch, heading, bank,
                                velLon, velAlt, velLat, velPitch, velHeading, velBank, noseGearAngle);
}

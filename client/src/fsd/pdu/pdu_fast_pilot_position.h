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

#ifndef PDU_FASTPILOTPOSITION_H
#define PDU_FASTPILOTPOSITION_H

#include <QString>
#include "pdu_base.h"

enum class FastPilotPositionType
{
    Fast,
    Slow,
    Stopped
};

class PDUFastPilotPosition : public PDUBase
{
public:
    PDUFastPilotPosition(FastPilotPositionType type, QString from, double lat, double lon, double altTrue, double altAgl, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank, double noseGearAngle);

    QStringList toTokens() const;

    static PDUFastPilotPosition fromTokens(FastPilotPositionType type, const QStringList& fields);

    QString pdu() const
    {
        switch(Type) {
        case FastPilotPositionType::Fast:
            return "^";
        case FastPilotPositionType::Slow:
            return "#SL";
        case FastPilotPositionType::Stopped:
            return "#ST";
        default:
            return "^";
        }
    }

    FastPilotPositionType Type;
    double Lat;
    double Lon;
    double AltitudeTrue;
    double AltitudeAgl;
    double Pitch;
    double Heading;
    double Bank;
    double VelocityLongitude;
    double VelocityAltitude;
    double VelocityLatitude;
    double VelocityPitch;
    double VelocityHeading;
    double VelocityBank;
    double NoseGearAngle;

private:
    PDUFastPilotPosition();
};

#endif

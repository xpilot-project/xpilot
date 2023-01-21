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

#ifndef PDU_PILOTPOS_H
#define PDU_PILOTPOS_H

#include <QString>
#include "pdu_base.h"

class PDUPilotPosition: public PDUBase
{
public:
    PDUPilotPosition(QString from, int txCode, bool squawkingModeC, bool identing, NetworkRating rating, double lat, double lon, int trueAlt, int pressureAlt, int gs, double pitch, double heading, double bank);

    QStringList toTokens() const;

    static PDUPilotPosition fromTokens(const QStringList& fields);

    static QString pdu() { return "@"; }

    int SquawkCode;
    bool SquawkingModeC;
    bool Identing;
    NetworkRating Rating;
    double Lat;
    double Lon;
    int TrueAltitude;
    int PressureAltitude;
    int GroundSpeed;
    double Pitch;
    double Heading;
    double Bank;

private:
    PDUPilotPosition();
};

#endif

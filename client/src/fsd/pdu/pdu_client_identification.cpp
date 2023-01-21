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

#include "pdu_client_identification.h"

PDUClientIdentification::PDUClientIdentification() : PDUBase() {}

PDUClientIdentification::PDUClientIdentification(QString from, ushort clientID, QString clientName, int majorVersion, int minorVersion, QString cid, QString sysUID, QString initialChallenge) : PDUBase(from, ServerCallsign)
{
    ClientId = clientID;
    ClientName = clientName;
    MajorVersion = majorVersion;
    MinorVersion = minorVersion;
    CID = cid;
    SystemUID = sysUID;
    InitialChallenge = initialChallenge;
}

QStringList PDUClientIdentification::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number(ClientId, 16).toLower());
    tokens.append(ClientName);
    tokens.append(QString::number(MajorVersion));
    tokens.append(QString::number(MinorVersion));
    tokens.append(CID);
    tokens.append(SystemUID);
    if(!InitialChallenge.isEmpty()) {
        tokens.append(InitialChallenge);
    }
    return tokens;
}

PDUClientIdentification PDUClientIdentification::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 8) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUClientIdentification(tokens[0],tokens[2].toUShort(nullptr, 16),tokens[3], tokens[4].toInt(),
            tokens[5].toInt(), tokens[6], tokens[7], tokens.size() > 8 ? tokens[8] : QString());
}

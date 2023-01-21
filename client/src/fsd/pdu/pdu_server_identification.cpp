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

#include "pdu_server_identification.h"

PDUServerIdentification::PDUServerIdentification() : PDUBase() {}

PDUServerIdentification::PDUServerIdentification(QString from, QString to, QString version, QString initialChallengeKey)
    : PDUBase(from, to)
{
    Version = version;
    InitialChallengeKey = initialChallengeKey;
}

QStringList PDUServerIdentification::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Version);
    tokens.append(InitialChallengeKey);
    return tokens;
}

PDUServerIdentification PDUServerIdentification::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 4) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUServerIdentification(tokens[0], tokens[1], tokens[2], tokens[3]);
}

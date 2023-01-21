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

#include "pdu_kill_request.h"

PDUKillRequest::PDUKillRequest() : PDUBase() {}

PDUKillRequest::PDUKillRequest(QString from, QString victim, QString reason) :
    PDUBase(from, victim)
{
    Reason = reason;
}

QStringList PDUKillRequest::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Reason);
    return tokens;
}

PDUKillRequest PDUKillRequest::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 2) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUKillRequest(tokens[0], tokens[1], tokens.length() > 2 ? tokens[2] : "");
}

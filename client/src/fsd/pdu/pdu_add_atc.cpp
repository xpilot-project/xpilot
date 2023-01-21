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

#include "pdu_add_atc.h"

PDUAddATC::PDUAddATC() : PDUBase() {}

PDUAddATC::PDUAddATC(QString callsign, QString realName, QString cid, QString password, NetworkRating rating, ProtocolRevision proto) : PDUBase(callsign, "")
{
    RealName = realName;
    CID = cid;
    Password = password;
    Rating = rating;
    Protocol = proto;
}

QStringList PDUAddATC::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(PDUBase::ServerCallsign);
    tokens.append(RealName);
    tokens.append(CID);
    tokens.append(Password);
    tokens.append(toQString(Rating));
    tokens.append(toQString(Protocol));
    return tokens;
}

PDUAddATC PDUAddATC::fromTokens(const QStringList &tokens)
{
    if(tokens.size() < 6) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUAddATC(tokens[0], tokens[2], tokens[3], tokens[4], fromQString<NetworkRating>(tokens[5]), fromQString<ProtocolRevision>(tokens[6]));
}

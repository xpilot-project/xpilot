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

#include "pdu_client_query_response.h"

PDUClientQueryResponse::PDUClientQueryResponse() : PDUBase() {}

PDUClientQueryResponse::PDUClientQueryResponse(QString from, QString to, ClientQueryType queryType, QStringList payload) :
    PDUBase(from, to)
{
    QueryType = queryType;
    Payload = payload;
}

QStringList PDUClientQueryResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(toQString(QueryType));
    tokens.append(Payload);
    return tokens;
}

PDUClientQueryResponse PDUClientQueryResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList responseData;
    if (tokens.size() > 3) { responseData = tokens.mid(3); }
    return PDUClientQueryResponse(tokens[0], tokens[1], fromQString<ClientQueryType>(tokens[2]), responseData);
}

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

#include "pdu_plane_info_response.h"

PDUPlaneInfoResponse::PDUPlaneInfoResponse() : PDUBase() {}

PDUPlaneInfoResponse::PDUPlaneInfoResponse(QString from, QString to, QString equipment, QString airline, QString livery, QString csl) :
    PDUBase(from, to)
{
    Equipment = equipment;
    Airline = airline;
    Livery = livery;
    CSL = csl;
}

QStringList PDUPlaneInfoResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append("PI");
    tokens.append("GEN");
    tokens.append("EQUIPMENT=" + Equipment);
    if(!Airline.isEmpty()) {
        tokens.append("AIRLINE=" + Airline);
    }
    if(!Livery.isEmpty()) {
        tokens.append("LIVERY=" + Livery);
    }
    if(!CSL.isEmpty()) {
        tokens.append("CSL=" + CSL);
    }
    return tokens;
}

PDUPlaneInfoResponse PDUPlaneInfoResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 5) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUPlaneInfoResponse(tokens[0], tokens[1], FindValue(tokens, "EQUIPMENT"),
            FindValue(tokens, "AIRLINE"), FindValue(tokens, "LIVERY"), FindValue(tokens, "CSL"));
}

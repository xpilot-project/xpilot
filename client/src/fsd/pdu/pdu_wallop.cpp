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

#include "pdu_wallop.h"

PDUWallop::PDUWallop() : PDUBase() {}

PDUWallop::PDUWallop(QString from, QString message) :
    PDUBase(from, "*S")
{
    Message = message;
}

QStringList PDUWallop::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Message);
    return tokens;
}

PDUWallop PDUWallop::fromTokens(const QStringList &tokens)
{
    if(tokens.length() > 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList msgTokens = tokens.mid(2);
    return PDUWallop(tokens[0], msgTokens.join(":"));
}

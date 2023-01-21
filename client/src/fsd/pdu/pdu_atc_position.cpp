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

#include "pdu_atc_position.h"

PDUATCPosition::PDUATCPosition() : PDUBase() {}

PDUATCPosition::PDUATCPosition(QString from, QList<int> freqs, NetworkFacility facility, qint32 visRange, NetworkRating rating, double lat, double lon)
    : PDUBase(from, "")
{
    From = from;
    Frequencies = freqs;
    Facility = facility;
    VisibilityRange = visRange;
    Rating = rating;
    Lat = lat;
    Lon = lon;
}

QStringList PDUATCPosition::toTokens() const
{
    QStringList freqs;
    for(auto &freq : Frequencies) {
        freqs.append(QString::number(freq));
    }

    QStringList tokens;
    tokens.append(From);
    tokens.append(freqs.join("&"));
    tokens.append(toQString(Facility));
    tokens.append(QString::number(VisibilityRange));
    tokens.append(toQString(Rating));
    tokens.append(QString::number(Lat, 'f', 5));
    tokens.append(QString::number(Lon, 'f', 5));
    tokens.append(QString::number(0));
    return tokens;
}

PDUATCPosition PDUATCPosition::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 7) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    auto freqs = tokens[1].split("&", Qt::SkipEmptyParts);
    QList<int> freqInts;
    for(int i = 0; i < freqs.length(); i++) {
        freqInts.push_back(freqs[i].toInt() + 100000);
    }

    return PDUATCPosition(tokens[0], freqInts, fromQString<NetworkFacility>(tokens[2]),
            tokens[3].toInt(), fromQString<NetworkRating>(tokens[4]), tokens[5].toDouble(), tokens[6].toDouble());
}

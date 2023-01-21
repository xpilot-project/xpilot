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

#include "pdu_radio_message.h"

PDURadioMessage::PDURadioMessage() : PDUBase() {}

PDURadioMessage::PDURadioMessage(QString from, QList<uint> freqs, QString message) :
    PDUBase(from, "")
{
    Frequencies = freqs;
    Messages = message;
}

QStringList PDURadioMessage::toTokens() const
{
    QStringList freqs;
    for(auto & freq : Frequencies) {
        if(freqs.length() > 0) {
            freqs.append("&");
        }
        freqs.append("@" + QString::number(freq));
    }

    QStringList tokens;
    tokens.append(From);
    tokens.append(freqs.join(""));
    tokens.append(Messages);
    return tokens;
}

PDURadioMessage PDURadioMessage::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList freqs = tokens[1].split("&");
    QList<uint> freqInts;
    for(int i = 0; i < freqs.size(); i++) {
        freqInts.push_back(freqs[i].mid(1, freqs[i].length() - 1).toUInt());
    }

    QStringList messageTokens = tokens.mid(2);
    return PDURadioMessage(tokens[0], freqInts, messageTokens.join(":"));
}

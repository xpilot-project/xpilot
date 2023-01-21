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

#include "pdu_protocol_error.h"

PDUProtocolError::PDUProtocolError() : PDUBase() {}

PDUProtocolError::PDUProtocolError(QString from, QString to, NetworkError type, QString param, QString msg, bool fatal) :
    PDUBase(from, to)
{
    ErrorType = type;
    Param = param;
    Message = msg;
    Fatal = fatal;
}

QStringList PDUProtocolError::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number(static_cast<int>(ErrorType)));
    tokens.append(Param);
    tokens.append(Message);
    return tokens;
}

PDUProtocolError PDUProtocolError::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 5) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    NetworkError err = static_cast<NetworkError>(tokens[2].toInt());
    bool fatal = ((err == NetworkError::CallsignInUse) ||
                  (err == NetworkError::CallsignInvalid) ||
                  (err == NetworkError::AlreadyRegistered) ||
                  (err == NetworkError::InvalidLogon) ||
                  (err == NetworkError::InvalidProtocolRevision) ||
                  (err == NetworkError::RequestedLevelTooHigh) ||
                  (err == NetworkError::ServerFull) ||
                  (err == NetworkError::CertificateSuspended) ||
                  (err == NetworkError::InvalidPositionForRating) ||
                  (err == NetworkError::UnauthorizedSoftware));

    return PDUProtocolError(tokens[0], tokens[1], err, tokens[3], tokens[4], fatal);
}

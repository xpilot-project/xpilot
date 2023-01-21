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

#ifndef PDUBASE_H
#define PDUBASE_H

#include <QString>
#include <QStringList>
#include <QStringBuilder>
#include <QDebug>

#include "pdu_format_exception.h"
#include "../serializer.h"

class PDUBase
{
public:
    PDUBase() {}
    PDUBase(QString from, QString to);

    static uint PackPitchBankHeading(double pitch, double bank, double heading);
    static void UnpackPitchBankHeading(uint pbh, double &pitch, double& bank, double& heading);

    inline static const QString ClientQueryBroadcastRecipient = "@94835";
    inline static const QString ClientQueryBroadcastRecipientPilots = "@94386";
    inline static const QChar Delimeter = ':';
    inline static const QString PacketDelimeter = "\r\n";
    inline static const QString ServerCallsign = "SERVER";

    static QString Reassemble(QStringList fields)
    {
        return fields.join(Delimeter);
    }

    QString From;
    QString To;
};

template <class T>
QString Serialize(const T &message)
{
    return message.pdu() % message.toTokens().join(':') % QStringLiteral("\r\n");
}

#endif

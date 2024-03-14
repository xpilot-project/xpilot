/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2024 Justin Shannon
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

#ifndef RADIO_MESSAGE_RECEIVED_H
#define RADIO_MESSAGE_RECEIVED_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>

struct RadioMessageReceived
{
    Q_GADGET

    Q_PROPERTY(QString From MEMBER From)
    Q_PROPERTY(QString Message MEMBER Message)
    Q_PROPERTY(QVariant Frequencies MEMBER Frequencies)
    Q_PROPERTY(bool IsDirect MEMBER IsDirect)
    Q_PROPERTY(bool DualReceiver MEMBER DualReceiver)

public:
    QString From;
    QString Message;
    QVariant Frequencies;
    bool IsDirect;
    bool DualReceiver;
};

Q_DECLARE_METATYPE(RadioMessageReceived)

#endif

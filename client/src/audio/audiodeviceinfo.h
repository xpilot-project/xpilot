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

#ifndef AUDIODEVICEINFO_H
#define AUDIODEVICEINFO_H

#include <QString>
#include <QObject>

struct AudioDeviceInfo
{
    Q_GADGET

    Q_PROPERTY(QString DeviceName MEMBER DeviceName)
    Q_PROPERTY(QString Id MEMBER Id)

public:
    QString DeviceName;
    QString Id;

    bool operator==(const AudioDeviceInfo& b) const {
        return DeviceName == b.DeviceName && Id == b.Id;
    }
    bool operator!=(const AudioDeviceInfo& b) const {
        return DeviceName != b.DeviceName || Id != b.Id;
    }
};
Q_DECLARE_METATYPE(AudioDeviceInfo)

#endif // AUDIODEVICEINFO_H

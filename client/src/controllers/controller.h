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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QtGlobal>
#include <QString>
#include <QObject>

struct Controller
{
    Q_GADGET

    Q_PROPERTY(QString Callsign MEMBER Callsign)
    Q_PROPERTY(uint Frequency MEMBER Frequency)
    Q_PROPERTY(uint NormalizedFrequency MEMBER NormalizedFrequency)
    Q_PROPERTY(double Latitude MEMBER Latitude)
    Q_PROPERTY(double Longitude MEMBER Longitude)
    Q_PROPERTY(QString RealName MEMBER RealName)

public:
    QString Callsign;
    uint Frequency;
    uint NormalizedFrequency;
    quint32  FrequencyHz;
    double Latitude;
    double Longitude;
    qint64 LastUpdateReceived;
    QString RealName;
    bool IsValidATC;
    bool IsDeletePending;

    bool IsValid() {
        return IsValidATC && Frequency != 199998;
    }

    bool operator==(const Controller& rhs) const
    {
      return Callsign == rhs.Callsign
              && Frequency == rhs.Frequency
              && NormalizedFrequency == rhs.NormalizedFrequency
              && FrequencyHz == rhs.FrequencyHz
              && Latitude == rhs.Latitude
              && Longitude == rhs.Longitude
              && LastUpdateReceived == rhs.LastUpdateReceived
              && RealName == rhs.RealName
              && IsValidATC == rhs.IsValidATC
              && IsDeletePending == rhs.IsDeletePending;
    }

    bool operator!=(const Controller& rhs) const
    {
        return Callsign != rhs.Callsign
                || Frequency != rhs.Frequency
                || NormalizedFrequency != rhs.NormalizedFrequency
                || FrequencyHz != rhs.FrequencyHz
                || Latitude != rhs.Latitude
                || Longitude != rhs.Longitude
                || LastUpdateReceived != rhs.LastUpdateReceived
                || RealName != rhs.RealName
                || IsValidATC != rhs.IsValidATC
                || IsDeletePending != rhs.IsDeletePending;
    }
};

Q_DECLARE_METATYPE(Controller)

#endif

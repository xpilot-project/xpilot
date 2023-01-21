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

#ifndef CONNECT_INFO_H
#define CONNECT_INFO_H

#include <QString>
#include <QObject>

namespace xpilot
{
    struct ConnectInfo
    {
        Q_GADGET

        Q_PROPERTY(QString Callsign MEMBER Callsign)
        Q_PROPERTY(QString TypeCode MEMBER TypeCode)
        Q_PROPERTY(QString SelcalCode MEMBER SelcalCode)
        Q_PROPERTY(bool ObserverMode MEMBER ObserverMode)
        Q_PROPERTY(bool TowerViewMode MEMBER TowerViewMode)

    public:
        QString Callsign;
        QString TypeCode;
        QString SelcalCode;
        bool ObserverMode;
        bool TowerViewMode;

        bool operator==(ConnectInfo& rhs) const
        {
            return Callsign == rhs.Callsign && TypeCode == rhs.TypeCode && SelcalCode == rhs.SelcalCode
                    && ObserverMode == rhs.ObserverMode && TowerViewMode == rhs.TowerViewMode;
        }

        bool operator!=(ConnectInfo& rhs) const
        {
            return Callsign != rhs.Callsign || TypeCode != rhs.TypeCode || SelcalCode != rhs.SelcalCode
                    || ObserverMode != rhs.ObserverMode || TowerViewMode != rhs.TowerViewMode;
        }
    };
}

Q_DECLARE_METATYPE(xpilot::ConnectInfo)

#endif

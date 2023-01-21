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

#ifndef NEARBYATC_H
#define NEARBYATC_H

#include <QObject>

struct NearbyAtc {
    Q_GADGET
    Q_PROPERTY(QString callsign MEMBER callsign)
    Q_PROPERTY(QString realname MEMBER realname)
    Q_PROPERTY(QString frequency MEMBER frequency)
    Q_PROPERTY(int sim_frequency MEMBER sim_frequency)
public:
    QString callsign;
    QString realname;
    QString frequency;
    int sim_frequency;
};
Q_DECLARE_METATYPE(NearbyAtc)

#endif // NEARBYATC_H

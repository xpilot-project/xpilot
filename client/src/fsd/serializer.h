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

#ifndef FSD_SERIALIZER_H
#define FSD_SERIALIZER_H

#include <QtGlobal>
#include <QString>
#include "enums.h"

template<typename T>
inline QString toQString(const T& value);

template<typename T>
T fromQString(const QString &str);

template<>
QString toQString(const NetworkRating& value);

template<>
NetworkRating fromQString(const QString& str);

template<>
QString toQString(const NetworkFacility& value);

template<>
NetworkFacility fromQString(const QString& str);

template<>
QString toQString(const ProtocolRevision& value);

template<>
ProtocolRevision fromQString(const QString& str);

template<>
QString toQString(const SimulatorType& value);

template<>
SimulatorType fromQString(const QString& str);

template<>
QString toQString(const ClientQueryType& value);

template<>
ClientQueryType fromQString(const QString& str);

template<>
QString toQString(const FlightRules& value);

template<>
FlightRules fromQString(const QString& str);

#endif

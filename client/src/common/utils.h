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

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>

#include <QString>
#include <QtGlobal>
#include <QChar>
#include <QDir>

#include "common/enums.h"

constexpr qint64 COLOR_GREEN = 0x85a664;
constexpr qint64 COLOR_ORANGE = 0xffa500;
constexpr qint64 COLOR_WHITE = 0xffffff;
constexpr qint64 COLOR_GRAY = 0xc0c0c0;
constexpr qint64 COLOR_YELLOW = 0xffff00;
constexpr qint64 COLOR_RED = 0xeb2f06;
constexpr qint64 COLOR_CYAN = 0x00ffff;
constexpr qint64 COLOR_MAGENTA = 0xff00ff;

// is 0-9 char?
inline bool is09(const QChar &c) { return c >= u'0' && c <= u'9'; }

// returns true if any character in the string matches the given predicate
template <class F> bool containsChar(const QString &s, F predicate)
{
    return std::any_of(s.begin(), s.end(), predicate);
}

inline QString pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

static inline qint64 toColorHex(const enums::MessageType& type)
{
    switch(type) {
    case enums::MessageType::Server:
        return 0x85a664; // Green
    case enums::MessageType::IncomingPrivate:
        return 0x00ffff; // Cyan
    case enums::MessageType::OutgoingPrivate:
        return 0xc0c0c0; // Gray
    case enums::MessageType::TextOverride:
        return 0xcd5c5c; // IndianRed
    case enums::MessageType::IncomingRadioPrimary:
        return 0xffffff; // White
    case enums::MessageType::IncomingRadioSecondary:
        return 0xc0c0c0; // Gray
    case enums::MessageType::OutgoingRadio:
        return 0x00ffff; // Cyan
    case enums::MessageType::Broadcast:
        return 0xffa500; // Orange
    case enums::MessageType::Wallop:
        return 0xff0000; // Red
    case enums::MessageType::Info:
        return 0xffff00; // Yellow
    case enums::MessageType::Error:
        return 0xff0000; // Red
    default:
        return 0xffff00; // Yellow
    }
}

#endif // UTILS_H

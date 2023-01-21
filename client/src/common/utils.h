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

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>

#include <QString>
#include <QtGlobal>
#include <QChar>
#include <QDir>

constexpr qint64 COLOR_GREEN = 0x85a664;
constexpr qint64 COLOR_ORANGE = 0xffa500;
constexpr qint64 COLOR_WHITE = 0xffffff;
constexpr qint64 COLOR_GRAY = 0xc0c0c0;
constexpr qint64 COLOR_YELLOW = 0xffff00;
constexpr qint64 COLOR_RED = 0xeb2f06;
constexpr qint64 COLOR_CYAN = 0x00ffff;
constexpr qint64 COLOR_BRIGHT_GREEN = 0x00c000;

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

#endif // UTILS_H

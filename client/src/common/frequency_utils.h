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

#ifndef FREQUENCY_UTILS_H
#define FREQUENCY_UTILS_H

#include <QtGlobal>
#include <QString>

static uint Normalize25KhzFsdFrequency(uint freq)
{
    if(freq % 100 == 20 || freq % 100 == 70)
    {
        freq += 5;
    }
    return freq;
}

static uint FromNetworkFormat(uint freq)
{
    return (freq + 100000);
}

static uint Denormalize25KhzFsdFrequency(uint freq)
{
    if((freq % 100) == 25 || (freq % 100) == 75)
    {
        freq -= 5;
    }
    return freq;
}

static uint MatchFsdFormat(uint freq)
{
    QString tmp = QString::number(freq);
    tmp = tmp.mid(1, tmp.length() - 1);
    return tmp.toUInt();
}

#endif

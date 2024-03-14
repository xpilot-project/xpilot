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

#ifndef WINDOW_CONFIG_H
#define WINDOW_CONFIG_H

#include <QObject>

namespace xpilot
{
    struct ClientWindowConfig
    {
        Q_GADGET

        Q_PROPERTY(int X MEMBER X)
        Q_PROPERTY(int Y MEMBER Y)
        Q_PROPERTY(int Width MEMBER Width)
        Q_PROPERTY(int Height MEMBER Height)
        Q_PROPERTY(bool Maximized MEMBER Maximized)

    public:
        int X = 10;
        int Y = 10;
        int Width = 800;
        int Height = 250;
        bool Maximized = false;

        bool operator==(ClientWindowConfig& rhs) const
        {
            return X == rhs.X && Y == rhs.Y && Width == rhs.Width && Height == rhs.Height && Maximized == rhs.Maximized;
        }
        bool operator!=(ClientWindowConfig& rhs) const
        {
            return X != rhs.X || Y != rhs.Y || Width != rhs.Width || Height != rhs.Height || Maximized != rhs.Maximized;
        }
    };
}

Q_DECLARE_METATYPE(xpilot::ClientWindowConfig)

#endif

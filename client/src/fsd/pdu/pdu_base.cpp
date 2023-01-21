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

#include "pdu_base.h"

PDUBase::PDUBase(QString from, QString to) :
    From(from),
    To(to)
{

}

uint PDUBase::PackPitchBankHeading(double pitch, double bank, double heading)
{
    double p = pitch / -360.0;
    if(p < 0)
    {
        p += 1.0;
    }
    p *= 1024.0;

    double b = bank / -360.0;
    if(b < 0)
    {
        b += 1.0;
    }
    b *= 1024.0;

    double h = heading / 360.0 * 1024.0;

    return ((uint)p << 22) | ((uint)b << 12) | ((uint)h << 2);
}

void PDUBase::UnpackPitchBankHeading(uint pbh, double &pitch, double &bank, double &heading)
{
    uint pitchInt = pbh >> 22;
    pitch = pitchInt / 1024.0 * -360.0;
    if(pitch > 180.0)
    {
        pitch -= 360.0;
    }
    else if (pitch <= -180.0)
    {
        pitch += 360.0;
    }

    uint bankInt = (pbh >> 12) & 0x3FF;
    bank = bankInt / 1024.0 * -360.0;
    if (bank > 180.0)
    {
        bank -= 360.0;
    }
    else if (bank <= -180.0)
    {
        bank += 360.0;
    }

    uint hdgInt = (pbh >> 2) & 0x3FF;
    heading = hdgInt / 1024.0 * 360.0;
    if (heading < 0.0)
    {
        heading += 360.0;
    }
    else if (heading >= 360.0)
    {
        heading -= 360.0;
    }
}

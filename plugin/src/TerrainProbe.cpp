/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2021 Justin Shannon
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

#include "TerrainProbe.h"

namespace xpilot
{
    TerrainProbe::TerrainProbe() :
        m_probeRef(XPLMCreateProbe(xplm_ProbeY))
    {

    }

    TerrainProbe::~TerrainProbe()
    {
        XPLMDestroyProbe(m_probeRef);
    }

    double TerrainProbe::getTerrainElevation(double degLat, double degLon) const
    {
        double x, y, z, foo, alt;
        XPLMProbeInfo_t probeinfo;
        probeinfo.structSize = sizeof(XPLMProbeInfo_t);

        XPLMWorldToLocal(degLat, degLon, 0, &x, &y, &z);
        XPLMProbeTerrainXYZ(m_probeRef, x, y, z, &probeinfo);
        XPLMLocalToWorld(probeinfo.locationX, probeinfo.locationY, probeinfo.locationZ, &foo, &foo, &alt);
        XPLMWorldToLocal(degLat, degLon, alt, &x, &y, &z);
        if (XPLMProbeTerrainXYZ(m_probeRef, x, y, z, &probeinfo) == xplm_ProbeHitTerrain) 
        {
            XPLMLocalToWorld(probeinfo.locationX, probeinfo.locationY, probeinfo.locationZ, &foo, &foo, &alt);
            return alt * 3.28084;
        }

        return 0;
    }
}
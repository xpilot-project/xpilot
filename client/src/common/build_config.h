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

#ifndef VATSIM_CONFIG_H
#define VATSIM_CONFIG_H

#include <QString>
#include <QVersionNumber>

namespace xpilot
{
    class BuildConfig
    {
    public:
        static const ushort TowerviewClientId();
        static const ushort VatsimClientId();
        static const QString VatsimClientKey();
        static const quint64 ConfigEncryptionKey();

        static constexpr bool isRunningOnMacOSPlatform();
        static constexpr bool isRunningOnLinuxPlatform();
        static constexpr bool isRunningOnWindowsPlatform();
        static const QString getPlatformString();

        static const QString versionCheckUrl();

        static const QVersionNumber getVersion();
        static const int getVersionInt();
        static const QString getVersionString();
        static const QString getShortVersionString();
        static const QString getVersionStringPlatform();

        static constexpr int versionMajor();
        static constexpr int versionMinor();
        static constexpr int versionPatch();

        static bool isBetaVersion();
        static const int versionBeta();

        static const QString getSentryDsn();
    };
}

#define IN_BUILDCONFIG_H
#include "build_config.inc"
#undef IN_BUILDCONFIG_H

#endif

#ifndef VATSIM_CONFIG_H
#define VATSIM_CONFIG_H

#include <QString>
#include <QVersionNumber>

namespace xpilot
{
    class BuildConfig
    {
    public:
        static const ushort VatsimClientId();
        static const QString VatsimClientKey();
        static const quint64 ConfigEncryptionKey();
        static const bool isVelocityBuild();

        static constexpr bool isRunningOnMacOSPlatform();
        static constexpr bool isRunningOnLinuxPlatform();
        static constexpr bool isRunningOnWindowsPlatform();
        static const QString getPlatformString();

        static const QVersionNumber getVersion();
        static const QString getVersionString();
        static const QString getShortVersionString();
        static const QString getVersionStringPlatform();

        static constexpr int versionMajor();
        static constexpr int versionMinor();
        static constexpr int versionPatch();
    };
}

#define IN_BUILDCONFIG_H
#include "build_config.inc"
#undef IN_BUILDCONFIG_H

#endif

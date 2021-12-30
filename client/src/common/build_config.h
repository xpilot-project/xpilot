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

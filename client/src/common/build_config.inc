namespace xpilot
{
    constexpr bool BuildConfig::isRunningOnWindowsPlatform()
    {
        #ifdef Q_OS_WIN
            return true;
        #else
            return false;
        #endif
    }

    constexpr bool BuildConfig::isRunningOnMacOSPlatform()
    {
        #ifdef Q_OS_MACOS
            return true;
        #else
            return false;
        #endif
    }

    constexpr bool BuildConfig::isRunningOnLinuxPlatform()
    {
        #ifdef Q_OS_LINUX
            return true;
        #else
            return false;
        #endif
    }
}

#include "appcore.h"

namespace xpilot
{
    AppCore::AppCore(QObject *owner) : QObject(owner)
    {
        AppConfig::Instance().LoadConfig();
    }

    void AppCore::SaveConfig()
    {
        AppConfig::Instance().SaveConfig();
    }
}

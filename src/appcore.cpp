#include "appcore.h"
#include "network/networkinfo.h"

namespace xpilot
{
    AppCore::AppCore(QObject *owner) : QObject(owner)
    {
        AppConfig::Instance().LoadConfig();

        auto serverList = NetworkInfo::GetServerList("http://data.vatsim.net/vatsim-servers.json");
        if(serverList.size() > 0) {
            AppConfig::Instance().CachedServers.clear();
            for(auto & server: serverList) {
                AppConfig::Instance().CachedServers.append(server);
            }
            SaveConfig();
        }
    }

    void AppCore::SaveConfig()
    {
        AppConfig::Instance().SaveConfig();
    }
}

#include "appcore.h"
#include "network/networkserverlist.h"

namespace xpilot
{
    AppCore::AppCore(QObject *owner) : QObject(owner)
    {
        NetworkServerList networkServerList;
        auto serverList = networkServerList.DownloadServerList("http://data.vatsim.net/vatsim-servers.json");
        if(serverList.size() > 0) {
            AppConfig::getInstance()->CachedServers.clear();
            for(auto & server: serverList) {
                AppConfig::getInstance()->CachedServers.append(server);
            }
            SaveConfig();
        }
    }

    void AppCore::SaveConfig()
    {
        AppConfig::getInstance()->saveConfig();
    }
}

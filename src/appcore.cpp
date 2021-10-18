#include "appcore.h"
#include "network/networkserverlist.h"

namespace xpilot
{
    AppCore::AppCore(QObject *owner) : QObject(owner)
    {
        QTimer::singleShot(0, this, [this]{
            DownloadServerList();
        });
    }

    void AppCore::DownloadServerList()
    {
        NetworkServerList networkServerList;
        auto serverList = networkServerList.DownloadServerList("https://data.vatsim.net/v3/vatsim-servers.json");

        if(serverList.size() > 0) {
            emit serverListDownloaded(serverList.size());
            AppConfig::getInstance()->CachedServers.clear();
            for(auto & server: serverList) {
                AppConfig::getInstance()->CachedServers.append(server);
            }
            SaveConfig();
        }
        else {
            emit serverListDownloadError();
        }
    }

    void AppCore::SaveConfig()
    {
        AppConfig::getInstance()->saveConfig();
    }
}

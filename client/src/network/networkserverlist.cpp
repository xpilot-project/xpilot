#include "networkserverlist.h"
#include "src/common/build_config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>

namespace xpilot
{
    NetworkServerList::NetworkServerList(QObject *parent) : QObject(parent)
    {

    }

    QVector<NetworkServerInfo> NetworkServerList::DownloadServerList(const QString &url)
    {
        QNetworkAccessManager manager;
        QByteArray response;
        QEventLoop loop;
        connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
        connect(&manager, &QNetworkAccessManager::finished, [&](QNetworkReply *reply) {
            response = reply->readAll();
        });
        manager.get(QNetworkRequest(QUrl(url)));
        loop.exec();

        QJsonDocument json = QJsonDocument::fromJson(response);

        if(!json.isArray()) {
            return {};
        }

        QJsonArray servers = json.array();

        QVector<NetworkServerInfo> serverList;

        for(const QJsonValue & server : servers) {
            NetworkServerInfo serverInfo;
            serverInfo.Name = server["name"].toString();
            serverInfo.Address = server["hostname_or_ip"].toString();
            serverInfo.Location = server["location"].toString();
            serverList.append(serverInfo);
        }

        emit serverListDownloaded(serverList);

        return serverList;
    }
}

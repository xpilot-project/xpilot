/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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

#include "serverlistmanager.h"
#include "src/config/appconfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QtDebug>
#include <random>

using namespace QtPromise;

namespace xpilot
{
    ServerListManager::ServerListManager(QObject *parent) :
        QObject(parent),
        nam(new QNetworkAccessManager)
    {

    }

    void ServerListManager::PerformServerListDownload(const QString &url)
    {
        DownloadStatusInfo(url).then([&](QString serverUrl){
            return DownloadServerList(serverUrl);
        }).then([&](QVector<NetworkServerInfo> serverList){
            if(serverList.size() > 0) {
                AppConfig::getInstance()->CachedServers.clear();
                for(auto & server: serverList) {
                    AppConfig::getInstance()->CachedServers.append(server);
                    if(server.Name == "AUTOMATIC") {
                        AppConfig::getInstance()->ServerName = "AUTOMATIC";
                    }
                }
                AppConfig::getInstance()->saveConfig();
                emit serverListDownloaded(serverList.size());
            }
        }).fail([&](const QString &err){
            emit serverListDownloadError(err);
        });
    }

    QtPromise::QPromise<QString> ServerListManager::DownloadStatusInfo(const QString &url)
    {
        return QtPromise::QPromise<QString>{[&](const auto resolve, const auto reject)
            {
                m_reply = nam->get(QNetworkRequest{url});

                QObject::connect(m_reply, &QNetworkReply::finished, [=](){
                    if(m_reply->error() == QNetworkReply::NoError) {
                        QJsonDocument json = QJsonDocument::fromJson(m_reply->readAll());

                        QJsonObject dataObj = json["data"].toObject();
                        if(dataObj.isEmpty()) {
                            reject(QString{"Data object is empty."});
                            return;
                        }

                        QJsonArray serverFiles = dataObj["servers"].toArray();
                        if(serverFiles.isEmpty()) {
                            reject(QString{"Server list is empty."});
                            return;
                        }

                        int random = rand() % serverFiles.size();
                        resolve(serverFiles[random].toString());
                    } else {
                        reject(QString{m_reply->errorString()});
                    }
                });
            }};
    }

    QtPromise::QPromise<QVector<NetworkServerInfo>> ServerListManager::DownloadServerList(const QString &url)
    {
        return QtPromise::QPromise<QVector<NetworkServerInfo>>{[&](const auto resolve, const auto reject)
        {
            m_reply = nam->get(QNetworkRequest{url});

            QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
                if(m_reply->error() == QNetworkReply::NoError) {
                    QJsonDocument json = QJsonDocument::fromJson(m_reply->readAll());
                    if(!json.isArray()) {
                        reject(QString{"Error parsing network server list: response is not an array."});
                        return;
                    }

                    QJsonArray servers = json.array();
                    QVector<NetworkServerInfo> serverList;

                    for(const QJsonValue &server : servers) {
                        NetworkServerInfo serverInfo;
                        serverInfo.Name = server["name"].toString();
                        serverInfo.Address = server["hostname_or_ip"].toString();
                        serverInfo.Location = server["location"].toString();
                        serverList.append(serverInfo);
                    }

                    resolve(serverList);
                }
                else {
                    reject(QString{m_reply->errorString()});
                }
            });
        }};
    }
}

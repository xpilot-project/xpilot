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

#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtPromise>
#include <QPointer>

namespace xpilot
{
    struct NetworkServerInfo
    {
        Q_GADGET
    public:
        QString Name;
        QString Address;
        QString Location;
        QString Description;
        Q_PROPERTY(QString Name MEMBER Name)
        Q_PROPERTY(QString Address MEMBER Address)
    };

    class ServerListManager : public QObject
    {
        Q_OBJECT
    public:
        ServerListManager(QObject * parent = nullptr);
        void PerformServerListDownload(const QString &url);
        QtPromise::QPromise<QString> DownloadStatusInfo(const QString &url);
        QtPromise::QPromise<QVector<NetworkServerInfo>> DownloadServerList(const QString &url);

    signals:
        void serverListDownloaded(int count);
        void serverListDownloadError(QString error);

    private:
        QNetworkAccessManager *nam = nullptr;
        QPointer<QNetworkReply> m_reply;
    };
}

Q_DECLARE_METATYPE(xpilot::NetworkServerInfo)

#endif

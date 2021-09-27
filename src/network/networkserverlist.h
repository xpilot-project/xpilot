#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

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

    class NetworkServerList : public QObject
    {
        Q_OBJECT
    public:
        NetworkServerList(QObject * parent = nullptr);
        QVector<NetworkServerInfo> DownloadServerList(const QString& url);

    signals:
        void serverListDownloaded(QVector<NetworkServerInfo>& servers);
    };
}

Q_DECLARE_METATYPE(xpilot::NetworkServerInfo)

#endif

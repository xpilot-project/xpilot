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
        QString Name;
        QString Address;
        QString Location;
        QString Description;
    };

    class NetworkInfo : public QObject
    {
        Q_OBJECT

    public:
        NetworkInfo(QObject * parent = nullptr);
        static QVector<NetworkServerInfo> GetServerList(const QString& url);

    signals:
        void serverListDownloaded();
    };
}

#endif

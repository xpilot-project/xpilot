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

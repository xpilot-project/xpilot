#ifndef VERSIONCHECK_H
#define VERSIONCHECK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QStringBuilder>
#include <QUrl>
#include <QSaveFile>
#include <QPointer>
#include <QtPromise>

using namespace QtPromise;

class VersionCheck : public QObject
{
    Q_OBJECT

public:
    VersionCheck(QObject *parent = nullptr);

    void PerformVersionCheck();
    void DeleteOlderInstallers();
    QtPromise::QPromise<QByteArray> CheckForUpdates();
    QtPromise::QPromise<void> DownloadInstaller();
    Q_INVOKABLE void downloadInstaller();
    Q_INVOKABLE void cancelDownload();
    void LaunchInstaller();

signals:
    void newVersionAvailable();
    void noUpdatesAvailable();
    void downloadStarted();
    void downloadPercentChanged(double pct);
    void downloadFinished();
    void errorEncountered(QString error);

private:
    QNetworkAccessManager *nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    QPointer<QSaveFile> m_file;
    QString m_fileName;
    QString m_downloadUrl;
};

#endif // VERSIONCHECK_H

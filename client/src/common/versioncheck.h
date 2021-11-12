#ifndef VERSIONCHECK_H
#define VERSIONCHECK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QStringBuilder>
#include <QUrl>

class VersionCheck : public QObject
{
    Q_OBJECT

public:
    VersionCheck(QObject *parent = nullptr);
    void checkForUpdates();

signals:
    void newVersionAvailable(QString versionNumber, QString downloadLink);
    void noUpdatesAvailable();
};

#endif // VERSIONCHECK_H

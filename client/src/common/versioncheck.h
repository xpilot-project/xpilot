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
    void readUpdateInfo();
    void getUpdateInfo() const;

private:

};

#endif // VERSIONCHECK_H

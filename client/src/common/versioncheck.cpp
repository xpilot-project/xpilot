#include "versioncheck.h"
#include "src/common/utils.h"
#include "src/common/build_config.h"

#include <QStringLiteral>
#include <QStringBuilder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace xpilot;

VersionCheck::VersionCheck(QObject *parent) :
    QObject(parent)
{

}

void VersionCheck::readUpdateInfo()
{
    QNetworkAccessManager manager;
    QByteArray response;
    QEventLoop loop;
    connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    connect(&manager, &QNetworkAccessManager::finished, [&](QNetworkReply *reply) {
        if(reply->error() == QNetworkReply::NoError)
        {
            response = reply->readAll();
        }
    });
    manager.get(QNetworkRequest(QUrl(BuildConfig::githubRepoApiUrl() % u"releases")));
    loop.exec();

    if(!response.isEmpty())
    {
        const QJsonValue release = QJsonDocument::fromJson(response).array().first();

        QString version = release[QLatin1String("tag_name")].toString();
        if(version.isEmpty() || version[0] != 'v') return;
        version.remove(0, 1);
        if(containsChar(version, [](QChar c) { return c != '.' && !is09(c); })) return;

        bool published = !release[QLatin1String("draft")].toBool();
        bool alpha = release[QLatin1String("prerelease")].toBool();

        for (const QJsonValue asset : release[QLatin1String("assets")].toArray())
        {
            QString name = asset[QLatin1String("name")].toString();
            QString filename = QStringLiteral("v%1/%2").arg(version, name);
            int size = asset[QLatin1String("size")].toInt();

            if(name.contains("windows"))
            {

            }
            else if(name.contains("macos"))
            {

            }
            else if(name.contains("linux"))
            {

            }
        }
    }
}

void VersionCheck::getUpdateInfo() const
{

}

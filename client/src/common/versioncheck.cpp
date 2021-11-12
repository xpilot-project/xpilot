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

void VersionCheck::checkForUpdates()
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
        bool isNewVersion = false;

        auto release = QJsonDocument::fromJson(response).array().first();

        if(release.isUndefined())
        {
            emit noUpdatesAvailable();
            return;
        }

        QString version = release[QLatin1String("tag_name")].toString();
        if(version.isEmpty() || version[0] != 'v') return;
        version.remove(0, 1);

        bool published = !release[QLatin1String("draft")].toBool();
        bool beta = release[QLatin1String("prerelease")].toBool();

        if(!published) return;

        int suffixIndex;
        auto v = QVersionNumber::fromString(version, &suffixIndex);

        QString betaIdentifier = version.mid(suffixIndex + 1);
        if(beta && !betaIdentifier.isEmpty())
        {
            QRegularExpression rx("beta\\.(\\d+)", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = rx.match(betaIdentifier);
            int betaNumber = match.captured(1).toInt();

            if(BuildConfig::isBetaVersion())
            {
                if(betaNumber > BuildConfig::versionBeta())
                {
                    isNewVersion = true;
                }
            }
        }
        else
        {
            if(v > BuildConfig::getVersion())
            {
                isNewVersion = true;
            }
        }

        if(!isNewVersion)
        {
            emit noUpdatesAvailable();
            return;
        }

        auto assets = release[QLatin1String("assets")].toArray();

        if(assets.isEmpty())
        {
            emit noUpdatesAvailable();
            return;
        }

        for (const QJsonValue asset : assets)
        {
            QString name = asset[QLatin1String("name")].toString();
            QString downloadUrl = asset[QLatin1String("browser_download_url")].toString();

            if(BuildConfig::isRunningOnWindowsPlatform())
            {
                if(name.toLower().contains("windows"))
                {
                    emit newVersionAvailable(name, downloadUrl);
                }
            }
            else if(BuildConfig::isRunningOnMacOSPlatform())
            {
                if(name.toLower().contains("macos"))
                {
                    emit newVersionAvailable(name, downloadUrl);
                }
            }
            else if(BuildConfig::isRunningOnLinuxPlatform())
            {
                if(name.toLower().contains("linux"))
                {
                    emit newVersionAvailable(name, downloadUrl);
                }
            }
        }
    }
}

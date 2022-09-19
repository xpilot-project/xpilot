#include "versioncheck.h"
#include "src/common/utils.h"
#include "src/common/build_config.h"
#include "src/config/appconfig.h"

#include <QStringLiteral>
#include <QStringBuilder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QDirIterator>

using namespace xpilot;
using namespace QtPromise;

VersionCheck::VersionCheck(QObject *parent) :
    QObject(parent),
    nam(new QNetworkAccessManager)
{

}

QtPromise::QPromise<QByteArray> VersionCheck::CheckForUpdates()
{
    return QtPromise::QPromise<QByteArray>{[&](const auto resolve, const auto reject)
        {
            QString url(BuildConfig::versionCheckUrl());

            QNetworkRequest networkRequest(url);
            networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            m_reply = nam->get(networkRequest);

            QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
                if(m_reply->error() == QNetworkReply::NoError)
                {
                    resolve(m_reply->readAll());
                }
                else
                {
                    reject(QString{m_reply->errorString()});
                }
            });
        }};
}

void VersionCheck::PerformVersionCheck()
{
    DeleteOlderInstallers();
    CheckForUpdates().then([&](const QByteArray &response){
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
                    if(v > BuildConfig::getVersion() || betaNumber > BuildConfig::versionBeta())
                    {
                        isNewVersion = true;
                    }
                }
            }
            else
            {
                if((BuildConfig::isBetaVersion() && betaIdentifier.isEmpty()) && v >= BuildConfig::getVersion())
                {
                    isNewVersion = true;
                }
                else if(v > BuildConfig::getVersion())
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
                        emit newVersionAvailable();
                        m_fileName = name;
                        m_downloadUrl = downloadUrl;
                    }
                }
                else if(BuildConfig::isRunningOnMacOSPlatform())
                {
                    if(name.toLower().contains("macos"))
                    {
                        emit newVersionAvailable();
                        m_fileName = name;
                        m_downloadUrl = downloadUrl;
                    }
                }
                else if(BuildConfig::isRunningOnLinuxPlatform())
                {
                    emit newVersionAvailable();
                    m_fileName = name;
                    m_downloadUrl = downloadUrl;
                }
            }
        }
    }).fail([&](const QString &err){
        emit errorEncountered("Version check error: " + err);
    });
}

void VersionCheck::DeleteOlderInstallers()
{
    QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
    QDirIterator it(tempPath, {"*.exe", "*.dmg", "*.run"}, QDir::Files);
    while(it.hasNext()) {
        QFile file(it.next());
        file.remove();
    }
}

QtPromise::QPromise<void> VersionCheck::DownloadInstaller()
{
    return QtPromise::QPromise<void>{[&](const auto resolve, const auto reject)
        {
            emit downloadStarted();

            QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());

            m_file = new QSaveFile(pathAppend(tempPath, m_fileName));
            if(!m_file->open(QIODevice::WriteOnly))
            {
                reject(QString{"Error opening file for writing. Restart xPilot and try again."});
                return;
            }

            QNetworkRequest req(m_downloadUrl);
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            m_reply = nam->get(req);

            QObject::connect(m_reply, &QNetworkReply::downloadProgress, [&](qint64 read, qint64 total) {
                emit downloadPercentChanged((double)read / (double)total);
            });
            QObject::connect(m_reply, &QNetworkReply::readyRead, [&]{
                if(m_file) {
                    m_file->write(m_reply->readAll());
                }
            });
            QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
                if(m_reply->error() == QNetworkReply::NoError) {
                    if(m_file){
                        m_file->write(m_reply->readAll());
                        m_file->commit();
                        resolve();
                    }
                }
                else {
                    if(m_reply->error() != QNetworkReply::OperationCanceledError) {
                        reject(QString{m_reply->errorString()});
                    }
                }
                m_reply->deleteLater();
            });
        }};
}

void VersionCheck::downloadInstaller()
{
    DownloadInstaller().then([&](){
        emit downloadFinished();
        LaunchInstaller();
    }).fail([&](const QString &err){
        emit errorEncountered("Error downloading xPilot update: " + err);
    });
}

void VersionCheck::cancelDownload()
{
    if(m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
    if(m_file) {
        m_file->cancelWriting();
        m_file->deleteLater();
    }
}

void VersionCheck::LaunchInstaller()
{
    QString path(pathAppend(QDir::fromNativeSeparators(AppConfig::dataRoot()), m_fileName));

    if(BuildConfig::isRunningOnLinuxPlatform()) {
        // ensure linux installer is executable
        QFile executable(path);
        executable.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner| QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);
    }

    if(BuildConfig::isRunningOnMacOSPlatform()) {
        // macos: we have to mount the disk image
        QProcess p;
        p.setProgram("hdiutil");
        p.setArguments(QStringList() << "attach" << path << "-autoopen");
        p.startDetached();
        p.waitForStarted();
        QCoreApplication::quit();
    }
    else {
        // windows: we can directly launch the installer
        QProcess p;
        p.setProgram(path);
        p.startDetached();
        p.waitForStarted();
        QCoreApplication::quit();
    }
}

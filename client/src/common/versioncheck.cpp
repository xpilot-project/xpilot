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

#include <QStringLiteral>
#include <QStringBuilder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QDirIterator>

#include "versioncheck.h"
#include "common/utils.h"
#include "common/build_config.h"
#include "config/appconfig.h"

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

            qsizetype suffixIndex;
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

            for (const QJsonValue& asset : assets)
            {
                QString name = asset[QLatin1String("name")].toString();
                QString downloadUrl = asset[QLatin1String("browser_download_url")].toString();

                if(BuildConfig::isRunningOnWindowsPlatform())
                {
                    if(name.contains("windows", Qt::CaseInsensitive))
                    {
                        emit newVersionAvailable();
                        m_fileName = name;
                        m_downloadUrl = downloadUrl;
                    }
                }
                else if(BuildConfig::isRunningOnMacOSPlatform())
                {
                    if(name.contains("macos", Qt::CaseInsensitive))
                    {
                        emit newVersionAvailable();
                        m_fileName = name;
                        m_downloadUrl = downloadUrl;
                    }
                }
                else if(BuildConfig::isRunningOnLinuxPlatform())
                {
                    if(name.contains("linux", Qt::CaseInsensitive))
                    {
                        emit newVersionAvailable();
                        m_fileName = name;
                        m_downloadUrl = downloadUrl;
                    }
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

    QProcess *process = new QProcess();

    if(BuildConfig::isRunningOnMacOSPlatform()) {
        // macOS: Mount installer disk image then launch installer
        process->setProgram("hdiutil");
        process->setArguments(QStringList() << "attach" << path << "-autoopen");
    }
    else {
        // Windows and Linux: launch installer directly
        process->setProgram(path);
    }

    connect(process, &QProcess::errorOccurred, this, [this](auto error){
        emit errorEncountered("Error starting update process: " + error);
    });

    auto thread = new QThread;
    process->moveToThread(thread);
    process->startDetached();
    process->waitForStarted();

    qApp->quit();
}

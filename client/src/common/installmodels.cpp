#include "installmodels.h"
#include "libzippp.h"
#include "utils.h"
#include "build_config.h"
#include "src/config/appconfig.h"

#include <fstream>

#include <QFile>
#include <QDir>
#include <QtConcurrent/QtConcurrent>
#include <QtPromise>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

using namespace libzippp;
using namespace QtPromise;
using namespace xpilot;

InstallModels::InstallModels(QObject *parent) :
    QObject(parent),
    nam(new QNetworkAccessManager)
{

}

InstallModels::~InstallModels()
{

}

QtPromise::QPromise<QString> InstallModels::ValidateToken(const QString &url, const QString& vatsimId, const QString &token)
{
    return QtPromise::QPromise<QString>{[&](const auto resolve, const auto reject)
        {
            QNetworkRequest networkRequest(url);
            networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            QJsonObject obj;
            obj["id"] = vatsimId;
            obj["token"] = token;
            QJsonDocument doc(obj);

            m_reply = nam->post(networkRequest, doc.toJson());
            QObject::connect(m_reply, &QNetworkReply::finished, [=]{
                if(m_reply->error() == QNetworkReply::NoError) {
                    QJsonDocument jsonResponse = QJsonDocument::fromJson(m_reply->readAll());
                    QJsonObject jsonObject = jsonResponse.object();

                    if(jsonObject.contains("status") && jsonObject["status"] == "error") {
                        emit tokenValidationError(jsonObject["error"].toString());
                    }
                    else {
                        QString downloadUrl = jsonObject["download_url"].toString();
                        resolve(downloadUrl);
                    }
                }
                m_reply->deleteLater();
            });
        }};
}

QtPromise::QPromise<void> InstallModels::DownloadModels(const QString &url)
{
    return QtPromise::QPromise<void>{[&](const auto resolve, const auto reject)
        {
            emit downloadStarted();

            QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());

            if(QFile::exists(pathAppend(tempPath, "Bluebell.zip")))
            {
                // zip already exists, don't download again
                resolve();
                return;
            }

            m_file = new QSaveFile(pathAppend(tempPath, "Bluebell.zip"));
            if(!m_file->open(QIODevice::WriteOnly))
            {
                reject(QString{"Error opening file for writing. Restart xPilot and try again."});
                return;
            }

            QNetworkRequest networkRequest(url);
            networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

            m_reply = nam->get(networkRequest);
            QObject::connect(m_reply, &QNetworkReply::downloadProgress, [&](qint64 read, qint64 total) {
                double pct = ((double)read / (double)total);
                emit downloadProgressChanged(pct);
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

QtPromise::QPromise<void> InstallModels::UnzipModels(const QString &path)
{
    return QtPromise::QPromise<void>{[&](const auto resolve, const auto reject)
        {
            return QtConcurrent::run([=]
            {
                QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());

                if(!QFile(pathAppend(tempPath, "Bluebell.zip")).exists()) {
                    reject(QString{"Model package not found. Re-launch xPilot and try downloading the CSL models again."});
                    return;
                }

                ZipArchive zf(pathAppend(tempPath, "Bluebell.zip").toStdString());
                if(!zf.open(ZipArchive::ReadOnly))
                {
                    reject(QString{"Error opening file for extracting. Re-launch xPilot and try downloading the CSL models again."});
                    return;
                }

                QString xplanePath = QDir::fromNativeSeparators(pathAppend(path, "Resources/plugins/xPilot/Resources/CSL"));

                std::vector<ZipEntry> entries = zf.getEntries();
                std::vector<ZipEntry>::iterator it;
                ZipEntry entry;

                size_t totalBytes = 0;
                size_t bytesWritten = 0;
                for(it = entries.begin(); it != entries.end(); ++it)
                {
                    entry = *it;
                    if(entry.isFile())
                    {
                        totalBytes += entry.getSize();
                    }
                }

                // create directories
                for(it = entries.begin(); it != entries.end(); ++it)
                {
                    if(m_stopExtract) {
                        zf.close();
                        return;
                    }

                    entry = *it;
                    size_t size = entry.getSize();
                    size_t crc = entry.getCRC();

                    // in some rare cases, a directory might be coded incorrectly in a zip file
                    // and no '/' is appended at the end of its name, hence why we check the size and crc
                    if(entry.isDirectory() || (size == 0 && crc == 0))
                    {
                        QString path(pathAppend(xplanePath, entry.getName().c_str()));
                        QDir().mkdir(path);
                    }
                }

                // create files
                for(it = entries.begin(); it != entries.end(); ++it)
                {
                    if(m_stopExtract) {
                        zf.close();
                        return;
                    }

                    entry = *it;
                    size_t size = entry.getSize();

                    if(entry.isFile())
                    {
                        QString path(pathAppend(xplanePath, entry.getName().c_str()));
                        std::ofstream unzippedFile(path.toStdString(), std::ofstream::binary);
                        if(unzippedFile)
                        {
                            if(entry.readContent(unzippedFile, ZipArchive::Current) == 0)
                            {
                                bytesWritten += size;
                                emit unzipProgressChanged((double)bytesWritten / (double)totalBytes);
                            }
                            else
                            {
                                continue;
                            }
                        }
                        unzippedFile.close();
                    }
                }

                zf.close();
                resolve();
            });
        }};
}

void InstallModels::CreatePluginConfig(const QString &path)
{
    QJsonObject config;

    QString tmp(pathAppend(path, "Resources/plugins/xPilot/Resources/CSL/Bluebell"));
    QString nativePath = QDir::toNativeSeparators(tmp);

    QJsonObject cslPath;
    cslPath.insert("Enabled", true);
    cslPath.insert("Path", nativePath);

    QJsonArray paths;
    paths.push_back(cslPath);

    config.insert("CSL", paths);

    QJsonDocument doc(config);

    QFile configFile(pathAppend(path, "Resources/plugins/xPilot/Resources/Config.json"));
    configFile.open(QFile::WriteOnly);
    configFile.write(doc.toJson());
}

void InstallModels::DeleteTempDownload()
{
    QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
    QFile tempFile(pathAppend(tempPath, "Bluebell.zip"));
    if(!tempFile.remove()) {
        emit errorEncountered("Delete temporary download error: " + tempFile.errorString());
    }
}

void InstallModels::checkIfZipExists()
{
    QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
    if(QFile::exists(pathAppend(tempPath, "Bluebell.zip")))
    {
        // zip already exists, don't download again
        emit setXplanePath();
    }
}

void InstallModels::downloadModels(QString token, QString vatsimId)
{
    ValidateToken("https://xpilot-project.org/ValidateCdnAuth", vatsimId, token).then([&](const QString& url){
        DownloadModels(url).then([&]{
            emit setXplanePath();
        }).fail([&](const QString& err){
            errorEncountered("Download error: " + err);
        });
    });
}

void InstallModels::validatePath(QString path)
{
    bool pathValid = false;

    QString localPath = QUrl(QDir::fromNativeSeparators(path)).toLocalFile();
    QDir xplanePath(localPath);

    // instead of checking if the directory is readable (because that's not a reliable method according to the qt docs), create a temporary file to verify permissions instead
    QTemporaryFile temp(xplanePath.path() + "/");
    if(!temp.open()) {
        emit invalidXplanePath("The X-Plane folder is not readable. Verify the folder permissions and try again.");
        return;
    }

    if(BuildConfig::isRunningOnWindowsPlatform()) {
        QString xplaneExe = pathAppend(xplanePath.path(), "X-Plane.exe");
        pathValid = QFileInfo::exists(xplaneExe) && QFileInfo(xplaneExe).isFile();
        if(!pathValid) {
            emit invalidXplanePath("Invalid X-Plane folder path. The path should be the root folder of where X-Plane.exe is installed.");
            return;
        }
    }
    else if(BuildConfig::isRunningOnMacOSPlatform()) {
        QString xplaneExe = pathAppend(xplanePath.path(), "X-Plane.app");
        pathValid = QFileInfo::exists(xplaneExe) && QFileInfo(xplaneExe).isDir();
        if(!pathValid) {
            emit invalidXplanePath("Invalid X-Plane folder path. The path should be the root folder of where X-Plane.app is installed.");
            return;
        }
    }
    else if(BuildConfig::isRunningOnLinuxPlatform()) {
        QString xplaneExe = pathAppend(xplanePath.path(), "X-Plane-x86_64");
        pathValid = QFileInfo::exists(xplaneExe) && QFileInfo(xplaneExe).isFile();
        if(!pathValid) {
            emit invalidXplanePath("Invalid X-Plane folder path. The path should be the root folder of where the X-Plane-x86_64 executable is installed.");
            return;
        }
    }

    bool pluginValid = false;
    QString pluginError = "xPilot plugin not found. Please re-run the xPilot installer and make sure to choose the correct X-Plane folder path.";

    QDir xpilotPath(pathAppend(xplanePath.path(), "Resources/plugins/xPilot"));

    // instead of checking if the directory is readable (because that's not a reliable method according to the qt docs), create a temporary file to verify permissions instead
    QTemporaryFile temp2(xpilotPath.path() + "/");
    if(!temp2.open()) {
        QString err("The xPilot plugin resources folder (%1) is not readable. Verify the folder permissions and try again.");
        emit invalidXplanePath(err.arg(xpilotPath.path()));
        return;
    }

    if(BuildConfig::isRunningOnWindowsPlatform()) {
        QString pluginFile = pathAppend(xpilotPath.path(), "win_x64/xPilot.xpl");
        pluginValid = QFileInfo::exists(pluginFile) && QFileInfo(pluginFile).isFile();
        if(!pluginValid) {
            emit invalidXplanePath(pluginError);
            return;
        }
    }
    else if(BuildConfig::isRunningOnMacOSPlatform()) {
        QString pluginFile = pathAppend(xpilotPath.path(), "mac_x64/xPilot.xpl");
        pluginValid = QFileInfo::exists(pluginFile) && QFileInfo(pluginFile).isFile();
        if(!pluginValid) {
            emit invalidXplanePath(pluginError);
            return;
        }
    }
    else if(BuildConfig::isRunningOnLinuxPlatform()) {
        QString pluginFile = pathAppend(xpilotPath.path(), "lin_x64/xPilot.xpl");
        pluginValid = QFileInfo::exists(pluginFile) && QFileInfo(pluginFile).isFile();
        if(!pluginValid) {
            emit invalidXplanePath(pluginError);
            return;
        }
    }

    if(pathValid && pluginValid) {
        QString xpPath = xplanePath.path();
        emit validXplanePath();
        UnzipModels(xpPath).then([=]{
            CreatePluginConfig(xpPath);
            DeleteTempDownload();
            emit unzipFinished();
        })
                .fail([&](const QString &err){
            emit errorEncountered("Unzip error: " + err);
        });
    }
}

void InstallModels::cancel()
{
    if(m_file) {
        m_file->cancelWriting();
        m_file->deleteLater();
    }

    if(m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }

    m_stopExtract = true;
}

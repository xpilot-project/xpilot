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

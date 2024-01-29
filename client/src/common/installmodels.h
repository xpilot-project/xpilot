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

#ifndef INSTALLMODELS_H
#define INSTALLMODELS_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QNetworkAccessManager>
#include <QtPromise>
#include <QSaveFile>

class InstallModels : public QObject
{
    Q_OBJECT

public:
    InstallModels(QObject *parent = nullptr);
    ~InstallModels();

    QtPromise::QPromise<QString> GetAuthToken();
    QtPromise::QPromise<QString> ValidateAuthToken(const QString& token);
    QtPromise::QPromise<void> DownloadModels(const QString& url);
    QtPromise::QPromise<void> UnzipModels(const QString &path);
    void CreatePluginConfig(const QString &path);
    void DeleteTempDownload();
    Q_INVOKABLE void downloadModels();
    Q_INVOKABLE void validatePath(const QString path);
    Q_INVOKABLE void cancel();

signals:
    void downloadProgressChanged(double value);
    void setXplanePath();
    void invalidXplanePath(QString errorText);
    void validXplanePath();
    void unzipProgressChanged(double value);
    void unzipFinished();
    void errorEncountered(QString error);
    void tokenValidationError(QString error);
    void downloadStarted();

private:
    QNetworkAccessManager *nam = nullptr;
    QPointer<QNetworkReply> m_reply = nullptr;
    QPointer<QSaveFile> m_file = nullptr;
    bool m_stopExtract = false;
};

#endif // INSTALLMODELS_H

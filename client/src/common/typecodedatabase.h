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

#ifndef TYPECODEDATABASE_H
#define TYPECODEDATABASE_H

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

struct TypeCodeInfo
{
    QString TypeCode;
    QString Name;
    QString Manufacturer;
};

Q_DECLARE_METATYPE(TypeCodeInfo)

class TypeCodeDatabase : public QObject
{
    Q_OBJECT

public:
    TypeCodeDatabase(QObject *parent = nullptr);

    void PerformTypeCodeDownload();
    QtPromise::QPromise<QString> GetTypeCodeUrl();
    QtPromise::QPromise<void> DownloadTypeCodes(QString url);
    Q_INVOKABLE void searchTypeCodes(QString predicate);
    Q_INVOKABLE void validateTypeCodeBeforeConnect(QString typeCode);

signals:
    void typeCodeDownloadError(QString error);
    void typeCodeResults(QVariantList typeCodes);
    void validateTypeCode(bool valid);

private:
    QNetworkAccessManager *nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    QPointer<QSaveFile> m_file;
    QList<TypeCodeInfo> m_typeCodes;
};

#endif // TYPECODEDATABASE_H

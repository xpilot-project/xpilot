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

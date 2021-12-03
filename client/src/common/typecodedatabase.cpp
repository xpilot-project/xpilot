#include "typecodedatabase.h"
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

using namespace xpilot;
using namespace QtPromise;

QByteArray fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile sourceFile(fileName);
    qint64 fileSize = sourceFile.size();
    const qint64 bufferSize = 10240;

    if (sourceFile.open(QIODevice::ReadOnly))
    {
        char buffer[bufferSize];
        int bytesRead;
        int readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash(hashAlgorithm);
        while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0)
        {
            fileSize -= bytesRead;
            hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        sourceFile.close();
        return hash.result();
    }
    return QByteArray();
}

TypeCodeDatabase::TypeCodeDatabase(QObject *parent) :
    QObject(parent),
    nam(new QNetworkAccessManager)
{

}

void TypeCodeDatabase::PerformTypeCodeDownload()
{
    GetTypeCodeUrl().then([&](QString url) {
        DownloadTypeCodes(url).then([&] {
            QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
            QFile file(pathAppend(tempPath, "TypeCodes.json"));
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                auto doc = QJsonDocument::fromJson(file.readAll()).array();
                for(const QJsonValue &v : doc)
                {
                    TypeCodeInfo typeCode{};
                    typeCode.Name = v["Name"].toString();
                    typeCode.Manufacturer = v["Manufacturer"].toString();
                    typeCode.TypeCode = v["TypeCode"].toString();
                    m_typeCodes.append(typeCode);
                }
            }
            file.close();
        });
    });
}

QPromise<QString> TypeCodeDatabase::GetTypeCodeUrl()
{
    return QPromise<QString>{[&](const auto resolve, const auto reject)
        {
            QString url("https://xpilot-project.org/api/v3/TypeCodes");
            QNetworkReply *reply = nam->get(QNetworkRequest{url});

            QObject::connect(reply, &QNetworkReply::finished, [=]() {
                if(reply->error() == QNetworkReply::NoError)
                {
                    QByteArray response = reply->readAll();
                    if(!response.isEmpty()) {
                        auto json = QJsonDocument::fromJson(response).object();

                        QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
                        auto hash = fileChecksum(pathAppend(tempPath, "TypeCodes.json"), QCryptographicHash::Sha256);
                        if(!hash.isEmpty() && hash.toHex() != json["Hash"].toString()) {
                            resolve(json["TypeCodesUrl"].toString());
                        }
                        else {
                            resolve(""); // unchanged, don't download again
                        }
                    }
                    else {
                        reject(QString{"Could not download aircraft typecode database. Download URL not found."});
                    }
                }
                else
                {
                    reject(QString{reply->errorString()});
                }
            });
        }};
}

QPromise<void> TypeCodeDatabase::DownloadTypeCodes(QString url)
{
    return QPromise<void>{[&](const auto resolve, const auto reject)
        {
            if(url.isEmpty())
            {
                // if url is empty, it probably means the local file is up to date
                resolve();
                return;
            }

            QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
            QSaveFile *typeCodes = new QSaveFile(pathAppend(tempPath, "TypeCodes.json"));

            if(!typeCodes->open(QIODevice::WriteOnly))
            {
                return;
            }

            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            QPointer<QNetworkReply> reply = nam->get(req);

            QObject::connect(reply, &QNetworkReply::readyRead, [&]{
                if(typeCodes) {
                    typeCodes->write(reply->readAll());
                }
            });
            QObject::connect(reply, &QNetworkReply::finished, [=]() {
                if(reply->error() == QNetworkReply::NoError) {
                    if(typeCodes){
                        typeCodes->write(reply->readAll());
                        typeCodes->commit();
                    }
                    resolve();
                }
                else {
                    if(reply->error() != QNetworkReply::OperationCanceledError) {
                        emit typeCodeDownloadError("Error downloading aircraft type code database: " + reply->errorString());
                        reject();
                    }
                }
                reply->deleteLater();
            });
        }};
}

void TypeCodeDatabase::searchTypeCodes(QString predicate)
{
    QList<TypeCodeInfo> results;
    std::copy_if(m_typeCodes.begin(), m_typeCodes.end(), std::back_inserter(results), [&](TypeCodeInfo info) {
        return info.TypeCode.toUpper().contains(predicate.toUpper())
                || info.Name.toUpper().contains(predicate.toUpper())
                || info.Manufacturer.toUpper().contains(predicate.toUpper());
    });

    QVariantList variantList;
    for(const auto& v : std::as_const(results))
    {
        QVariantMap mapItem;
        mapItem.insert("name", v.Name);
        mapItem.insert("typeCode", v.TypeCode);
        mapItem.insert("manufacturer", v.Manufacturer);
        variantList.append(mapItem);
    }
    results.clear();

    emit typeCodeResults(variantList);
}

void TypeCodeDatabase::validateTypeCodeBeforeConnect(QString typeCode)
{
    auto result = std::find_if(m_typeCodes.begin(), m_typeCodes.end(), [&](TypeCodeInfo info){
        return info.TypeCode.toUpper() == typeCode.toUpper();
    });

    emit validateTypeCode(result != m_typeCodes.end());
}

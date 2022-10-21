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
#include <QCryptographicHash>

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
    }).fail([&](QString error) {
        emit typeCodeDownloadError(error);
    });
}

QtPromise::QPromise<QString> TypeCodeDatabase::GetTypeCodeUrl()
{
    return QtPromise::QPromise<QString>{[&](const auto resolve, const auto reject)
        {
            QString url("https://xpilot-project.org/api/v3/TypeCodes");

            QNetworkRequest networkRequest(url);
            networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            m_reply = nam->get(networkRequest);

            QObject::connect(m_reply, &QNetworkReply::finished, [=]() {
                if(m_reply->error() == QNetworkReply::NoError)
                {
                    QByteArray response = m_reply->readAll();
                    if(!response.isEmpty()) {
                        auto json = QJsonDocument::fromJson(response).object();

                        QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
                        auto hash = fileChecksum(pathAppend(tempPath, "TypeCodes.json"), QCryptographicHash::Sha256);
                        if(hash.isEmpty() || hash.toHex() != json["Hash"].toString()) {
                            resolve(json["TypeCodesUrl"].toString());
                        }
                        else {
                            resolve(""); // unchanged, don't download again
                        }
                    }
                    else {
                        reject(QString{"Could not download aircraft typecode database - response is empty."});
                    }
                }
                else
                {
                    reject(QString{"Aircraft Type Code Database Error: " + m_reply->errorString()});
                }
            });
        }};
}

QtPromise::QPromise<void> TypeCodeDatabase::DownloadTypeCodes(QString url)
{
    return QtPromise::QPromise<void>{[&](const auto resolve, const auto reject)
        {
            if(url.isEmpty())
            {
                // if url is empty, it probably means the local file is up to date
                resolve();
                return;
            }

            QString tempPath = QDir::fromNativeSeparators(AppConfig::dataRoot());
            m_file = new QSaveFile(pathAppend(tempPath, "TypeCodes.json"));

            if(!m_file->open(QIODevice::WriteOnly | QIODevice::Text))
            {
                emit typeCodeDownloadError("Error creating aircraft database file: " + m_file->errorString());
                return;
            }

            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
            m_reply = nam->get(req);

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
                    }
                    resolve();
                }
                else {
                    if(m_reply->error() != QNetworkReply::OperationCanceledError) {
                        emit typeCodeDownloadError("Error downloading aircraft type code database: " + m_reply->errorString());
                        reject();
                    }
                }
                m_reply->deleteLater();
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

    // limit results to no more than 10 type codes
    QList<TypeCodeInfo> limitedResults(results.begin(), std::next(results.begin(), std::min(10, (int)results.size())));

    QVariantList variantList;
    for(const auto& v : qAsConst(limitedResults))
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

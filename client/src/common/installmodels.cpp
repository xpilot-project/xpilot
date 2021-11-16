#include "installmodels.h"
#include "libzippp.h"
#include "utils.h"

#include <fstream>

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QtConcurrent/QtConcurrent>
#include <QtPromise>
#include <QNetworkReply>

using namespace libzippp;

InstallModels::InstallModels(QObject *parent) :
    QObject(parent),
    nam(new QNetworkAccessManager)
{
    //    m_worker = new ModelWorker(this);

    //    connect(m_worker, &ModelWorker::updateProgress, [&](double total, double written){
    //        qDebug() << (written / total)*100 << "%";
    //    });
}

InstallModels::~InstallModels()
{

}

void UnzipProgressCallback(double dFileSize, double dBytesCount)
{
    if(dFileSize <= 0.0) return;

    double dFractionProcessed = dBytesCount / dFileSize;

    qDebug() << static_cast<unsigned>(dFractionProcessed * 100) << "%";
}

void InstallModels::DoExtractModels()
{
    //    QFuture<void> future = QtConcurrent::run(m_worker, &ModelWorker::asyncFunction);
}

QtPromise::QPromise<void> InstallModels::download(const QString &url)
{
    return QtPromise::QPromise<void>{[&](const auto &resolve, const auto &reject)
        {
            m_file = new QSaveFile("C:/X-Plane 11.50/Resources/plugins/xPilot/Resources/CSL/Bluebell.json");
            if(!m_file->open(QIODevice::WriteOnly))
            {
                reject("Error opening file for writing");
                return;
            }

            m_reply = nam->get(QNetworkRequest{url});
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
                m_reply->deleteLater();
            });
        }};
}

void InstallModels::downloadModels()
{
    download("https://cdn.xpilot-project.org/CSL/Bluebell.zip").then([&](){
        emit downloadFinished();
    }).fail([](QNetworkReply::NetworkError error) {
        // error
    }).fail([](QString err) {
        qDebug() << "ERROR: " << err;
    });
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
}

ModelWorker::ModelWorker(QObject *parent) :
    QObject(parent),
    m_stop(false)
{

}

void ModelWorker::stop()
{
    m_stop = true;
}

void ModelWorker::asyncFunction()
{
    qDebug() << "Worker thread: " << this->thread()->currentThreadId();

    QString xplanePath = QDir::fromNativeSeparators("C:\\X-Plane 11.50\\Resources\\plugins\\xPilot\\Resources\\CSL");

    ZipArchive zf("C:/Users/Justin/Downloads/Bluebell.zip");
    qDebug() << zf.open(ZipArchive::ReadOnly);

    std::vector<ZipEntry> entries = zf.getEntries();
    std::vector<ZipEntry>::iterator it;
    ZipEntry entry;

    size_t totalSize = 0;
    size_t bytesWritten = 0;
    for(it = entries.begin(); it != entries.end(); ++it)
    {
        entry = *it;
        if(entry.isFile())
        {
            totalSize += entry.getSize();
        }
    }

    // create directories
    for(it = entries.begin(); it != entries.end(); ++it)
    {
        if(m_stop) return;

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
        if(m_stop) return;

        entry = *it;
        size_t size = entry.getSize();
        size_t crc = entry.getCRC();

        if(entry.isFile())
        {
            QString path(pathAppend(xplanePath, entry.getName().c_str()));
            std::ofstream unzippedFile(path.toStdString(), std::ofstream::binary);
            if(unzippedFile)
            {
                if(entry.readContent(unzippedFile, ZipArchive::Current, 32 * 20 * 820) == 0)
                {
                    bytesWritten += size;
                    emit updateProgress(totalSize, bytesWritten);
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
}

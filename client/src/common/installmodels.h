#ifndef INSTALLMODELS_H
#define INSTALLMODELS_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QNetworkAccessManager>
#include <QtPromise>
#include <QSaveFile>

typedef std::function<void(double, double)> ProgressCallback;

class ModelWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModelWorker(QObject *parent = nullptr);

public slots:
    void stop();
    void asyncFunction();

signals:
    void updateProgress(double totalBytes, double bytesWritten);

private:
    bool m_stop;
};

class InstallModels : public QObject
{
    Q_OBJECT

public:
    InstallModels(QObject *parent = nullptr);
    ~InstallModels();

    QtPromise::QPromise<void> download(const QString& url);
    QtPromise::QPromise<void> extractModelSet();
    Q_INVOKABLE void downloadModels();
    Q_INVOKABLE void cancel();

signals:
    void downloadFinished();
    void downloadProgressChanged(double value);

private:
    QNetworkAccessManager *nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QSaveFile *m_file = nullptr;
    bool m_stopExtract = false;
};

#endif // INSTALLMODELS_H

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

    void DoExtractModels();
    QtPromise::QPromise<void> download(const QString& url);
    Q_INVOKABLE void downloadModels();
    Q_INVOKABLE void cancel();

private:
    QNetworkAccessManager nam;
    QNetworkReply *m_reply = nullptr;
    QSaveFile *m_file = nullptr;
};

#endif // INSTALLMODELS_H

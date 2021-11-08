#pragma once

#include <QObject>

class QQmlApplicationEngine;
class QQmlEngine;
class QJSEngine;

namespace xpilot
{
    int Main(int argc, char* argv[]);

    class AppCore : public QObject
    {
        Q_OBJECT

    public:
        AppCore(QQmlEngine* qmlEngine);
        virtual ~AppCore() {}

        static QObject* appConfigInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

    signals:
        void serverListDownloaded(int count);
        void serverListDownloadError();

    private:
        QQmlApplicationEngine* engine;
        void DownloadServerList();
        void PerformVersionCheck();
    };
}

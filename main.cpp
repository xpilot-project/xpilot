#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QProcess>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <qicon.h>

class InterProcess: public QObject
{
public:
    InterProcess()
    {
        qDebug("Starting IPC process");

        process.setProgram("QtProcess/QtProcess.exe");
        process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        process.start();
    }

    ~InterProcess()
    {
        qDebug("Terminating IPC process");
        process.kill();
    }

    void tick() {
        process.write("\n");
        qDebug() << "Recieved: " + process.readAllStandardOutput();
    }

private:
    QProcess process;
};

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

//    InterProcess ipc;
//    QTimer timer(&app);
//    QCoreApplication::connect(&timer, &QTimer::timeout, &ipc, &InterProcess::tick);
//    timer.start(50);

    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/Resources/Views/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

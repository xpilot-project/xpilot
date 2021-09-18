#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QProcess>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <QQmlContext>
#include <qicon.h>
#include <QObject>
#include <QQuickWindow>
#include "interprocess.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    InterProcess ipc;
    context->setContextProperty("ipc", &ipc);

    const QUrl url(QStringLiteral("qrc:/Resources/Views/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    QObject *root = engine.rootObjects().first();
    QObject::connect(root, SIGNAL(setTransponderCode(int)), &ipc, SLOT(onSetTransponderCode(int)));
    QObject::connect(root, SIGNAL(setRadioStack(int, int)), &ipc, SLOT(onSetRadioStack(int, int)));

    return app.exec();
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

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
#include <QSslSocket>

#include "appcore.h"
#include "network/networkmanager.h"
#include "network/networkserverlist.h"

using namespace xpilot;

static QObject *singletonTypeProvider(QQmlEngine *, QJSEngine *)
{
    return AppConfig::getInstance();
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    AppCore appCore;
    NetworkManager networkManager;
    NetworkServerList serverList;

    QObject::connect(&app, SIGNAL(aboutToQuit()), &appCore, SLOT(SaveConfig()));

    context->setContextProperty("appCore", &appCore);
    context->setContextProperty("networkManager", &networkManager);
    context->setContextProperty("serverList", &serverList);
    qmlRegisterSingletonType<AppConfig>("AppConfig", 1, 0, "AppConfig", singletonTypeProvider);
    qRegisterMetaType<ConnectInfo>("ConnectInfo");

    const QUrl url(QStringLiteral("qrc:/Resources/Views/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

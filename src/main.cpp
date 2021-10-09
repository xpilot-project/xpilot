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
#include "audio/afv.h"
#include "controllers/controller_manager.h"
#include "network/networkmanager.h"
#include "network/networkserverlist.h"
#include "simulator/udpclient.h"
#include "aircrafts/user_aircraft_manager.h"
#include "aircrafts/radio_stack_state.h"
#include "version.h"

using namespace xpilot;

static QObject *singletonTypeProvider(QQmlEngine *, QJSEngine *)
{
    return AppConfig::getInstance();
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setApplicationVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH));

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    AppCore appCore;
    UdpClient udpClient;
    NetworkManager networkManager(udpClient);
    ControllerManager controllerManager(networkManager);
    NetworkServerList serverList;
    UserAircraftManager aircraftManager(udpClient, networkManager);

#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
#endif

    struct event_base* ev_base = nullptr;
    ev_base = event_base_new();
    AudioForVatsim audio(ev_base);

    QObject::connect(&app, SIGNAL(aboutToQuit()), &appCore, SLOT(SaveConfig()));

    context->setContextProperty("appCore", &appCore);
    context->setContextProperty("networkManager", &networkManager);
    context->setContextProperty("udpClient", &udpClient);
    context->setContextProperty("serverList", &serverList);
    context->setContextProperty("controllerManager", &controllerManager);
    qmlRegisterSingletonType<AppConfig>("AppConfig", 1, 0, "AppConfig", singletonTypeProvider);
    qRegisterMetaType<ConnectInfo>("ConnectInfo");
    qRegisterMetaType<ClientWindowConfig>("ClientWindowConfig");
    qRegisterMetaType<RadioStackState>("RadioStackState");

    const QUrl url(QStringLiteral("qrc:/Resources/Views/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "appcore.h"
#include "version.h"
#include "config/appconfig.h"
#include "controllers/controller_manager.h"
#include "network/networkmanager.h"
#include "network/networkserverlist.h"
#include "simulator/xplane_adapter.h"
#include "aircrafts/user_aircraft_manager.h"
#include "aircrafts/network_aircraft_manager.h"
#include "aircrafts/radio_stack_state.h"
#include "audio/afv.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QProcess>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <QQmlContext>
#include <QIcon>
#include <QObject>
#include <QQuickWindow>
#include <QSslSocket>

using namespace xpilot;

int xpilot::Main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QCoreApplication::setOrganizationName(VER_COMPANYNAME_STR);
    QCoreApplication::setOrganizationDomain("xpilot-project.org");
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH));

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    AppCore appCore(&engine);
    XplaneAdapter xplaneAdapter;
    NetworkManager networkManager(xplaneAdapter);
    AircraftManager networkAircraftManager(networkManager, xplaneAdapter);
    ControllerManager controllerManager(networkManager);
    UserAircraftManager aircraftManager(xplaneAdapter, networkManager);
    AudioForVatsim audio(networkManager, xplaneAdapter);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        networkManager.disconnectFromNetwork();
        xplaneAdapter.DeleteAllAircraft();
        AppConfig::getInstance()->saveConfig();
    });

    qmlRegisterSingletonType<AppConfig>("AppConfig", 1, 0, "AppConfig", &AppCore::appConfigInstance);
    qRegisterMetaType<ConnectInfo>("ConnectInfo");
    qRegisterMetaType<ClientWindowConfig>("ClientWindowConfig");
    qRegisterMetaType<RadioStackState>("RadioStackState");
    qRegisterMetaType<AudioDeviceInfo>("AudioDeviceInfo");

    context->setContextProperty("appCore", &appCore);
    context->setContextProperty("networkManager", &networkManager);
    context->setContextProperty("xplaneAdapter", &xplaneAdapter);
    context->setContextProperty("controllerManager", &controllerManager);
    context->setContextProperty("audio", &audio);

    const QUrl url(QStringLiteral("qrc:/Resources/Views/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

AppCore::AppCore(QQmlEngine* qmlEngine) :
    QObject(qmlEngine),
    engine(qobject_cast<QQmlApplicationEngine*>(qmlEngine))
{
    QTimer::singleShot(0, this, [this]{
        DownloadServerList();
    });
}

QObject *AppCore::appConfigInstance(QQmlEngine*, QJSEngine*)
{
    return AppConfig::getInstance();
}

void AppCore::DownloadServerList()
{
    NetworkServerList networkServerList;
    auto serverList = networkServerList.DownloadServerList("https://data.vatsim.net/v3/vatsim-servers.json");

    if(serverList.size() > 0) {
        emit serverListDownloaded(serverList.size());
        AppConfig::getInstance()->CachedServers.clear();
        for(auto & server: serverList) {
            AppConfig::getInstance()->CachedServers.append(server);
        }
        AppConfig::getInstance()->saveConfig();
    }
    else {
        emit serverListDownloadError();
    }
}

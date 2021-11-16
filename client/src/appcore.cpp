#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "appcore.h"
#include "config/appconfig.h"
#include "controllers/controller_manager.h"
#include "network/networkmanager.h"
#include "network/networkserverlist.h"
#include "simulator/xplane_adapter.h"
#include "aircrafts/user_aircraft_manager.h"
#include "aircrafts/network_aircraft_manager.h"
#include "aircrafts/radio_stack_state.h"
#include "audio/afv.h"
#include "common/build_config.h"
#include "common/versioncheck.h"
#include "common/installmodels.h"
#include "sentry.h"

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
#include <QScopeGuard>

using namespace xpilot;

int xpilot::Main(int argc, char* argv[])
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, BuildConfig::getSentryDsn().toStdString().c_str());
    sentry_options_set_release(options, BuildConfig::getVersionString().toStdString().c_str());
    sentry_init(options);

    auto sentryClose = qScopeGuard([]{ sentry_close(); });

    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QCoreApplication::setApplicationName("xPilot");
    QCoreApplication::setApplicationVersion(xpilot::BuildConfig::getVersionString());

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    AppCore appCore(&engine);
    InstallModels installModels;
    XplaneAdapter xplaneAdapter;
    NetworkManager networkManager(xplaneAdapter);
    AircraftManager networkAircraftManager(networkManager, xplaneAdapter);
    ControllerManager controllerManager(networkManager, xplaneAdapter);
    UserAircraftManager aircraftManager(xplaneAdapter, networkManager);
    AudioForVatsim audio(networkManager, xplaneAdapter, controllerManager);

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
    context->setContextProperty("appVersion", BuildConfig::getVersionString());
    context->setContextProperty("isVelocityEnabled", AppConfig::getInstance()->VelocityEnabled);
    context->setContextProperty("installModels", &installModels);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        networkManager.disconnectFromNetwork();
        xplaneAdapter.DeleteAllAircraft();
        AppConfig::getInstance()->saveConfig();
    });

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
        PerformVersionCheck();
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

void AppCore::PerformVersionCheck()
{
    VersionCheck versionCheck;

    connect(&versionCheck, &VersionCheck::newVersionAvailable, [=](QString version, QString url)
    {
        emit newVersionAvailable(version, url);
    });
    connect(&versionCheck, &VersionCheck::noUpdatesAvailable, [=](){
       emit noUpdatesAvailable();
    });

    versionCheck.checkForUpdates();
}

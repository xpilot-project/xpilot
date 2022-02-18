#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "appcore.h"
#include "config/appconfig.h"
#include "controllers/controller_manager.h"
#include "network/networkmanager.h"
#include "network/serverlistmanager.h"
#include "simulator/xplane_adapter.h"
#include "aircrafts/user_aircraft_manager.h"
#include "aircrafts/network_aircraft_manager.h"
#include "aircrafts/radio_stack_state.h"
#include "audio/afv.h"
#include "common/build_config.h"
#include "common/versioncheck.h"
#include "common/typecodedatabase.h"
#include "common/installmodels.h"
#include "common/runguard.h"
#include "common/utils.h"
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
#include <QFont>
#include <QFontDatabase>
#include <QCommandLineParser>

using namespace xpilot;

static QObject *appConfigSingleton(QQmlEngine *, QJSEngine *)
{
    return AppConfig::getInstance();
}

int xpilot::Main(int argc, char* argv[])
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, BuildConfig::getSentryDsn().toStdString().c_str());
    sentry_options_set_release(options, BuildConfig::getVersionString().toStdString().c_str());
    sentry_init(options);

    auto sentryClose = qScopeGuard([]{ sentry_close(); });

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QGuiApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QCoreApplication::setApplicationName("xPilot");
    QCoreApplication::setApplicationVersion(xpilot::BuildConfig::getVersionString());
    QCoreApplication::setOrganizationName("Justin Shannon");
    QCoreApplication::setOrganizationDomain("org.vatsim.xpilot");

    RunGuard guard("org.vatsim.xpilot");
    if(!guard.tryToRun()) {
        return 0;
    }

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(app);

    app.setWindowIcon(QIcon(":/Resources/Icons/AppIcon.ico"));

    QFontDatabase database;
    if(!database.families().contains("Ubuntu"))
    {
        // we must check if Ubuntu is already instead (at least for Linux), otherwise the FileDialog gets all corrupted...
        QFontDatabase::addApplicationFont(":/Resources/Fonts/Ubuntu-Regular.ttf");
    }
    app.setFont(QFont("Ubuntu", 10));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    ServerListManager serverListManager;
    VersionCheck versionCheck;
    TypeCodeDatabase typeCodeDatabase;
    InstallModels installModels;
    XplaneAdapter xplaneAdapter;
    NetworkManager networkManager(xplaneAdapter);
    AircraftManager networkAircraftManager(networkManager, xplaneAdapter);
    ControllerManager controllerManager(networkManager, xplaneAdapter);
    UserAircraftManager aircraftManager(xplaneAdapter, networkManager);
    AudioForVatsim audio(networkManager, xplaneAdapter, controllerManager);

    QTimer::singleShot(500, [&](){
        serverListManager.PerformServerListDownload("https://data.vatsim.net/v3/vatsim-servers.json");
        versionCheck.PerformVersionCheck();
        typeCodeDatabase.PerformTypeCodeDownload();
    });

    qmlRegisterSingletonType<AppConfig>("AppConfig", 1, 0, "AppConfig", appConfigSingleton);
    qRegisterMetaType<ConnectInfo>("ConnectInfo");
    qRegisterMetaType<ClientWindowConfig>("ClientWindowConfig");
    qRegisterMetaType<RadioStackState>("RadioStackState");
    qRegisterMetaType<AudioDeviceInfo>("AudioDeviceInfo");
    qRegisterMetaType<TypeCodeInfo>("TypeCodeInfo");

    context->setContextProperty("networkManager", &networkManager);
    context->setContextProperty("xplaneAdapter", &xplaneAdapter);
    context->setContextProperty("controllerManager", &controllerManager);
    context->setContextProperty("audio", &audio);
    context->setContextProperty("appVersion", BuildConfig::getVersionString());
    context->setContextProperty("installModels", &installModels);
    context->setContextProperty("versionCheck", &versionCheck);
    context->setContextProperty("serverListManager", &serverListManager);
    context->setContextProperty("typeCodeDatabase", &typeCodeDatabase);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        networkManager.disconnectFromNetwork();
        xplaneAdapter.DeleteAllAircraft();
        xplaneAdapter.DeleteAllControllers();
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

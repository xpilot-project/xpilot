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
#include <QIcon>
#include <QObject>
#include <QQuickWindow>
#include <QSslSocket>
#include <QScopeGuard>
#include <QFont>
#include <QFontDatabase>
#include <QCommandLineParser>

#include "src/appcore.h"
#include "src/common/build_config.h"
#include "src/common/versioncheck.h"
#include "src/common/typecodedatabase.h"
#include "src/common/installmodels.h"
#include "src/common/runguard.h"
#include "src/common/clipboardadapter.h"
#include "src/config/appconfig.h"
#include "src/audio/afv.h"
#include "src/controllers/controller_manager.h"
#include "src/network/networkmanager.h"
#include "src/network/serverlistmanager.h"
#include "src/simulator/xplane_adapter.h"
#include "src/aircrafts/user_aircraft_manager.h"
#include "src/aircrafts/network_aircraft_manager.h"
#include "src/aircrafts/radio_stack_state.h"
#include "src/qinjection/dependencypool.h"

using namespace xpilot;

static QObject *appConfigSingleton(QQmlEngine *, QJSEngine *)
{
    return AppConfig::getInstance();
}

int xpilot::Main(int argc, char* argv[])
{
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

    auto families = QFontDatabase::families();
    if(!families.contains("Ubuntu")) {
        // We must check if Ubuntu is already instead (at least for Linux), otherwise the FileDialog gets all corrupted...
        QFontDatabase::addApplicationFont(":/Resources/Fonts/Ubuntu-Regular.ttf");
    }
    app.setFont(QFont("Ubuntu", 10));

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    ClipboardAdapter clipboardAdapter;
    ServerListManager serverListManager;
    VersionCheck versionCheck;
    TypeCodeDatabase typeCodeDatabase;
    InstallModels installModels;

    QInjection::addSingleton(new XplaneAdapter);
    QInjection::addSingleton(new NetworkManager);
    QInjection::addSingleton(new AircraftManager);
    QInjection::addSingleton(new ControllerManager);
    QInjection::addSingleton(new UserAircraftManager);
    QInjection::addSingleton(new AudioForVatsim);

    QTimer::singleShot(500, [&] {
        serverListManager.PerformServerListDownload("https://status.vatsim.net/status.json");
        versionCheck.PerformVersionCheck();
        typeCodeDatabase.PerformTypeCodeDownload();
    });

    qmlRegisterSingletonType<AppConfig>("AppConfig", 1, 0, "AppConfig", appConfigSingleton);
    qRegisterMetaType<ConnectInfo>("ConnectInfo");
    qRegisterMetaType<ClientWindowConfig>("ClientWindowConfig");
    qRegisterMetaType<RadioStackState>("RadioStackState");
    qRegisterMetaType<AudioDeviceInfo>("AudioDeviceInfo");
    qRegisterMetaType<TypeCodeInfo>("TypeCodeInfo");

    context->setContextProperty("clipboard", &clipboardAdapter);
    context->setContextProperty("networkManager", QInjection::Pointer<NetworkManager>());
    context->setContextProperty("xplaneAdapter", QInjection::Pointer<XplaneAdapter>());
    context->setContextProperty("controllerManager", QInjection::Pointer<ControllerManager>());
    context->setContextProperty("audio", QInjection::Pointer<AudioForVatsim>());
    context->setContextProperty("appVersion", BuildConfig::getVersionString());
    context->setContextProperty("installModels", &installModels);
    context->setContextProperty("versionCheck", &versionCheck);
    context->setContextProperty("serverListManager", &serverListManager);
    context->setContextProperty("typeCodeDatabase", &typeCodeDatabase);
    context->setContextProperty("appDataPath", AppConfig::getInstance()->dataRoot());

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        QInjection::Pointer<NetworkManager> network;
        QInjection::Pointer<XplaneAdapter> xplaneAdapter;

        network->disconnectFromNetwork();
        xplaneAdapter->DeleteAllAircraft();
        xplaneAdapter->DeleteAllControllers();

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

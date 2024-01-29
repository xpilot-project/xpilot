/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2024 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

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
#include <QtQml/qqml.h>

#include <qinjection/dependencypool.h>

#include "appcore.h"
#include "common/enums.h"
#include "common/build_config.h"
#include "common/versioncheck.h"
#include "common/typecodedatabase.h"
#include "common/installmodels.h"
#include "common/runguard.h"
#include "common/clipboardadapter.h"
#include "config/appconfig.h"
#include "audio/afv.h"
#include "controllers/controller_manager.h"
#include "network/networkmanager.h"
#include "network/serverlistmanager.h"
#include "simulator/xplane_adapter.h"
#include "aircrafts/user_aircraft_manager.h"
#include "aircrafts/network_aircraft_manager.h"
#include "aircrafts/radio_stack_state.h"

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
    if(!families.contains("Open Sans")) {
        // We must check if Open Sans is already instead (at least for Linux), otherwise the FileDialog gets all corrupted...
        QFontDatabase::addApplicationFont(":/Resources/Fonts/OpenSans.ttf");
    }
    app.setFont(QFont("Open Sans", 10));

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

    qmlRegisterSingletonType<AppConfig>("org.vatsim.xpilot", 1, 0, "AppConfig", appConfigSingleton);
    qmlRegisterUncreatableMetaObject(enums::staticMetaObject, "org.vatsim.xpilot", 1, 0, "Enum", "Only enums can be registered");
    qRegisterMetaType<MessageType>("MessageType");
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

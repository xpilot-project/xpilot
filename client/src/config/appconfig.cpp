/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
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

#include <QtGlobal>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include <QStringEncoder>

#include "appconfig.h"
#include "common/build_config.h"

using namespace xpilot;

AppConfig* AppConfig::instance = nullptr;

AppConfig::AppConfig(QObject* parent) :
    QObject(parent)
{
    loadConfig();
}

AppConfig *AppConfig::getInstance()
{
    if(instance == nullptr) {
        instance = new AppConfig;
    }
    return instance;
}

const QString &AppConfig::dataRoot()
{
    QString folder("/org.vatsim.xpilot/");
    static const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + folder;
    return path;
}

const QString &AppConfig::xplanePath()
{
    QFile f(dataRoot() + "lastxplanepath.txt");
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&f);
    static const QString path = in.readAll();
    return path;
}

void AppConfig::loadConfig()
{
    QDir dir(dataRoot());
    if(!dir.exists()) {
        if(!dir.mkpath(dataRoot())) {
            emit permissionError("Failed to write configuration file. Please make sure you have correct read/write permissions to " + dataRoot());
            return;
        }
    }

    QFile configFile(dataRoot() + "AppConfig.json");
    if(!configFile.open(QIODevice::ReadOnly)) {
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        QRect primaryGeometry = primaryScreen->availableGeometry();
        // set default values
        WindowConfig.X = primaryGeometry.center().x() - DefaultWidth / 2;
        WindowConfig.Y = primaryGeometry.center().y() - DefaultHeight / 2;
        WindowConfig.Width = DefaultWidth;
        WindowConfig.Height = DefaultHeight;
        WindowConfig.Maximized = false;
        AlertPrivateMessage = true;
        AlertDirectRadioMessage = true;
        AlertRadioMessage = true;
        AlertDisconnect = true;
        AlertSelcal = true;
        AlertNetworkBroadcast = true;
        AudioEffectsDisabled = false;
        AutoModeC = true;
        Com1Volume = 50;
        Com2Volume = 50;
        Com1OnHeadset = true;
        Com2OnHeadset = true;
        MicrophoneVolume = 0;
        HFSquelchEnabled = false;
        XplaneNetworkAddress = DEFAULT_XPLANE_NETWORK_ADDRESS;
        XplanePluginPort = DEFAULT_PLUGIN_PORT;
        XplaneUdpPort = XPLANE_UDP_PORT;
        SilenceModelInstall = false;
        KeepWindowVisible = false;
        AircraftRadioStackControlsVolume = false;
        MicrophoneCalibrated = false;
        SplitAudioChannels = false;

        if(!saveConfig()) {
            emit permissionError("Failed to write configuration file. Please make sure you have correct read/write permissions to " + dataRoot());
            return;
        }

        loadConfig();
        return;
    }

    QTextStream config(&configFile);
    config.setEncoding(QStringEncoder::Utf8);
    QString json(config.readAll());
    configFile.close();

    QByteArray jsonBytes = json.toUtf8();
    auto doc = QJsonDocument::fromJson(jsonBytes);
    QJsonObject jsonObj = doc.object();
    QVariantMap jsonMap = jsonObj.toVariantMap();

    crypto.setKey(BuildConfig::ConfigEncryptionKey());

    VatsimId = getJsonValue(jsonMap, "VatsimId", QString());
    VatsimPassword = getJsonValue(jsonMap, "VatsimPassword", QString());
    Name = getJsonValue(jsonMap, "Name", QString());
    HomeAirport = getJsonValue(jsonMap, "HomeAirport", QString());
    ServerName = getJsonValue(jsonMap, "ServerName", QString());
    InputDevice = getJsonValue(jsonMap, "InputDevice", QString());
    SpeakerDevice = getJsonValue(jsonMap, "SpeakerDevice", QString());
    HeadsetDevice = getJsonValue(jsonMap, "HeadsetDevice", QString());
    SplitAudioChannels = getJsonValue(jsonMap, "SplitAudioChannels", false);
    Com1Volume = qMin(qMax(getJsonValue<int>(jsonMap, "Com1Volume", 50), 0), 100);
    Com2Volume = qMin(qMax(getJsonValue<int>(jsonMap, "Com2Volume", 50), 0), 100);
    Com1OnHeadset = getJsonValue(jsonMap, "Com1OnHeadset", true);
    Com2OnHeadset = getJsonValue(jsonMap, "Com2OnHeadset", true);
    MicrophoneVolume = qMin(qMax(getJsonValue(jsonMap, "MicrophoneVolume", 0), -60), 18);
    AudioEffectsDisabled = getJsonValue(jsonMap, "AudioEffectsDisabled", false);
    HFSquelchEnabled = getJsonValue(jsonMap, "HFSquelchEnabled", false);
    AutoModeC = getJsonValue(jsonMap, "AutoModeC", true);
    AlertPrivateMessage = getJsonValue(jsonMap, "AlertPrivateMessage", true);
    AlertRadioMessage = getJsonValue(jsonMap, "AlertRadioMessage", true);
    AlertDirectRadioMessage = getJsonValue(jsonMap, "AlertDirectRadioMessage", true);
    AlertSelcal = getJsonValue(jsonMap, "AlertSelcal", true);
    AlertDisconnect = getJsonValue(jsonMap, "AlertDisconnect", true);
    AlertNetworkBroadcast = getJsonValue(jsonMap, "AlertNetworkBroadcast", true);
    XplaneNetworkAddress = getJsonValue(jsonMap, "XplaneNetworkAddress", QString(DEFAULT_XPLANE_NETWORK_ADDRESS));
    XplanePluginPort = getJsonValue<int>(jsonMap, "XplanePluginPort", 53100);
    XplaneUdpPort = getJsonValue<int>(jsonMap, "XplaneUdpPort", 49000);
    SilenceModelInstall = getJsonValue(jsonMap, "SilenceModelInstall", false);
    VisualMachines = getJsonValue(jsonMap, "VisualMachines", QStringList());
    KeepWindowVisible = getJsonValue(jsonMap, "KeepWindowVisible", false);
    AircraftRadioStackControlsVolume = getJsonValue(jsonMap, "AircraftRadioStackControlsVolume", false);
    MicrophoneCalibrated = getJsonValue(jsonMap, "MicrophoneCalibrated", false);

    QJsonArray cachedServers = jsonMap["CachedServers"].toJsonArray();
    CachedServers.clear();
    for(const auto & value : cachedServers) {
        QJsonObject item = value.toObject();
        NetworkServerInfo server;
        server.Name = item["name"].toString();
        server.Address = item["address"].toString();
        CachedServers.append(server);
    }

    QJsonObject recent = jsonMap["RecentConnection"].toJsonObject();
    RecentConnection.Callsign = recent["Callsign"].toString();
    RecentConnection.TypeCode = recent["TypeCode"].toString();
    RecentConnection.SelcalCode = recent["SelcalCode"].toString();

    QJsonObject window = jsonMap["WindowConfig"].toJsonObject();
    WindowConfig.X = window["X"].toInt();
    WindowConfig.Y = window["Y"].toInt();
    WindowConfig.Width = qMax(window["Width"].toInt(), DefaultWidth);
    WindowConfig.Height = qMax(window["Height"].toInt(), DefaultHeight);
    WindowConfig.Maximized = window["Maximized"].toBool();

    if(!VatsimPassword.isEmpty()) {
        VatsimPasswordDecrypted = crypto.decryptToString(VatsimPassword);
    }

    setInitialTempValues();
}

bool AppConfig::saveConfig()
{
    QJsonObject jsonObj;
    jsonObj["VatsimId"] = trim(VatsimId);
    if(!VatsimPasswordDecrypted.isEmpty()) {
        jsonObj["VatsimPassword"] = crypto.encryptToString(VatsimPasswordDecrypted);
    } else {
        jsonObj["VatsimPassword"] = "";
    }
    jsonObj["Name"] = trim(Name);
    jsonObj["HomeAirport"] = trim(HomeAirport);
    jsonObj["ServerName"] = trim(ServerName);
    jsonObj["InputDevice"] = trim(InputDevice);
    jsonObj["SpeakerDevice"] = trim(SpeakerDevice);
    jsonObj["HeadsetDevice"] = trim(HeadsetDevice);
    jsonObj["SplitAudioChannels"] = SplitAudioChannels;
    jsonObj["Com1Volume"] = qMin(qMax(Com1Volume, 0), 100);
    jsonObj["Com2Volume"] = qMin(qMax(Com2Volume, 0), 100);
    jsonObj["Com1OnHeadset"] = Com1OnHeadset;
    jsonObj["Com2OnHeadset"] = Com2OnHeadset;
    jsonObj["MicrophoneVolume"] = qMin(qMax(MicrophoneVolume, -60), 18);
    jsonObj["AudioEffectsDisabled"] = AudioEffectsDisabled;
    jsonObj["HFSquelchEnabled"] = HFSquelchEnabled;
    jsonObj["AutoModeC"] = AutoModeC;
    jsonObj["AlertNetworkBroadcast"] = AlertNetworkBroadcast;
    jsonObj["AlertPrivateMessage"] = AlertPrivateMessage;
    jsonObj["AlertRadioMessage"] = AlertRadioMessage;
    jsonObj["AlertDirectRadioMessage"] = AlertDirectRadioMessage;
    jsonObj["AlertSelcal"] = AlertSelcal;
    jsonObj["AlertDisconnect"] = AlertDisconnect;
    jsonObj["XplaneNetworkAddress"] = XplaneNetworkAddress.isEmpty() ? DEFAULT_XPLANE_NETWORK_ADDRESS : trim(XplaneNetworkAddress);
    jsonObj["XplanePluginPort"] = XplanePluginPort == 0 ? DEFAULT_PLUGIN_PORT : XplanePluginPort;
    jsonObj["XplaneUdpPort"] = XplaneUdpPort == 0 ? XPLANE_UDP_PORT : XplaneUdpPort;
    jsonObj["SilenceModelInstall"] = SilenceModelInstall;
    jsonObj["KeepWindowVisible"] = KeepWindowVisible;
    jsonObj["AircraftRadioStackControlsVolume"] = AircraftRadioStackControlsVolume;
    jsonObj["MicrophoneCalibrated"] = MicrophoneCalibrated;

    QJsonArray cachedServers;
    for(auto & server : CachedServers) {
        QJsonObject item;
        item["name"] = server.Name;
        item["address"] = server.Address;
        cachedServers.append(item);
    }
    jsonObj["CachedServers"] = cachedServers;

    QJsonArray visualMachines;
    for(auto &machine : VisualMachines) {
        visualMachines.append(machine);
    }
    jsonObj["VisualMachines"] = visualMachines;

    QJsonObject recentConnection;
    recentConnection["Callsign"] = RecentConnection.Callsign;
    recentConnection["TypeCode"] = RecentConnection.TypeCode;
    recentConnection["SelcalCode"] = RecentConnection.SelcalCode;
    jsonObj["RecentConnection"] = recentConnection;

    QJsonObject window;
    window["X"] = WindowConfig.X;
    window["Y"] = WindowConfig.Y;
    window["Width"] = WindowConfig.Maximized ? DefaultWidth : qMax(WindowConfig.Width, DefaultWidth);
    window["Height"] = WindowConfig.Maximized ? DefaultHeight : qMax(WindowConfig.Height, DefaultHeight);
    window["Maximized"] = WindowConfig.Maximized;
    jsonObj["WindowConfig"] = window;

    QJsonDocument jsonDoc(jsonObj);
    QFile configFile(dataRoot() + "AppConfig.json");
    if(!configFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    if(configFile.write(jsonDoc.toJson()) == -1) {
        return false;
    }
    configFile.close();

    emit settingsChanged();
    return true;
}

bool AppConfig::configRequired()
{
    return VatsimId.isEmpty() || VatsimPasswordDecrypted.isEmpty() || Name.isEmpty();
}

void AppConfig::openAppDataFolder()
{
    QDesktopServices::openUrl(QUrl("file:///" + dataRoot()));
}

void AppConfig::applySettings()
{
    VatsimId = tempVatsimId;
    VatsimPasswordDecrypted = tempVatsimPasswordDecrypted;
    Name = tempName;
    HomeAirport = tempHomeAirport;
    ServerName = tempServerName;
    SpeakerDevice = tempSpeakerDevice;
    HeadsetDevice = tempHeadsetDevice;
    InputDevice = tempInputDevice;
    SplitAudioChannels = tempSplitAudioChannels;
    Com1Volume = tempCom1Volume;
    Com2Volume = tempCom2Volume;
    MicrophoneVolume = tempMicrophoneVolume;
    AudioEffectsDisabled = tempAudioEffectsDisabled;
    HFSquelchEnabled = tempHFSquelchEnabled;
    AutoModeC = tempAutoModeC;
    AlertPrivateMessage = tempAlertPrivateMessage;
    AlertDirectRadioMessage = tempAlertDirectRadioMessage;
    AlertRadioMessage = tempAlertRadioMessage;
    AlertNetworkBroadcast = tempAlertNetworkBroadcast;
    AlertSelcal = tempAlertSelcal;
    AlertDisconnect = tempAlertDisconnect;
    KeepWindowVisible = tempKeepWindowVisible;
    AircraftRadioStackControlsVolume = tempAircraftRadioStackControlsVolume;
}

QString AppConfig::getNetworkServer()
{
    if(ServerName.isEmpty() || CachedServers.isEmpty()) return "";
    for(auto & server : CachedServers)
    {
        if(server.Name == ServerName)
        {
            return server.Address;
        }
    }

    return "";
}

void AppConfig::setInitialTempValues()
{
    tempVatsimId = VatsimId;
    tempVatsimPasswordDecrypted = VatsimPasswordDecrypted;
    tempName = Name;
    tempHomeAirport = HomeAirport;
    tempServerName = ServerName;
    tempSpeakerDevice = SpeakerDevice;
    tempHeadsetDevice = HeadsetDevice;
    tempInputDevice = InputDevice;
    tempSplitAudioChannels = SplitAudioChannels;
    tempCom1Volume = Com1Volume;
    tempCom2Volume = Com2Volume;
    tempMicrophoneVolume = MicrophoneVolume;
    tempAudioEffectsDisabled = AudioEffectsDisabled;
    tempHFSquelchEnabled = HFSquelchEnabled;
    tempAutoModeC = AutoModeC;
    tempAlertPrivateMessage = AlertPrivateMessage;
    tempAlertDirectRadioMessage = AlertDirectRadioMessage;
    tempAlertRadioMessage = AlertRadioMessage;
    tempAlertNetworkBroadcast = AlertNetworkBroadcast;
    tempAlertSelcal = AlertSelcal;
    tempAlertDisconnect = AlertDisconnect;
    tempKeepWindowVisible = KeepWindowVisible;
    tempAircraftRadioStackControlsVolume = AircraftRadioStackControlsVolume;
}

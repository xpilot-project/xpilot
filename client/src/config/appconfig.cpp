#include <QtGlobal>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QDir>
#include "appconfig.h"
#include "src/common/build_config.h"

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
        int x = (primaryScreen->size().width() / 2) - (DefaultWidth / 2);
        int y = (primaryScreen->size().height() / 2) - (DefaultHeight / 2);

        // set default values
        WindowConfig.X = x;
        WindowConfig.Y = y;
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
        MicrophoneVolume = 0;
        HFSquelchEnabled = false;
        XplaneNetworkAddress = DEFAULT_XPLANE_NETWORK_ADDRESS;
        XplanePluginPort = DEFAULT_PLUGIN_PORT;
        XplaneUdpPort = XPLANE_UDP_PORT;
        SilenceModelInstall = false;
        KeepWindowVisible = false;
        AircraftRadioStackControlsVolume = true;
        MicrophoneCalibrated = false;

        if(!saveConfig()) {
            emit permissionError("Failed to write configuration file. Please make sure you have correct read/write permissions to " + dataRoot());
            return;
        }

        loadConfig();
        return;
    }

    QTextStream config(&configFile);
    config.setCodec("UTF-8");
    QString json(config.readAll());
    configFile.close();

    QByteArray jsonBytes = json.toUtf8();
    auto doc = QJsonDocument::fromJson(jsonBytes);
    QJsonObject jsonObj = doc.object();
    QVariantMap jsonMap = jsonObj.toVariantMap();

    crypto.setKey(BuildConfig::ConfigEncryptionKey());

    VatsimId = jsonMap["VatsimId"].toString();
    VatsimPassword = jsonMap["VatsimPassword"].toString();
    Name = jsonMap["Name"].toString();
    HomeAirport = jsonMap["HomeAirport"].toString();
    ServerName = jsonMap["ServerName"].toString();
    AudioApi = jsonMap["AudioApi"].toString();
    InputDevice = jsonMap["InputDevice"].toString();
    OutputDevice = jsonMap["OutputDevice"].toString();
    Com1Volume = qMin(qMax(jsonMap["Com1Volume"].toInt(), 0), 100);
    Com2Volume = qMin(qMax(jsonMap["Com2Volume"].toInt(), 0), 100);
    MicrophoneVolume = qMin(qMax(jsonMap["MicrophoneVolume"].toInt(), -18), 18);
    AudioEffectsDisabled = jsonMap["AudioEffectsDisabled"].toBool();
    HFSquelchEnabled = jsonMap["HFSquelchEnabled"].toBool();
    AutoModeC = jsonMap["AutoModeC"].toBool();
    AlertPrivateMessage = jsonMap["AlertPrivateMessage"].toBool();
    AlertRadioMessage = jsonMap["AlertRadioMessage"].toBool();
    AlertDirectRadioMessage = jsonMap["AlertDirectRadioMessage"].toBool();
    AlertSelcal = jsonMap["AlertSelcal"].toBool();
    AlertDisconnect = jsonMap["AlertDisconnect"].toBool();
    AlertNetworkBroadcast = jsonMap["AlertNetworkBroadcast"].toBool();
    XplaneNetworkAddress = jsonMap["XplaneNetworkAddress"].toString();
    XplanePluginPort = jsonMap["XplanePluginPort"].toInt();
    XplaneUdpPort = jsonMap["XplaneUdpPort"].toInt();
    SilenceModelInstall = jsonMap["SilenceModelInstall"].toBool();
    VisualMachines = jsonMap["VisualMachines"].toStringList();
    KeepWindowVisible = jsonMap["KeepWindowVisible"].toBool();
    AircraftRadioStackControlsVolume = jsonMap["AircraftRadioStackControlsVolume"].toBool();
    MicrophoneCalibrated = jsonMap["MicrophoneCalibrated"].toBool();

    QJsonArray cachedServers = jsonMap["CachedServers"].toJsonArray();
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
}

bool AppConfig::saveConfig()
{
    QJsonObject jsonObj;
    jsonObj["VatsimId"] = VatsimId;
    if(!VatsimPasswordDecrypted.isEmpty()) {
        jsonObj["VatsimPassword"] = crypto.encryptToString(VatsimPasswordDecrypted);
    } else {
        jsonObj["VatsimPassword"] = "";
    }
    jsonObj["Name"] = Name;
    jsonObj["HomeAirport"] = HomeAirport;
    jsonObj["ServerName"] = ServerName;
    jsonObj["AudioApi"] = AudioApi;
    jsonObj["InputDevice"] = InputDevice;
    jsonObj["OutputDevice"] = OutputDevice;
    jsonObj["Com1Volume"] = qMin(qMax(Com1Volume, 0), 100);
    jsonObj["Com2Volume"] = qMin(qMax(Com2Volume, 0), 100);
    jsonObj["MicrophoneVolume"] = qMin(qMax(MicrophoneVolume, -18), 18);
    jsonObj["AudioEffectsDisabled"] = AudioEffectsDisabled;
    jsonObj["HFSquelchEnabled"] = HFSquelchEnabled;
    jsonObj["AutoModeC"] = AutoModeC;
    jsonObj["AlertNetworkBroadcast"] = AlertNetworkBroadcast;
    jsonObj["AlertPrivateMessage"] = AlertPrivateMessage;
    jsonObj["AlertRadioMessage"] = AlertRadioMessage;
    jsonObj["AlertDirectRadioMessage"] = AlertDirectRadioMessage;
    jsonObj["AlertSelcal"] = AlertSelcal;
    jsonObj["AlertDisconnect"] = AlertDisconnect;
    jsonObj["XplaneNetworkAddress"] = XplaneNetworkAddress.isEmpty() ? DEFAULT_XPLANE_NETWORK_ADDRESS : XplaneNetworkAddress;
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
    window["Width"] = qMax(WindowConfig.Width, DefaultWidth);
    window["Height"] = qMax(WindowConfig.Height, DefaultHeight);
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

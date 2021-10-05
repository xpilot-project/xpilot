#include "appconfig.h"

using namespace xpilot;

AppConfig* AppConfig::instance = 0;

AppConfig::AppConfig(QObject* parent) :
    QObject(parent)
{
    loadConfig();
}

AppConfig *AppConfig::getInstance()
{
    if(instance == 0) {
        instance = new AppConfig;
    }
    return instance;
}

void AppConfig::loadConfig()
{
    QFile configFile("AppConfig.json");
    if(!configFile.open(QIODevice::ReadOnly)) {
        saveConfig();
        loadConfig();
        return;
    }

    QTextStream config(&configFile);
    QString json(config.readAll());
    configFile.close();

    QByteArray jsonBytes = json.toLocal8Bit();
    auto doc = QJsonDocument::fromJson(jsonBytes);
    QJsonObject jsonObj = doc.object();
    QVariantMap jsonMap = jsonObj.toVariantMap();

    crypto.setKey(Q_UINT64_C(0x195ead8a7710623b));

    VatsimId = jsonMap["VatsimId"].toString();
    VatsimPassword = jsonMap["VatsimPassword"].toString();
    Name = jsonMap["Name"].toString();
    HomeAirport = jsonMap["HomeAirport"].toString();
    ServerName = jsonMap["ServerName"].toString();

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
    WindowConfig.Width = window["Width"].toInt();
    WindowConfig.Height = window["Height"].toInt();

    if(!VatsimPassword.isEmpty()) {
        VatsimPasswordDecrypted = crypto.decryptToString(VatsimPassword);
    }
}

void AppConfig::saveConfig()
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

    QJsonArray cachedServers;
    for(auto & server : CachedServers) {
        QJsonObject item;
        item["name"] = server.Name;
        item["address"] = server.Address;
        cachedServers.append(item);
    }
    jsonObj["CachedServers"] = cachedServers;

    QJsonObject recentConnection;
    recentConnection["Callsign"] = RecentConnection.Callsign;
    recentConnection["TypeCode"] = RecentConnection.TypeCode;
    recentConnection["SelcalCode"] = RecentConnection.SelcalCode;
    jsonObj["RecentConnection"] = recentConnection;

    QJsonObject window;
    window["X"] = WindowConfig.X;
    window["Y"] = WindowConfig.Y;
    window["Width"] = WindowConfig.Width;
    window["Height"] = WindowConfig.Height;
    jsonObj["WindowConfig"] = window;

    QJsonDocument jsonDoc(jsonObj);
    QFile configFile("AppConfig.json");
    if(!configFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to write config file";
        return;
    }

    configFile.write(jsonDoc.toJson());
    configFile.close();
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
}

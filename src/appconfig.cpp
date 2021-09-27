#include "appconfig.h"

using namespace xpilot;

AppConfig& AppConfig::Instance()
{
    static auto&& config = AppConfig();
    return config;
}

void AppConfig::LoadConfig()
{
    QFile configFile("AppConfig.json");
    if(!configFile.open(QIODevice::ReadOnly)) {
        SaveConfig();
        LoadConfig();
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

    if(!VatsimPassword.isEmpty()) {
        VatsimPasswordDecrypted = crypto.decryptToString(VatsimPassword);
    }
}

void AppConfig::SaveConfig()
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

    QJsonDocument jsonDoc(jsonObj);
    QFile configFile("AppConfig.json");
    if(!configFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to write config file";
        return;
    }

    configFile.write(jsonDoc.toJson());
    configFile.close();
}

bool AppConfig::ConfigRequired()
{
    return VatsimId.isEmpty() || VatsimPasswordDecrypted.isEmpty() || Name.isEmpty();
}

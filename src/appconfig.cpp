#include <nlohmann/json.hpp>
#include "appconfig.h"

using namespace xpilot;
using json = nlohmann::json;

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

    QJsonDocument jsonDoc(jsonObj);
    QFile configFile("AppConfig.json");
    if(!configFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to write config file";
        return;
    }

    configFile.write(jsonDoc.toJson());
    configFile.close();
}

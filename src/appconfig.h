#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QFile>
#include <QDebug>
#include <QVector>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "common/simplecrypt.h"
#include "network/networkinfo.h"

namespace xpilot
{
    class AppConfig
    {
    public:
        static AppConfig& Instance();
        ~AppConfig() = default;
        AppConfig(const AppConfig&) = delete;
        void operator=(const AppConfig&) = delete;
        AppConfig(AppConfig&&)noexcept = default;
        AppConfig& operator=(AppConfig&&)noexcept = default;
        AppConfig() = default;

        void LoadConfig();
        void SaveConfig();
        bool ConfigRequired();

        QString VatsimId;
        QString VatsimPasswordDecrypted;
        QString Name;
        QString HomeAirport;
        QString ServerName;
        QVector<NetworkServerInfo> CachedServers;

    private:
        SimpleCrypt crypto;
        QString VatsimPassword;
    };
}

#endif // APPCONFIG_H

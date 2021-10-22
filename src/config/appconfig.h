#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QDebug>
#include <QVector>
#include <QVariant>
#include <QVariantList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "windowconfig.h"
#include "src/common/simplecrypt.h"
#include "src/network/networkserverlist.h"
#include "src/network/connectinfo.h"

namespace xpilot
{
    class AppConfig : public QObject
    {
        Q_OBJECT

    public:
        explicit AppConfig(QObject * owner = nullptr);
        static AppConfig *getInstance();

        Q_INVOKABLE void saveConfig();
        void loadConfig();
        bool configRequired();
        QString getNetworkServer();

        QString VatsimId;
        QString VatsimPasswordDecrypted;
        QString Name;
        QString HomeAirport;
        QString ServerName;
        QVector<NetworkServerInfo> CachedServers;
        ConnectInfo RecentConnection;
        ClientWindowConfig WindowConfig;
        QString AudioApi;
        QString OutputDevice;
        QString InputDevice;
        int Com1Volume;
        int Com2Volume;

        QVariant VariantCachedServers() const
        {
            QVariantList itemList;

            for(const NetworkServerInfo &server: CachedServers)
            {
                QVariantMap itemMap;
                itemMap.insert("name", server.Name);
                itemMap.insert("address", server.Address);
                itemList.append(itemMap);
            }

            return QVariant::fromValue(itemList);
        }

        Q_PROPERTY(QString VatsimId MEMBER VatsimId)
        Q_PROPERTY(QString VatsimPasswordDecrypted MEMBER VatsimPasswordDecrypted)
        Q_PROPERTY(QString Name MEMBER Name)
        Q_PROPERTY(QString HomeAirport MEMBER HomeAirport)
        Q_PROPERTY(QString ServerName MEMBER ServerName)
        Q_PROPERTY(QVariant CachedServers READ VariantCachedServers)
        Q_PROPERTY(ConnectInfo RecentConnection MEMBER RecentConnection)
        Q_PROPERTY(ClientWindowConfig WindowConfig MEMBER WindowConfig)\
        Q_PROPERTY(QString AudioApi MEMBER AudioApi)
        Q_PROPERTY(QString InputDevice MEMBER InputDevice)
        Q_PROPERTY(QString OutputDevice MEMBER OutputDevice)
        Q_PROPERTY(int Com1Volume MEMBER Com1Volume)
        Q_PROPERTY(int Com2Volume MEMBER Com2Volume)

    private:
        static AppConfig* instance;
        SimpleCrypt crypto;
        QString VatsimPassword;
    };
}

#endif // APPCONFIG_H

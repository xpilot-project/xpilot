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
#include "src/network/serverlistmanager.h"
#include "src/network/connectinfo.h"

#define DEFAULT_XPLANE_NETWORK_ADDRESS "127.0.0.1"
#define DEFAULT_PLUGIN_PORT 53100
#define XPLANE_UDP_PORT 49000

namespace xpilot
{
    class AppConfig : public QObject
    {
        Q_OBJECT

    public:
        explicit AppConfig(QObject * owner = nullptr);
        static AppConfig *getInstance();
        const static QString &dataRoot();
        const static QString &xplanePath();

        Q_INVOKABLE bool saveConfig();
        Q_INVOKABLE void loadConfig();
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
        QString OutputDevice;
        QString InputDevice;
        int Com1Volume = 100;
        int Com2Volume = 100;
        int MicrophoneVolume = 0;
        bool AudioEffectsDisabled;
        bool HFSquelchEnabled;
        bool AutoModeC;
        bool AlertPrivateMessage;
        bool AlertDirectRadioMessage;
        bool AlertRadioMessage;
        bool AlertNetworkBroadcast;
        bool AlertSelcal;
        bool AlertDisconnect;
        QString XplaneNetworkAddress;
        int XplanePluginPort; // used for visual machines or if the sim is on a different machine than the xPilot client
        int XplaneUdpPort;
        bool SilenceModelInstall;
        QStringList VisualMachines;
        bool KeepWindowVisible;
        bool AircraftRadioStackControlsVolume;
        bool MicrophoneCalibrated;

        QString NameWithHomeAirport() const
        {
            if(!HomeAirport.isEmpty())
            {
                return QString("%1 %2").arg(Name).arg(HomeAirport);
            }
            return Name;
        }

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
        Q_PROPERTY(ClientWindowConfig WindowConfig MEMBER WindowConfig)
        Q_PROPERTY(QString InputDevice MEMBER InputDevice NOTIFY inputDeviceChanged)
        Q_PROPERTY(QString OutputDevice MEMBER OutputDevice)
        Q_PROPERTY(int Com1Volume MEMBER Com1Volume)
        Q_PROPERTY(int Com2Volume MEMBER Com2Volume)
        Q_PROPERTY(int MicrophoneVolume MEMBER MicrophoneVolume)
        Q_PROPERTY(bool AudioEffectsDisabled MEMBER AudioEffectsDisabled)
        Q_PROPERTY(bool HFSquelchEnabled MEMBER HFSquelchEnabled)
        Q_PROPERTY(bool AutoModeC MEMBER AutoModeC)
        Q_PROPERTY(bool AlertPrivateMessage MEMBER AlertPrivateMessage NOTIFY alertPrivateMessageChanged)
        Q_PROPERTY(bool AlertRadioMessage MEMBER AlertRadioMessage NOTIFY alertRadioMessageChanged)
        Q_PROPERTY(bool AlertDirectRadioMessage MEMBER AlertDirectRadioMessage NOTIFY alertDirectRadioMessageChanged)
        Q_PROPERTY(bool AlertSelcal MEMBER AlertSelcal NOTIFY alertSelcalChanged)
        Q_PROPERTY(bool AlertNetworkBroadcast MEMBER AlertNetworkBroadcast NOTIFY alertNetworkBroadcastChanged)
        Q_PROPERTY(bool AlertDisconnect MEMBER AlertDisconnect NOTIFY alertDisconnectChanged)
        Q_PROPERTY(bool SilenceModelInstall MEMBER SilenceModelInstall)
        Q_PROPERTY(QStringList VisualMachines MEMBER VisualMachines)
        Q_PROPERTY(QString XplaneNetworkAddress MEMBER XplaneNetworkAddress)
        Q_PROPERTY(bool KeepWindowVisible MEMBER KeepWindowVisible)
        Q_PROPERTY(bool AircraftRadioStackControlsVolume MEMBER AircraftRadioStackControlsVolume)
        Q_PROPERTY(bool MicrophoneCalibrated MEMBER MicrophoneCalibrated)

    signals:
        void alertPrivateMessageChanged();
        void alertRadioMessageChanged();
        void alertDirectRadioMessageChanged();
        void alertSelcalChanged();
        void alertNetworkBroadcastChanged();
        void alertDisconnectChanged();
        void settingsChanged();
        void inputDeviceChanged();
        void permissionError(QString error);

    private:
        static AppConfig* instance;
        SimpleCrypt crypto;
        QString VatsimPassword;

        int DefaultWidth = 800;
        int DefaultHeight = 250;
    };
}

#endif // APPCONFIG_H

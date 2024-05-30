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
#include "common/simplecrypt.h"
#include "network/serverlistmanager.h"
#include "network/connectinfo.h"

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
        const static QString &soundsPath();

        Q_INVOKABLE bool saveConfig();
        Q_INVOKABLE void loadConfig();
        Q_INVOKABLE bool configRequired();
        Q_INVOKABLE void openAppDataFolder();
        Q_INVOKABLE void applySettings();
        QString getNetworkServer();

        void setInitialTempValues();

        QString VatsimId;
        QString VatsimPasswordDecrypted;
        QString Name;
        QString HomeAirport;
        QString ServerName;
        QVector<NetworkServerInfo> CachedServers;
        ConnectInfo RecentConnection;
        ClientWindowConfig WindowConfig;
        QString SpeakerDevice;
        QString HeadsetDevice;
        QString InputDevice;
        QString NotificationAudioDevice;
        bool Com1OnHeadset = true;
        bool Com2OnHeadset = true;
        bool SplitAudioChannels;
        int Com1Volume = 50;
        int Com2Volume = 50;
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

        Q_PROPERTY(QString VatsimId READ getVatsimId WRITE setVatsimId NOTIFY vatsimIdChanged)
        Q_PROPERTY(QString VatsimPasswordDecrypted READ getVatsimPasswordDecrypted WRITE setVatsimPasswordDecrypted NOTIFY vatsimPasswordDecryptedChanged)
        Q_PROPERTY(QString Name READ getName WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QString HomeAirport READ getHomeAirport WRITE setHomeAirport NOTIFY homeAirportChanged)
        Q_PROPERTY(QString ServerName READ getServerName WRITE setServerName NOTIFY serverNameChanged)
        Q_PROPERTY(QString SpeakerDevice READ getSpeakerDevice WRITE setSpeakerDevice NOTIFY speakerDeviceChanged)
        Q_PROPERTY(QString HeadsetDevice READ getHeadsetDevice WRITE setHeadsetDevice NOTIFY headsetDeviceChanged)
        Q_PROPERTY(QString InputDevice READ getInputDevice WRITE setInputDevice NOTIFY inputDeviceChanged)
        Q_PROPERTY(QString NotificationAudioDevice READ getNotificationAudioDevice WRITE setNotificationAudioDevice NOTIFY notificationAudioDeviceChanged)
        Q_PROPERTY(bool SplitAudioChannels READ getSplitAudioChannels WRITE setSplitAudioChannels NOTIFY splitAudioChannelsChanged)
        Q_PROPERTY(int Com1Volume READ getCom1Volume WRITE setCom1Volume NOTIFY com1VolumeChanged)
        Q_PROPERTY(int Com2Volume READ getCom2Volume WRITE setCom2Volume NOTIFY com2VolumeChanged)
        Q_PROPERTY(int MicrophoneVolume READ getMicrophoneVolume WRITE setMicrophoneVolume NOTIFY microphoneVolumeChanged)
        Q_PROPERTY(bool AudioEffectsDisabled READ getAudioEffectsDisabled WRITE setAudioEffectsDisabled NOTIFY audioEffectsDisabledChanged)
        Q_PROPERTY(bool HFSquelchEnabled READ getHFSquelchEnabled WRITE setHFSquelchEnabled NOTIFY hfSquelchEnabledChanged)
        Q_PROPERTY(bool AutoModeC READ getAutoModeC WRITE setAutoModeC NOTIFY autoModeCChanged)
        Q_PROPERTY(bool AlertPrivateMessage READ getAlertPrivateMessage WRITE setAlertPrivateMessage NOTIFY alertPrivateMessageChanged)
        Q_PROPERTY(bool AlertDirectRadioMessage READ getAlertDirectRadioMessage WRITE setAlertDirectRadioMessage NOTIFY alertDirectRadioMessageChanged)
        Q_PROPERTY(bool AlertRadioMessage READ getAlertRadioMessage WRITE setAlertRadioMessage NOTIFY alertRadioMessageChanged)
        Q_PROPERTY(bool AlertNetworkBroadcast READ getAlertNetworkBroadcast WRITE setAlertNetworkBroadcast NOTIFY alertNetworkBroadcastChanged)
        Q_PROPERTY(bool AlertSelcal READ getAlertSelcal WRITE setAlertSelcal NOTIFY alertSelcalChanged)
        Q_PROPERTY(bool AlertDisconnect READ getAlertDisconnect WRITE setAlertDisconnect NOTIFY alertDisconnectChanged)
        Q_PROPERTY(bool KeepWindowVisible READ getKeepWindowVisible WRITE setKeepWindowVisible NOTIFY keepWindowVisibleChanged)
        Q_PROPERTY(bool AircraftRadioStackControlsVolume READ getAircraftRadioStackControlsVolume WRITE setAircraftRadioStackControlsVolume NOTIFY aircraftRadioStackControlsVolumeChanged)

        Q_PROPERTY(bool Com1OnHeadset MEMBER Com1OnHeadset)
        Q_PROPERTY(bool Com2OnHeadset MEMBER Com2OnHeadset)

        Q_PROPERTY(QVariant CachedServers READ VariantCachedServers)
        Q_PROPERTY(ConnectInfo RecentConnection MEMBER RecentConnection)
        Q_PROPERTY(ClientWindowConfig WindowConfig MEMBER WindowConfig)
        Q_PROPERTY(bool MicrophoneCalibrated MEMBER MicrophoneCalibrated)
        Q_PROPERTY(bool SilenceModelInstall MEMBER SilenceModelInstall)
        Q_PROPERTY(QStringList VisualMachines MEMBER VisualMachines)
        Q_PROPERTY(QString XplaneNetworkAddress MEMBER XplaneNetworkAddress)

        // Setter functions
        void setVatsimId(const QString &value) { tempVatsimId = value; }
        void setVatsimPasswordDecrypted(const QString &value) { tempVatsimPasswordDecrypted = value; }
        void setName(const QString &value) { tempName = value; }
        void setHomeAirport(const QString &value) { tempHomeAirport = value; }
        void setServerName(const QString &value) { tempServerName = value; }
        void setRecentConnection(const ConnectInfo &value) { RecentConnection = value; }
        void setWindowConfig(const ClientWindowConfig &value) { WindowConfig = value; }
        void setSpeakerDevice(const QString &value) { tempSpeakerDevice = value; }
        void setHeadsetDevice(const QString &value) { tempHeadsetDevice = value; }
        void setInputDevice(const QString &value) { tempInputDevice = value; }
        void setNotificationAudioDevice(const QString &value) { tempNotificationAudioDevice = value; }
        void setSplitAudioChannels(bool value) { tempSplitAudioChannels = value; }
        void setCom1Volume(int value) { tempCom1Volume = value; }
        void setCom2Volume(int value) { tempCom2Volume = value; }
        void setMicrophoneVolume(int value) { tempMicrophoneVolume = value; }
        void setAudioEffectsDisabled(bool value) { tempAudioEffectsDisabled = value; }
        void setHFSquelchEnabled(bool value) { tempHFSquelchEnabled = value; }
        void setAutoModeC(bool value) { tempAutoModeC = value; }
        void setAlertPrivateMessage(bool value) { tempAlertPrivateMessage = value; }
        void setAlertDirectRadioMessage(bool value) { tempAlertDirectRadioMessage = value; }
        void setAlertRadioMessage(bool value) { tempAlertRadioMessage = value; }
        void setAlertNetworkBroadcast(bool value) { tempAlertNetworkBroadcast = value; }
        void setAlertSelcal(bool value) { tempAlertSelcal = value; }
        void setAlertDisconnect(bool value) { tempAlertDisconnect = value; }
        void setKeepWindowVisible(bool value) { tempKeepWindowVisible = value; }
        void setAircraftRadioStackControlsVolume(bool value) { tempAircraftRadioStackControlsVolume = value; }

        // Getter functions
        QString getVatsimId() const { return VatsimId; }
        QString getVatsimPasswordDecrypted() const { return VatsimPasswordDecrypted; }
        QString getName() const { return Name; }
        QString getHomeAirport() const { return HomeAirport; }
        QString getServerName() const { return ServerName; }
        ConnectInfo getRecentConnection() const { return RecentConnection; }
        ClientWindowConfig getWindowConfig() const { return WindowConfig; }
        QString getSpeakerDevice() const { return SpeakerDevice; }
        QString getHeadsetDevice() const { return HeadsetDevice; }
        QString getInputDevice() const { return InputDevice; }
        QString getNotificationAudioDevice() const { return NotificationAudioDevice; }
        bool getSplitAudioChannels() const { return SplitAudioChannels; }
        int getCom1Volume() const { return Com1Volume; }
        int getCom2Volume() const { return Com2Volume; }
        int getMicrophoneVolume() const { return MicrophoneVolume; }
        bool getAudioEffectsDisabled() const { return AudioEffectsDisabled; }
        bool getHFSquelchEnabled() const { return HFSquelchEnabled; }
        bool getAutoModeC() const { return AutoModeC; }
        bool getAlertPrivateMessage() const { return AlertPrivateMessage; }
        bool getAlertDirectRadioMessage() const { return AlertDirectRadioMessage; }
        bool getAlertRadioMessage() const { return AlertRadioMessage; }
        bool getAlertNetworkBroadcast() const { return AlertNetworkBroadcast; }
        bool getAlertSelcal() const { return AlertSelcal; }
        bool getAlertDisconnect() const { return AlertDisconnect; }
        bool getKeepWindowVisible() const { return KeepWindowVisible; }
        bool getAircraftRadioStackControlsVolume() const { return AircraftRadioStackControlsVolume; }

    signals:
        void settingsChanged();
        void inputDeviceChanged();
        void permissionError(QString error);

        void vatsimIdChanged();
        void vatsimPasswordDecryptedChanged();
        void nameChanged();
        void homeAirportChanged();
        void serverNameChanged();
        void speakerDeviceChanged();
        void headsetDeviceChanged();
        void notificationAudioDeviceChanged();
        void splitAudioChannelsChanged();
        void com1VolumeChanged();
        void com2VolumeChanged();
        void microphoneVolumeChanged();
        void audioEffectsDisabledChanged();
        void hfSquelchEnabledChanged();
        void autoModeCChanged();
        void alertPrivateMessageChanged();
        void alertDirectRadioMessageChanged();
        void alertRadioMessageChanged();
        void alertNetworkBroadcastChanged();
        void alertSelcalChanged();
        void alertDisconnectChanged();
        void keepWindowVisibleChanged();
        void aircraftRadioStackControlsVolumeChanged();

    private:
        static AppConfig* instance;
        SimpleCrypt crypto;
        QString VatsimPassword;

        int DefaultWidth = 840;
        int DefaultHeight = 250;

        QString tempVatsimId;
        QString tempVatsimPasswordDecrypted;
        QString tempName;
        QString tempHomeAirport;
        QString tempServerName;
        QString tempSpeakerDevice;
        QString tempHeadsetDevice;
        QString tempInputDevice;
        QString tempNotificationAudioDevice;
        bool tempSplitAudioChannels;
        int tempCom1Volume;
        int tempCom2Volume;
        int tempMicrophoneVolume;
        bool tempAudioEffectsDisabled;
        bool tempHFSquelchEnabled;
        bool tempAutoModeC;
        bool tempAlertPrivateMessage;
        bool tempAlertDirectRadioMessage;
        bool tempAlertRadioMessage;
        bool tempAlertNetworkBroadcast;
        bool tempAlertSelcal;
        bool tempAlertDisconnect;
        bool tempKeepWindowVisible;
        bool tempAircraftRadioStackControlsVolume;

        template <typename T>
        T getJsonValue(const QVariantMap &map, const QString &key, const T &defaultValue) {
            if (map.contains(key)) {
                return map[key].value<T>();
            }
            return defaultValue;
        }

        static QString trim(const QString& str) {
            static const QRegularExpression regex("[\n\t\r]");
            QString trimmedStr = str;
            trimmedStr.remove(regex);
            return trimmedStr;
        }
    };
}

#endif // APPCONFIG_H

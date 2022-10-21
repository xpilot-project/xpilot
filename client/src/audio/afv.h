#ifndef AFV_H
#define AFV_H

#include <QtGlobal>
#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QVariantList>
#include <QFile>
#include <QTextStream>

#include <thread>
#include <memory>

#include "src/network/networkmanager.h"
#include "src/simulator/xplane_adapter.h"
#include "src/controllers/controller_manager.h"
#include "src/controllers/controller.h"
#include <event2/event.h>
#include "afv-native/Client.h"
#include "audiodeviceinfo.h"

namespace xpilot
{
    class AudioForVatsim : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariant OutputDevices READ getOutputDevices NOTIFY outputDevicesChanged)
        Q_PROPERTY(QVariant InputDevices READ getInputDevices NOTIFY inputDevicesChanged)

    public:
        AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, ControllerManager& controllerManager, QObject* parent = nullptr);
        ~AudioForVatsim();

        void afvLogger(QString message);

        QVariant getOutputDevices() const
        {
            QVariantList itemList;

            for(const auto &device: m_outputDevices)
            {
                QVariantMap itemMap;
                itemMap.insert("id", device.Id);
                itemMap.insert("name", device.DeviceName);
                itemList.append(itemMap);
            }

            return QVariant::fromValue(itemList);
        }

        QVariant getInputDevices() const
        {
            QVariantList itemList;

            for(const auto &device: m_inputDevices)
            {
                QVariantMap itemMap;
                itemMap.insert("id", device.Id);
                itemMap.insert("name", device.DeviceName);
                itemList.append(itemMap);
            }

            return QVariant::fromValue(itemList);
        }

        Q_INVOKABLE void setInputDevice(QString deviceId);
        Q_INVOKABLE void setOutputDevice(QString deviceId);
        Q_INVOKABLE void setCom1Volume(double volume);
        Q_INVOKABLE void setCom2Volume(double volume);
        Q_INVOKABLE void disableAudioEffects(bool disabled);
        Q_INVOKABLE void enableHfSquelch(bool enabled);
        Q_INVOKABLE void setMicrophoneVolume(int volume);
        Q_INVOKABLE void settingsWindowOpened();
        Q_INVOKABLE void settingsWindowClosed();

    private slots:
        void OnNetworkConnected(QString callsign, bool enableVoice);
        void OnNetworkDisconnected();
        void OnTransceiverTimer();
        void OnAudioDevicesTimer();

    signals:
        void notificationPosted(int type, QString message);
        void radioRxChanged(uint radio, bool active);
        void outputDevicesChanged();
        void inputDevicesChanged();
        void inputVuChanged(float vu);
        void radioAliasChanged(uint radio, quint32 frequency);

    private:
        void configureAudioDevices();
        void updateTransceivers();

    private:
        XplaneAdapter& m_xplaneAdapter;
        struct event_base* ev_base;
        bool m_keepAlive = false;
        std::shared_ptr<afv_native::Client> m_client;
        QTimer m_transceiverTimer;
        QTimer m_eventTimer;
        QTimer m_rxTxQueryTimer;
        QTimer m_vuTimer;
        QThread *m_workerThread;

        QFile m_afvLog;
        QTextStream m_logDataStream;

        bool m_com1Rx = false;
        bool m_com2Rx = false;

        RadioStackState m_radioStackState;
        UserAircraftData m_userAircraftData;

        QList<AudioDeviceInfo> m_outputDevices;
        QList<AudioDeviceInfo> m_inputDevices;
        QTimer m_audioDevicesTimer;

        bool fuzzyMatchCallsign(const QString &callsign, const QString &compareTo) const;
        void getPrefixSuffix(const QString &callsign, QString &prefix, QString &suffix) const;

        QVector<afv_native::afv::dto::Station> m_aliasedStations;
        quint32 getAliasFrequency(quint32 frequency) const;

        QList<Controller> m_controllers;

        double scaleValue(double &value, double limitMin, double limitMax, double baseMin, double baseMax) const
        {
            return ((limitMax - limitMin) * (value - baseMin)) / (baseMax - baseMin) + limitMin;
        }
    };
}

#endif // AFV_H

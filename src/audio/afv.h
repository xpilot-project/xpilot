#ifndef AFV_H
#define AFV_H

#include <QtGlobal>
#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QVariantList>

#include <thread>
#include <memory>

#include "src/network/networkmanager.h"
#include "src/simulator/xplane_adapter.h"
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
        Q_PROPERTY(QVariant AudioApis READ getAudioApis NOTIFY audioApisChanged)

    public:
        AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject* parent = nullptr);
        ~AudioForVatsim();

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

        QVariant getAudioApis() const
        {
            QVariantList itemList;

            for(const auto &api: m_audioDrivers)
            {
                QVariantMap itemMap;
                itemMap.insert("id", api.first);
                itemMap.insert("name", api.second.c_str());
                itemList.append(itemMap);
            }

            return QVariant::fromValue(itemList);
        }

        Q_INVOKABLE void setAudioApi(int api);
        Q_INVOKABLE void setInputDevice(QString deviceId);
        Q_INVOKABLE void setOutputDevice(QString deviceId);
        Q_INVOKABLE void setCom1Volume(double volume);
        Q_INVOKABLE void setCom2Volume(double volume);
        Q_INVOKABLE void disableAudioEffects(bool disabled);
        Q_INVOKABLE void enableHfSquelch(bool enabled);

    private slots:
        void OnNetworkConnected(QString callsign);
        void OnNetworkDisconnected();
        void OnTransceiverTimer();

    signals:
        void notificationPosted(int type, QString message);
        void radioRxChanged(uint radio, bool active);
        void outputDevicesChanged();
        void inputDevicesChanged();
        void audioApisChanged();
        void inputVuChanged(float vu);

    private:
        void configureAudioDevices();
        void updateTransceivers();

    private:
        struct event_base* ev_base;
        bool m_keepAlive = false;
        std::shared_ptr<afv_native::Client> m_client;
        QTimer* m_transceiverTimer;
        QTimer* m_eventTimer;
        QTimer* m_rxTxQueryTimer;
        QTimer* m_vuTimer;
        QThread *m_workerThread;

        bool m_com1Rx = false;
        bool m_com2Rx = false;

        RadioStackState m_radioStackState;
        UserAircraftData m_userAircraftData;

        int m_audioApi = 0;
        QList<AudioDeviceInfo> m_outputDevices;
        QList<AudioDeviceInfo> m_inputDevices;
        std::map<afv_native::audio::AudioDevice::Api, std::string> m_audioDrivers;

        double scaleValue(double &value, double limitMin, double limitMax, double baseMin, double baseMax) const
        {
            return ((limitMax - limitMin) * (value - baseMin)) / (baseMax - baseMin) + limitMin;
        }
    };
}

#endif // AFV_H

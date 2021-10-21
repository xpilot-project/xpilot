#ifndef AFV_H
#define AFV_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>

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
        Q_PROPERTY(QList<AudioDeviceInfo> outputDeviceList MEMBER m_outputDevices)
        Q_PROPERTY(QList<AudioDeviceInfo> inputDeviceList MEMBER m_inputDevices)

    public:
        AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject* parent = nullptr);
        ~AudioForVatsim();

    private slots:
        void OnNetworkConnected(QString callsign);
        void OnNetworkDisconnected();
        void OnTransceiverTimer();

    signals:
        void notificationPosted(int type, QString message);
        void radioRxChanged(uint radio, bool active);

    private:
        void configureAudioDevices();
        void updateTransceivers();

    private:
        struct event_base* ev_base;
        bool m_keepAlive = false;
        std::shared_ptr<afv_native::Client> mClient;
        std::map<afv_native::audio::AudioDevice::Api, std::string> mAudioDrivers;
        QTimer* m_transceiverTimer;
        QTimer* m_eventTimer;
        QTimer* m_RxTxQueryTimer;
        QThread *m_workerThread;

        bool m_com1Rx = false;
        bool m_com2Rx = false;

        RadioStackState m_radioStackState;
        UserAircraftData m_userAircraftData;

        QList<AudioDeviceInfo> m_outputDevices;
        QList<AudioDeviceInfo> m_inputDevices;
    };
}

#endif // AFV_H

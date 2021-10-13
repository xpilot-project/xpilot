#ifndef AFV_H
#define AFV_H

#include <QObject>
#include <QTimer>
#include <memory>

#include "src/network/networkmanager.h"
#include "src/simulator/xplane_adapter.h"
#include <event2/event.h>
#include "afv-native/Client.h"

namespace xpilot
{
    class AudioForVatsim : public QObject
    {
        Q_OBJECT
    public:
        AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, struct event_base* eventBase, QObject* parent = nullptr);

    private slots:
        void OnNetworkConnected();
        void OnNetworkDisconnected();
        void OnTransceiverTimer();

    private:
        struct event_base* mEventBase;
        std::shared_ptr<afv_native::Client> mClient;
        std::map<afv_native::audio::AudioDevice::Api, std::string> mAudioDrivers;
        QTimer* m_transceiverTimer;
    };
}

#endif // AFV_H

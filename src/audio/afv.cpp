#include "afv.h"

namespace xpilot
{
    AudioForVatsim::AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, struct event_base* eventBase, QObject* parent) :
        QObject(parent),
        mEventBase(eventBase),
        mClient()
    {
        m_transceiverTimer = new QTimer(this);
        m_transceiverTimer->setInterval(5000);

        mClient = std::make_shared<afv_native::Client>(mEventBase, ".", 2, "xPilot");
        mAudioDrivers = afv_native::audio::AudioDevice::getAPIs();

        connect(m_transceiverTimer, &QTimer::timeout, this, &AudioForVatsim::OnTransceiverTimer);
        connect(&networkManager, &NetworkManager::networkConnected, this, &AudioForVatsim::OnNetworkConnected);
        connect(&networkManager, &NetworkManager::networkDisconnected, this, &AudioForVatsim::OnNetworkDisconnected);
        connect(&xplaneAdapter, &XplaneAdapter::pttPressed, this, [&]{
            mClient->setPtt(true);
        });
        connect(&xplaneAdapter, &XplaneAdapter::pttReleased, this, [&]{
            mClient->setPtt(false);
        });
    }

    void AudioForVatsim::OnNetworkConnected()
    {
        m_transceiverTimer->start();
    }

    void AudioForVatsim::OnNetworkDisconnected()
    {
        m_transceiverTimer->stop();
    }

    void AudioForVatsim::OnTransceiverTimer()
    {

    }
}

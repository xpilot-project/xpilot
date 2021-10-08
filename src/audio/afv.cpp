#include "afv.h"

namespace xpilot
{
    AudioForVatsim::AudioForVatsim(struct event_base* eventBase) : mEventBase(eventBase), mClient()
    {
        mClient = std::make_shared<afv_native::Client>(mEventBase, ".", 2, "xPilot-Native");

        mAudioDrivers = afv_native::audio::AudioDevice::getAPIs();

        for(const auto &driver : mAudioDrivers) {
            qDebug() << driver.second.c_str();
        }
    }
}

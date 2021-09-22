#ifndef AFV_H
#define AFV_H

#include <memory>
#include <QDebug>

#include <event2/event.h>
#include "afv-native/Client.h"

class AudioForVatsim
{
public:
    AudioForVatsim(struct event_base* eventBase);

private:
    struct event_base* mEventBase;
    std::shared_ptr<afv_native::Client> mClient;
    std::map<afv_native::audio::AudioDevice::Api, std::string> mAudioDrivers;
};

#endif // AFV_H

#include "notification_sound_engine.h"
#include "config/appconfig.h"

namespace xpilot {

    NotificationSoundEngine::NotificationSoundEngine(QObject *parent) :
        QObject(parent),
        m_audioOutput(this)
    {
        m_media_player = std::make_shared<QMediaPlayer>();
        m_media_player->setAudioOutput(&m_audioOutput);

        if(!AppConfig::getInstance()->NotificationAudioDevice.isEmpty()) {
            setNotificationAudioDevice(AppConfig::getInstance()->NotificationAudioDevice);
        }

        connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &NotificationSoundEngine::outputDevicesChanged);
    }

    NotificationSoundEngine::~NotificationSoundEngine()
    {
        m_media_player->stop();
        m_media_player.reset();
    }

    void NotificationSoundEngine::playAlert()
    {
        playSound(SoundType::Alert);
    }

    void NotificationSoundEngine::playBroadcast()
    {
        playSound(SoundType::Broadcast);
    }

    void NotificationSoundEngine::playDirectRadioMessage()
    {
        playSound(SoundType::DirectRadioMessage);
    }

    void NotificationSoundEngine::playError()
    {
        playSound(SoundType::Error);
    }

    void NotificationSoundEngine::playNewMessage()
    {
        playSound(SoundType::NewMessage);
    }

    void NotificationSoundEngine::playPrivateMessage()
    {
        playSound(SoundType::PrivateMessage);
    }

    void NotificationSoundEngine::playRadioMessage()
    {
        playSound(SoundType::RadioMessage);
    }

    void NotificationSoundEngine::playSelcal()
    {
        playSound(SoundType::SELCAL);
    }

    void NotificationSoundEngine::setNotificationAudioDevice(QString deviceName)
    {
        m_audioOutput.setDevice(getAudioDevice(deviceName));
    }

    QString NotificationSoundEngine::getSoundFilePath(SoundType sound)
    {
        QString fileName;
        switch(sound) {
        case SoundType::Alert:
            fileName = "Alert.wav";
            break;
        case SoundType::Broadcast:
            fileName = "Broadcast.wav";
            break;
        case SoundType::DirectRadioMessage:
            fileName = "DirectRadioMessage.wav";
            break;
        case SoundType::Error:
            fileName = "Error.wav";
            break;
        case SoundType::NewMessage:
            fileName = "NewMessage.wav";
            break;
        case SoundType::PrivateMessage:
            fileName = "PrivateMessage.wav";
            break;
        case SoundType::RadioMessage:
            fileName = "RadioMessage.wav";
            break;
        case SoundType::SELCAL:
            fileName = "SELCAL.wav";
            break;
        }

        return AppConfig::soundsPath() + fileName;
    }

    QAudioDevice NotificationSoundEngine::getAudioDevice(QString deviceName)
    {
        QAudioDevice device;
        auto devices = QMediaDevices::audioOutputs();
        for(int i = 0; i < devices.size(); i++) {
            if(devices.at(i).description() == deviceName) {
                device = devices.at(i);
                break;
            }
        }
        return device;
    }

    void NotificationSoundEngine::playSound(SoundType sound)
    {
        m_media_player->setSource(QUrl(getSoundFilePath(sound)));
        m_media_player->play();
    }
}

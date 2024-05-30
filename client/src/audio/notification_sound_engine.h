#ifndef NOTIFICATION_SOUND_ENGINE_H
#define NOTIFICATION_SOUND_ENGINE_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QList>
#include <QString>

namespace xpilot {

    enum SoundType {
        Alert,
        Broadcast,
        DirectRadioMessage,
        Error,
        NewMessage,
        PrivateMessage,
        RadioMessage,
        SELCAL
    };

    class NotificationSoundEngine : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariant AudioDevices READ getOutputDevices NOTIFY outputDevicesChanged)

    public:
        explicit NotificationSoundEngine(QObject *parent = nullptr);
        ~NotificationSoundEngine();

        Q_INVOKABLE void playAlert();
        Q_INVOKABLE void playBroadcast();
        Q_INVOKABLE void playDirectRadioMessage();
        Q_INVOKABLE void playError();
        Q_INVOKABLE void playNewMessage();
        Q_INVOKABLE void playPrivateMessage();
        Q_INVOKABLE void playRadioMessage();
        Q_INVOKABLE void playSelcal();

        Q_INVOKABLE void setNotificationAudioDevice(QString deviceName);

        QVariant getOutputDevices() const
        {
            QVariantList itemList;

            for(const auto&device : QMediaDevices::audioOutputs()) {
                QVariantMap itemMap;
                itemMap.insert("id", device.id());
                itemMap.insert("name", device.description());
                itemList.append(itemMap);
            }

            return QVariant::fromValue(itemList);
        }

    private:
        std::shared_ptr<QMediaPlayer> m_media_player;
        QString getSoundFilePath(SoundType sound);
        QAudioDevice getAudioDevice(QString deviceName);
        void playSound(SoundType sound);
        QAudioOutput m_audioOutput;
        QMediaDevices m_mediaDevices;

    signals:
        void outputDevicesChanged();
    };

}

#endif // NOTIFICATION_SOUND_ENGINE_H

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

#ifndef AFV_H
#define AFV_H

#include <thread>
#include <memory>

#include <QtGlobal>
#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QVariantList>
#include <QFile>
#include <QTextStream>
#include <QMap>

#include <event2/event.h>

#include "qinjection/dependencypointer.h"
#include "network/networkmanager.h"
#include "simulator/xplane_adapter.h"
#include "controllers/controller_manager.h"
#include "controllers/controller.h"
#include "afv-native/Client.h"
#include "audiodeviceinfo.h"
#include "common/enums.h"

using namespace enums;

namespace xpilot
{
    class AudioForVatsim : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariant OutputDevices READ getOutputDevices NOTIFY outputDevicesChanged)
        Q_PROPERTY(QVariant InputDevices READ getInputDevices NOTIFY inputDevicesChanged)

    public:
        AudioForVatsim(QObject* parent = nullptr);
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

        Q_INVOKABLE void setInputDevice(QString deviceName);
        Q_INVOKABLE void setSpeakerDevice(QString deviceName);
        Q_INVOKABLE void setHeadsetDevice(QString deviceName);
        Q_INVOKABLE void setSplitAudioChannels(bool split);
        Q_INVOKABLE void setCom1Volume(double volume);
        Q_INVOKABLE void setCom2Volume(double volume);
        Q_INVOKABLE void disableAudioEffects(bool disabled);
        Q_INVOKABLE void enableHfSquelch(bool enabled);
        Q_INVOKABLE void setMicrophoneVolume(int volume);
        Q_INVOKABLE void setOnHeadset(unsigned int radio, bool onHeadset);
        Q_INVOKABLE void settingsWindowOpened();
        Q_INVOKABLE void settingsWindowClosed();

    private slots:
        void OnNetworkConnected(QString callsign, bool enableVoice);
        void OnNetworkDisconnected();
        void OnTransceiverTimer();
        void OnAudioDevicesTimer();

    signals:
        void notificationPosted(QString message, MessageType type);
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
        NetworkManager& m_networkManager;
        ControllerManager& m_controllerManager;
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

        bool m_voiceTransmitDisabled = false;
        void EnableVoiceTransmit();
        void DisableVoiceTransmit();

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

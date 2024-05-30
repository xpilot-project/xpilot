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

#include "afv.h"
#include "config/appconfig.h"
#include "common/utils.h"
#include "common/build_config.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace afv_native::afv;

namespace xpilot
{
    float ScaleVolume(float v)
    {
        return 1.0f - sqrt(1.0f - (v * v));
    }

    static void defaultLogger(const char *subsystem, const char *file, int line, const char *outputLine, void* ref)
    {
        auto *self = reinterpret_cast<AudioForVatsim *>(ref);
        self->afvLogger(QString("%1: %2: %3\r\n")
                            .arg(QDateTime::currentDateTimeUtc().toString("MMM dd HH:mm:ss yyyy"))
                            .arg(QString::fromUtf8(subsystem).leftJustified(20, ' '))
                            .arg(outputLine));
    }

    static afv_native::log_fn gLogger = defaultLogger;

    AudioForVatsim::AudioForVatsim(QObject* parent) :
        QObject(parent),
        m_xplaneAdapter(*QInjection::Pointer<XplaneAdapter>().data()),
        m_networkManager(*QInjection::Pointer<NetworkManager>().data()),
        m_controllerManager(*QInjection::Pointer<ControllerManager>().data()),
        m_client()
    {
        QDir afvLogPath(pathAppend(AppConfig::getInstance()->dataRoot(), "AfvLogs"));
        if(!afvLogPath.exists()) {
            afvLogPath.mkpath(".");
        }

        // keep only the last 10 log files
        QFileInfoList files = afvLogPath.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
        const int MAX_LOGS_TO_RETAIN = 10;
        for(int index = files.size(); index >= MAX_LOGS_TO_RETAIN; --index) {
            const QFileInfo &info = files.at(index - 1);
            QFile::remove(info.absoluteFilePath());
        }

        m_afvLog.setFileName(pathAppend(afvLogPath.path(), QString("AfvLog-%1.txt").arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss"))));
        if(m_afvLog.open(QFile::WriteOnly))
        {
            m_logDataStream.setDevice(&m_afvLog);
        }

        m_transceiverTimer.setInterval(5000);
        m_rxTxQueryTimer.setInterval(50);
        m_vuTimer.setInterval(10);
        m_vuTimer.start();
        m_audioDevicesTimer.setInterval(500);

#ifdef Q_OS_WIN
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);
#endif

        afv_native::setLogger(gLogger, this);

        QString clientName = QString("xPilot %1").arg(BuildConfig::getVersionString());

        ev_base = event_base_new();
        m_client = std::make_shared<afv_native::Client>(ev_base, 2, clientName.toStdString().c_str());
        m_client->ClientEventCallback.addCallback(nullptr, [&](afv_native::ClientEventType evt, void* data, void* data2)
        {
            switch(evt)
            {
                case afv_native::ClientEventType::APIServerError:
                    if(data != nullptr) {
                        auto error = *reinterpret_cast<APISessionError*>(data);
                        switch(error) {
                            case APISessionError::BadPassword:
                            case APISessionError::RejectedCredentials:
                                emit notificationPosted("Error connecting to voice server. Please check your VATSIM credentials and try again.", MessageType::Error);
                                break;
                            case APISessionError::ConnectionError:
                                emit notificationPosted("Error initiating voice server connection.", MessageType::Error);
                                break;
                            case APISessionError::AuthTokenExpiryTimeInPast:
                                emit notificationPosted("Voice server auth token expiry time is in the past. Please make sure your system clock is synchronized.", MessageType::Error);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case afv_native::ClientEventType::VoiceServerChannelError:
                    if(data != nullptr) {
                        int error = *reinterpret_cast<int*>(data);
                        emit notificationPosted(QString("Voice server error: %s").arg(error), MessageType::Error);
                    }
                    break;
                case afv_native::ClientEventType::VoiceServerError:
                    if(data != nullptr) {
                        auto error = *reinterpret_cast<VoiceSessionError*>(data);
                        switch(error) {
                            case VoiceSessionError::BadResponseFromAPIServer:
                                emit notificationPosted("Voice server error: BadResponseFromAPIServer", MessageType::Error);
                                break;
                            case VoiceSessionError::Timeout:
                                emit notificationPosted("Voice server error: Timeout", MessageType::Error);
                                break;
                            case VoiceSessionError::UDPChannelError:
                                emit notificationPosted("Voice server error: UDPChannelError", MessageType::Error);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case afv_native::ClientEventType::StationAliasesUpdated:
                    {
                        auto stations = m_client->getStationAliases();
                        m_aliasedStations = QVector<afv_native::afv::dto::Station>(stations.begin(), stations.end());
                    }
                    break;
                case afv_native::ClientEventType::VoiceServerConnected:
                    emit notificationPosted("Connected to voice server.", MessageType::Info);
                    break;
                case afv_native::ClientEventType::VoiceServerDisconnected:
                    emit notificationPosted("Disconnected from voice server.", MessageType::Info);
                    break;
                case afv_native::ClientEventType::AudioError:
                    if(data != nullptr) {
                        const char* error = reinterpret_cast<const char*>(data);
                        QMetaObject::invokeMethod(this, [this, error]() {
                            emit notificationPosted(error, MessageType::Error);
                        }, Qt::QueuedConnection);
                    }
                    break;
                default:
                    break;
            }
        });
        m_client->setEnableInputFilters(true);
        m_client->setEnableOutputEffects(!AppConfig::getInstance()->AudioEffectsDisabled);
        m_client->setEnableHfSquelch(AppConfig::getInstance()->HFSquelchEnabled);

        configureAudioDevices();
        setMicrophoneVolume(AppConfig::getInstance()->MicrophoneVolume);
        setCom1Volume(AppConfig::getInstance()->Com1Volume);
        setCom2Volume(AppConfig::getInstance()->Com2Volume);

        connect(&m_audioDevicesTimer, &QTimer::timeout, this, &AudioForVatsim::OnAudioDevicesTimer);
        connect(&m_transceiverTimer, &QTimer::timeout, this, &AudioForVatsim::OnTransceiverTimer);
        connect(&m_rxTxQueryTimer, &QTimer::timeout, this, [&]{
            emit radioRxChanged(0, m_radioStackState.Com1ReceiveEnabled && m_client->getRxActive(0));
            emit radioRxChanged(1, m_radioStackState.Com2ReceiveEnabled && m_client->getRxActive(1));

            m_xplaneAdapter.setComRxDataref(0, m_client->getRxActive(0));
            m_xplaneAdapter.setComRxDataref(1, m_client->getRxActive(1));
        });
        connect(&m_vuTimer, &QTimer::timeout, this, [=]{
            double vu = m_client->getInputPeak();
            emit inputVuChanged(vu);
            m_xplaneAdapter.setVuDataref(vu);
        });
        connect(&m_networkManager, &NetworkManager::networkConnected, this, &AudioForVatsim::OnNetworkConnected);
        connect(&m_networkManager, &NetworkManager::networkDisconnected, this, &AudioForVatsim::OnNetworkDisconnected);
        connect(&m_networkManager, &NetworkManager::disableVoiceTransmit, this, [&] {
            DisableVoiceTransmit();
        });
        connect(&m_networkManager, &NetworkManager::muteReceived, this, [&](bool mute) {
            if(mute) {
                DisableVoiceTransmit();
            } else {
                EnableVoiceTransmit();
            }
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, [&](RadioStackState state){
            if(state != m_radioStackState) {
                m_radioStackState = state;

                if(m_radioStackState.Com1TransmitEnabled) {
                    m_client->setTxRadio(0);
                }
                else if(m_radioStackState.Com2TransmitEnabled) {
                    m_client->setTxRadio(1);
                }

                if(AppConfig::getInstance()->AircraftRadioStackControlsVolume) {
                    setCom1Volume(m_radioStackState.Com1Volume);
                    setCom2Volume(m_radioStackState.Com2Volume);
                }

                updateTransceivers();
            }
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::userAircraftDataChanged, this, [&](UserAircraftData data){
            if(data != m_userAircraftData) {
                m_userAircraftData = data;
            }
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::pttPressed, this, [&]{
            if(m_voiceTransmitDisabled) {
                m_client->setPtt(false);
            }
            else {
                m_client->setPtt(true);
            }
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::pttReleased, this, [&]{
            m_client->setPtt(false);
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::com1OnHeadsetChanged, this, [&](bool onHeadset) {
            setOnHeadset(0, onHeadset);
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::com2OnHeadsetChanged, this, [&](bool onHeadset) {
            setOnHeadset(1, onHeadset);
        });
        connect(&m_xplaneAdapter, &XplaneAdapter::splitAudioChannelsChanged, this, [&](bool split) {
            setSplitAudioChannels(split);
        });

        connect(&m_controllerManager, &ControllerManager::controllerAdded, this, [&](Controller controller)
        {
            m_controllers.push_back(controller);
        });
        connect(&m_controllerManager, &ControllerManager::controllerUpdated, this, [&](Controller controller)
        {
            auto it = std::find_if(m_controllers.begin(), m_controllers.end(), [=](Controller &c)
            {
                return c.Callsign == controller.Callsign;
            });

            if(it != m_controllers.constEnd())
            {
                *it = controller;
            }
        });
        connect(&m_controllerManager, &ControllerManager::controllerDeleted, this, [&](Controller controller)
        {
            auto it = std::find_if(m_controllers.begin(), m_controllers.end(), [=](Controller &c)
            {
                return c.Callsign == controller.Callsign;
            });
            if(it != m_controllers.end())
            {
                m_controllers.removeAll(*it);
            }
        });
        connect(this, &AudioForVatsim::notificationPosted, this, [&](QString message, MessageType type)
        {
            m_xplaneAdapter.NotificationPosted(message, toColorHex(type));
        });

        m_keepAlive = true;
        m_workerThread = QThread::create([&]{
            while(m_keepAlive)
            {
                event_base_loop(ev_base, EVLOOP_NONBLOCK);
#ifdef Q_OS_WIN
                Sleep(10);
#else
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
            }
        });
        m_workerThread->start();
    }

    AudioForVatsim::~AudioForVatsim()
    {
        m_keepAlive = false;
        m_workerThread->terminate();
        m_workerThread->deleteLater();
        m_client.reset();
#ifdef Q_OS_WIN
        WSACleanup();
#endif
    }

    void AudioForVatsim::afvLogger(QString message)
    {
        m_logDataStream << message;
        m_logDataStream.flush();
    }

    void AudioForVatsim::setInputDevice(QString deviceName)
    {
        if(!deviceName.isEmpty()) {
            m_client->setMicrophoneDevice(deviceName.toStdString().c_str());
            m_client->startMicrophone();
        }
    }

    void AudioForVatsim::setSpeakerDevice(QString deviceName)
    {
        if(!deviceName.isEmpty()) {
            m_client->setSpeakerDevice(deviceName.toStdString().c_str());
            m_client->startSpeaker();
        }
    }

    void AudioForVatsim::setHeadsetDevice(QString deviceName)
    {
        if(!deviceName.isEmpty()) {
            m_client->setHeadsetDevice(deviceName.toStdString().c_str());
            m_client->startHeadset();
        }
    }

    void AudioForVatsim::setCom1Volume(double volume)
    {
        double v = volume;
        if(v < 0) v = 0;
        if(v > 100) v = 100;

        m_client->setRadioGain(0, ScaleVolume(v / 100.0f));

        AppConfig::getInstance()->Com1Volume = v;
        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::setCom2Volume(double volume)
    {
        double v = volume;
        if(v < 0) v = 0;
        if(v > 100) v = 100;

        m_client->setRadioGain(1, ScaleVolume(v / 100.0f));

        AppConfig::getInstance()->Com2Volume = v;
        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::disableAudioEffects(bool disabled)
    {
        m_client->setEnableOutputEffects(!disabled);
        AppConfig::getInstance()->AudioEffectsDisabled = disabled;
        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::enableHfSquelch(bool enabled)
    {
        m_client->setEnableHfSquelch(enabled);
        AppConfig::getInstance()->HFSquelchEnabled = enabled;
        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::OnNetworkConnected(QString callsign, bool enableVoice)
    {
        if(!enableVoice)
            return;

        m_client->setCallsign(callsign.toStdString());
        m_client->setCredentials(AppConfig::getInstance()->VatsimId.toStdString(), AppConfig::getInstance()->VatsimPasswordDecrypted.toStdString());
        m_client->connect();
        m_transceiverTimer.start();
        m_rxTxQueryTimer.start();
        m_xplaneAdapter.EnableVoiceTransmit();
    }

    void AudioForVatsim::OnNetworkDisconnected()
    {
        emit radioRxChanged(0, false);
        emit radioRxChanged(1, false);

        m_client->disconnect();
        m_transceiverTimer.stop();
        m_rxTxQueryTimer.stop();
        m_xplaneAdapter.EnableVoiceTransmit();
    }

    void AudioForVatsim::OnTransceiverTimer()
    {
        updateTransceivers();
    }

    void AudioForVatsim::OnAudioDevicesTimer()
    {
        // output devices
        QList<AudioDeviceInfo> newOutputDevices;
        for(const auto& device: afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(0))
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = (QChar)device.first;
            newOutputDevices.append(audioDevice);
        }

        bool outputDeviceListChanged = !std::equal(std::begin(m_outputDevices), std::end(m_outputDevices),
                                                std::begin(newOutputDevices), std::end(newOutputDevices));
        if(outputDeviceListChanged) {
            m_outputDevices.clear();
            m_outputDevices = newOutputDevices;
            emit outputDevicesChanged();
        }

        // input devices
        QList<AudioDeviceInfo> newInputDevices;
        for(const auto& device: afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(0))
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = (QChar)device.first;
            newInputDevices.append(audioDevice);
        }

        bool inputDeviceListChanged = !std::equal(std::begin(m_inputDevices), std::end(m_inputDevices),
                                                std::begin(newInputDevices), std::end(newInputDevices));
        if(inputDeviceListChanged) {
            m_inputDevices.clear();
            m_inputDevices = newInputDevices;
            emit inputDevicesChanged();
        }
    }

    void AudioForVatsim::configureAudioDevices()
    {
        m_client->stopAudio();

        m_outputDevices.clear();
        m_inputDevices.clear();

        auto outputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(0);
        for(const auto& device: outputDevices)
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = (QChar)device.first;
            m_outputDevices.append(audioDevice);
        }

        emit outputDevicesChanged();

        auto inputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(0);
        for(const auto& device: inputDevices)
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = (QChar)device.first;
            m_inputDevices.append(audioDevice);
        }

        emit inputDevicesChanged();

        if(!AppConfig::getInstance()->InputDevice.isEmpty())
        {
            m_client->setMicrophoneDevice(AppConfig::getInstance()->InputDevice.toStdString());
        }

        if(!AppConfig::getInstance()->SpeakerDevice.isEmpty())
        {
            m_client->setSpeakerDevice(AppConfig::getInstance()->SpeakerDevice.toStdString());
        }

        if(!AppConfig::getInstance()->HeadsetDevice.isEmpty())
        {
            m_client->setHeadsetDevice(AppConfig::getInstance()->HeadsetDevice.toStdString());
        }

        setSplitAudioChannels(AppConfig::getInstance()->SplitAudioChannels);
        setOnHeadset(0, AppConfig::getInstance()->Com1OnHeadset);
        setOnHeadset(1, AppConfig::getInstance()->Com2OnHeadset);

        m_client->startAudio();
    }

    void AudioForVatsim::updateTransceivers()
    {
        quint32 com1Alias = getAliasFrequency(m_radioStackState.Com1Frequency * 1000);
        quint32 com2Alias = getAliasFrequency(m_radioStackState.Com2Frequency * 1000);

        com1Alias > 0 ? (emit radioAliasChanged(0, com1Alias)) : (emit radioAliasChanged(0, 0));
        com2Alias > 0 ? (emit radioAliasChanged(1, com2Alias)) : (emit radioAliasChanged(1, 0));

        m_client->setRadioState(0, m_radioStackState.Com1ReceiveEnabled && (m_radioStackState.AvionicsPowerOn || m_radioStackState.OverrideRadioPower)
                                       ? (com1Alias > 0 ? com1Alias : m_radioStackState.Com1Frequency * 1000) : 0);
        m_client->setRadioState(1, m_radioStackState.Com2ReceiveEnabled && (m_radioStackState.AvionicsPowerOn || m_radioStackState.OverrideRadioPower)
                                       ? (com2Alias > 0 ? com2Alias : m_radioStackState.Com2Frequency * 1000) : 0);
        m_client->setClientPosition(m_userAircraftData.Latitude, m_userAircraftData.Longitude, m_userAircraftData.AltitudeMslM,
                                    m_userAircraftData.AltitudeAglM);
    }

    void AudioForVatsim::EnableVoiceTransmit()
    {
        m_voiceTransmitDisabled = false;
        m_xplaneAdapter.EnableVoiceTransmit();
    }

    void AudioForVatsim::DisableVoiceTransmit()
    {
        m_voiceTransmitDisabled = true;
        m_xplaneAdapter.DisableVoiceTransmit();
    }

    void AudioForVatsim::setMicrophoneVolume(int volume)
    {
        m_client->setMicrophoneVolume(volume);
    }

    void AudioForVatsim::setOnHeadset(unsigned int radio, bool onHeadset)
    {
        m_client->setOnHeadset(radio, onHeadset);

        if(radio == 0) {
            AppConfig::getInstance()->Com1OnHeadset = onHeadset;
            m_xplaneAdapter.setCom1OnHeadset(onHeadset);
        }
        else {
            AppConfig::getInstance()->Com2OnHeadset = onHeadset;
            m_xplaneAdapter.setCom2OnHeadset(onHeadset);
        }

        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::setSplitAudioChannels(bool split)
    {
        m_client->stopAudio();
        m_client->setSplitAudioChannels(split);
        m_client->startAudio();

        AppConfig::getInstance()->SplitAudioChannels = split;

        m_xplaneAdapter.setSplitAudioChannels(split);

        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::settingsWindowOpened()
    {
        m_audioDevicesTimer.start();
    }

    void AudioForVatsim::settingsWindowClosed()
    {
        m_audioDevicesTimer.stop();
    }

    bool AudioForVatsim::fuzzyMatchCallsign(const QString &callsign, const QString &compareTo) const
    {
        if(callsign.isEmpty() || compareTo.isEmpty())
        {
            return false;
        }

        QString prefixA;
        QString suffixA;
        QString prefixB;
        QString suffixB;
        this->getPrefixSuffix(callsign, prefixA, suffixA);
        this->getPrefixSuffix(compareTo, prefixB, suffixB);
        return (prefixA == prefixB) && (suffixA == suffixB);
    }

    void AudioForVatsim::getPrefixSuffix(const QString &callsign, QString &prefix, QString &suffix) const
    {
        const QRegularExpression separator("[(\\-|_)]");
        const QStringList parts = callsign.split(separator);

        prefix = parts.size() > 0 ? parts.first() : QString();
        suffix = parts.size() > 1 ? parts.last() : QString();
    }

    quint32 AudioForVatsim::getAliasFrequency(quint32 frequency) const
    {
        auto it = std::find_if(m_controllers.constBegin(), m_controllers.constEnd(), [&](const Controller &c)
        {
            return c.FrequencyHz == frequency;
        });

        if(it != m_controllers.constEnd())
        {
            auto alias = std::find_if(m_aliasedStations.constBegin(), m_aliasedStations.constEnd(), [&](const afv_native::afv::dto::Station &station)
            {
                return it->FrequencyHz == station.FrequencyAlias && fuzzyMatchCallsign(station.Name.c_str(), it->Callsign);
            });

            if(alias != m_aliasedStations.constEnd())
            {
                return alias->Frequency;
            }
        }

        return 0;
    }
}

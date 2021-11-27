#include "afv.h"
#include "src/appcore.h"
#include "src/config/appconfig.h"
#include "src/common/notificationtype.h"
#include "src/common/utils.h"

#include <QMap>

using namespace afv_native::afv;

namespace xpilot
{
    AudioForVatsim::AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, ControllerManager& controllerManager, QObject* parent) :
        QObject(parent),
        m_xplaneAdapter(xplaneAdapter),
        m_client()
    {
        m_transceiverTimer = new QTimer(this);
        m_transceiverTimer->setInterval(5000);

        m_rxTxQueryTimer = new QTimer(this);
        m_rxTxQueryTimer->setInterval(50);

        m_vuTimer = new QTimer(this);
        m_vuTimer->setInterval(50);
        m_vuTimer->start();

#ifdef Q_OS_WIN
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);
#endif

        afv_native::setLogger(nullptr);

        ev_base = event_base_new();
        m_client = std::make_shared<afv_native::Client>(ev_base, 2, "xPilot");
        m_client->ClientEventCallback.addCallback(nullptr, [&](afv_native::ClientEventType evt, void* data)
        {
            switch(evt)
            {
            case afv_native::ClientEventType::APIServerError:
                if(data != nullptr) {
                    auto error = *reinterpret_cast<APISessionError*>(data);
                    switch(error) {
                    case APISessionError::BadPassword:
                    case APISessionError::RejectedCredentials:
                        emit notificationPosted((int)NotificationType::Error, "Error connecting to voice server. Please check your VATSIM credentials and try again.");
                        break;
                    case APISessionError::ConnectionError:
                        emit notificationPosted((int)NotificationType::Error, "Error initiating voice server connection.");
                        break;
                    default:
                        break;
                    }
                }
                break;
            case afv_native::ClientEventType::VoiceServerChannelError:
                if(data != nullptr) {
                    int error = *reinterpret_cast<int*>(data);
                    emit notificationPosted((int)NotificationType::Error, QString("Voice server error: %s").arg(error));
                }
                break;
            case afv_native::ClientEventType::VoiceServerError:
                if(data != nullptr) {
                    auto error = *reinterpret_cast<VoiceSessionError*>(data);
                    switch(error) {
                    case VoiceSessionError::BadResponseFromAPIServer:
                        emit notificationPosted((int)NotificationType::Error, "Voice server error: BadResponseFromAPIServer");
                        break;
                    case VoiceSessionError::Timeout:
                        emit notificationPosted((int)NotificationType::Error, "Voice server error: Timeout");
                        break;
                    case VoiceSessionError::UDPChannelError:
                        emit notificationPosted((int)NotificationType::Error, "Voice server error: UDPChannelError");
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
                emit notificationPosted((int)NotificationType::Info, "Connected to voice server.");
                break;
            case afv_native::ClientEventType::VoiceServerDisconnected:
                emit notificationPosted((int)NotificationType::Info, "Disconnected from voice server.");
                break;
            default:
                break;
            }
        });
        m_client->setEnableInputFilters(true);
        m_client->setEnableOutputEffects(!AppConfig::getInstance()->AudioEffectsDisabled);
        m_client->setEnableHfSquelch(AppConfig::getInstance()->HFSquelchEnabled);

        m_audioDrivers = afv_native::audio::AudioDevice::getAPIs();
        for(const auto& driver : m_audioDrivers)
        {
            if(QString(driver.second.c_str()) == AppConfig::getInstance()->AudioApi)
            {
                m_audioApi = driver.first;
            }
        }

        QTimer::singleShot(0, this, [this]{
            if(m_audioDrivers.empty())
            {
                emit notificationPosted((int)NotificationType::Error, "Could not initialize audio drivers or devices.");
            }
            else
            {
                configureAudioDevices();
            }
        });

        connect(m_transceiverTimer, &QTimer::timeout, this, &AudioForVatsim::OnTransceiverTimer);
        connect(m_rxTxQueryTimer, &QTimer::timeout, this, [=]{
            emit radioRxChanged(0, m_radioStackState.Com1ReceiveEnabled && m_client->getRxActive(0));
            emit radioRxChanged(1, m_radioStackState.Com2ReceiveEnabled && m_client->getRxActive(1));
        });
        connect(m_vuTimer, &QTimer::timeout, this, [=]{
            if(!AppConfig::getInstance()->InputDevice.isEmpty())
            {
                double vu = m_client->getInputPeak();
                emit inputVuChanged(scaleValue(vu, 0, 100, -40, 0));
            }
        });
        connect(&networkManager, &NetworkManager::networkConnected, this, &AudioForVatsim::OnNetworkConnected);
        connect(&networkManager, &NetworkManager::networkDisconnected, this, &AudioForVatsim::OnNetworkDisconnected);
        connect(&xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, [&](RadioStackState state){
            if(state != m_radioStackState) {
                m_radioStackState = state;

                if(m_radioStackState.Com1TransmitEnabled) {
                    m_client->setTxRadio(0);
                }
                else if(m_radioStackState.Com2TransmitEnabled) {
                    m_client->setTxRadio(1);
                }

                m_client->setRadioGain(0, m_radioStackState.Com1Volume / 100.0);
                m_client->setRadioGain(1, m_radioStackState.Com2Volume / 100.0);

                updateTransceivers();
            }
        });
        connect(&xplaneAdapter, &XplaneAdapter::userAircraftDataChanged, this, [&](UserAircraftData data){
            if(data != m_userAircraftData) {
                m_userAircraftData = data;
            }
        });
        connect(&xplaneAdapter, &XplaneAdapter::pttPressed, this, [&]{
            m_client->setPtt(true);
        });
        connect(&xplaneAdapter, &XplaneAdapter::pttReleased, this, [&]{
            m_client->setPtt(false);
        });

        connect(&controllerManager, &ControllerManager::controllerAdded, this, [&](Controller controller)
        {
            m_controllers.push_back(controller);
        });
        connect(&controllerManager, &ControllerManager::controllerUpdated, this, [&](Controller controller)
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
        connect(&controllerManager, &ControllerManager::controllerDeleted, this, [&](Controller controller)
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
        connect(this, &AudioForVatsim::notificationPosted, this, [&](int type, QString message)
        {
            auto msgType = static_cast<NotificationType>(type);
            switch(msgType)
            {
            case NotificationType::Error:
                m_xplaneAdapter.NotificationPosted(message, COLOR_RED);
                break;
            case NotificationType::Info:
                m_xplaneAdapter.NotificationPosted(message, COLOR_YELLOW);
                break;
            case NotificationType::RadioMessageSent:
                m_xplaneAdapter.NotificationPosted(message, COLOR_CYAN);
                break;
            case NotificationType::ServerMessage:
                m_xplaneAdapter.NotificationPosted(message, COLOR_GREEN);
                break;
            case NotificationType::Warning:
                m_xplaneAdapter.NotificationPosted(message, COLOR_ORANGE);
                break;
            }
        });

        m_keepAlive = true;
        m_workerThread = QThread::create([&]{
            while(m_keepAlive)
            {
                event_base_loop(ev_base, EVLOOP_NONBLOCK);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    void AudioForVatsim::setAudioApi(int api)
    {
        m_audioApi = api;
        m_client->setAudioApi(api);
        configureAudioDevices();
    }

    void AudioForVatsim::setInputDevice(QString deviceName)
    {
        m_client->stopAudio();
        m_client->setAudioInputDevice(deviceName.toStdString().c_str());
        m_client->startAudio();
    }

    void AudioForVatsim::setOutputDevice(QString deviceName)
    {
        m_client->stopAudio();
        m_client->setAudioOutputDevice(deviceName.toStdString().c_str());
        m_client->startAudio();
    }

    void AudioForVatsim::setCom1Volume(double volume)
    {
        double v = volume;
        if(v < 0) v = 0;
        if(v > 100) v = 100;
        m_client->setRadioGain(0, v / 100.0f);

        AppConfig::getInstance()->Com1Volume = v;
        AppConfig::getInstance()->saveConfig();
    }

    void AudioForVatsim::setCom2Volume(double volume)
    {
        double v = volume;
        if(v < 0) v = 0;
        if(v > 100) v = 100;
        m_client->setRadioGain(1, v / 100.0f);

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
        {
            return;
        }

        configureAudioDevices();

        m_client->setCallsign(callsign.toStdString());
        m_client->setCredentials(AppConfig::getInstance()->VatsimId.toStdString(), AppConfig::getInstance()->VatsimPasswordDecrypted.toStdString());
        m_client->connect();
        m_transceiverTimer->start();
        m_rxTxQueryTimer->start();
    }

    void AudioForVatsim::OnNetworkDisconnected()
    {
        emit radioRxChanged(0, false);
        emit radioRxChanged(1, false);

        m_client->disconnect();
        m_transceiverTimer->stop();
        m_rxTxQueryTimer->stop();
    }

    void AudioForVatsim::OnTransceiverTimer()
    {
        updateTransceivers();
    }

    void AudioForVatsim::configureAudioDevices()
    {
        m_client->stopAudio();

        m_outputDevices.clear();
        m_inputDevices.clear();

        auto outputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(m_audioApi);
        for(const auto& device: outputDevices)
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = device.first;
            m_outputDevices.append(audioDevice);
        }

        emit outputDevicesChanged();

        auto inputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(m_audioApi);
        for(const auto& device: inputDevices)
        {
            AudioDeviceInfo audioDevice{};
            audioDevice.DeviceName = device.second.name.c_str();
            audioDevice.Id = device.first;
            m_inputDevices.append(audioDevice);
        }

        emit inputDevicesChanged();

        if(!AppConfig::getInstance()->InputDevice.isEmpty())
        {
            m_client->setAudioInputDevice(AppConfig::getInstance()->InputDevice.toStdString());
        }

        if(!AppConfig::getInstance()->OutputDevice.isEmpty())
        {
            m_client->setAudioOutputDevice(AppConfig::getInstance()->OutputDevice.toStdString());
        }

        m_client->setRadioGain(0, AppConfig::getInstance()->Com1Volume / 100.0f);
        m_client->setRadioGain(1, AppConfig::getInstance()->Com2Volume / 100.0f);

        m_client->startAudio();
    }

    void AudioForVatsim::updateTransceivers()
    {
        quint32 com1Alias = getAliasFrequency(m_radioStackState.Com1Frequency * 1000);
        quint32 com2Alias = getAliasFrequency(m_radioStackState.Com2Frequency * 1000);

        com1Alias > 0 ? (emit radioAliasChanged(0, com1Alias)) : (emit radioAliasChanged(0, 0));
        com2Alias > 0 ? (emit radioAliasChanged(1, com2Alias)) : (emit radioAliasChanged(1, 0));

        m_client->setRadioState(0, m_radioStackState.Com1ReceiveEnabled ? (com1Alias > 0 ? com1Alias : m_radioStackState.Com1Frequency * 1000) : 0);
        m_client->setRadioState(1, m_radioStackState.Com2ReceiveEnabled ? (com2Alias > 0 ? com2Alias : m_radioStackState.Com2Frequency * 1000) : 0);
        m_client->setClientPosition(m_userAircraftData.Latitude, m_userAircraftData.Longitude, m_userAircraftData.AltitudeMslM, m_userAircraftData.AltitudePressure * 0.3048);
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

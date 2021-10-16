#include "afv.h"
#include "src/appcore.h"
#include "src/config/appconfig.h"

using namespace afv_native::afv;

namespace xpilot
{
    AudioForVatsim::AudioForVatsim(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject* parent) :
        QObject(parent),
        mClient()
    {
        m_transceiverTimer = new QTimer(this);
        m_transceiverTimer->setInterval(5000);

        m_RxTxQueryTimer = new QTimer(this);
        m_RxTxQueryTimer->setInterval(50);

#ifdef Q_OS_WIN
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);
#endif

        ev_base = event_base_new();
        mClient = std::make_shared<afv_native::Client>(ev_base, "afv-samples", 2, "xPilot", "https://afv.vatsim.dev");
        mClient->ClientEventCallback.addCallback(nullptr, [&](afv_native::ClientEventType evt, void* data)
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
                    }
                }
                break;
            case afv_native::ClientEventType::StationAliasesUpdated:
                break;
            case afv_native::ClientEventType::VoiceServerConnected:
                emit notificationPosted((int)NotificationType::Info, "Connected to voice server.");
                break;
            case afv_native::ClientEventType::VoiceServerDisconnected:
                emit notificationPosted((int)NotificationType::Info, "Disconnected from voice server.");
                break;
            }
        });
        mClient->setEnableInputFilters(true);
        mAudioDrivers = afv_native::audio::AudioDevice::getAPIs();

        //        for(const auto& driver : mAudioDrivers)
        //        {
        //            qDebug() << driver.first << ": " << driver.second.c_str();
        //        }

        connect(m_transceiverTimer, &QTimer::timeout, this, &AudioForVatsim::OnTransceiverTimer);
        connect(m_RxTxQueryTimer, &QTimer::timeout, this, [=]{
            emit radioRxChanged(0, m_radioStackState.Com1ReceiveEnabled && mClient->getRxActive(0));
            emit radioRxChanged(1, m_radioStackState.Com2ReceiveEnabled && mClient->getRxActive(1));
        });
        connect(&networkManager, &NetworkManager::networkConnected, this, &AudioForVatsim::OnNetworkConnected);
        connect(&networkManager, &NetworkManager::networkDisconnected, this, &AudioForVatsim::OnNetworkDisconnected);
        connect(&xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, [&](RadioStackState state){
            if(state != m_radioStackState) {
                m_radioStackState = state;

                if(m_radioStackState.Com1TransmitEnabled) {
                    mClient->setTxRadio(0);
                }
                else if(m_radioStackState.Com2TransmitEnabled) {
                    mClient->setTxRadio(1);
                }

                updateTransceivers();
            }
        });
        connect(&xplaneAdapter, &XplaneAdapter::userAircraftDataChanged, this, [&](UserAircraftData data){
            if(data != m_userAircraftData) {
                m_userAircraftData = data;
            }
        });
        connect(&xplaneAdapter, &XplaneAdapter::pttPressed, this, [&]{
            mClient->setPtt(true);
        });
        connect(&xplaneAdapter, &XplaneAdapter::pttReleased, this, [&]{
            mClient->setPtt(false);
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
        mClient.reset();
#ifdef Q_OS_WIN
        WSACleanup();
#endif
    }

    void AudioForVatsim::OnNetworkConnected(QString callsign)
    {
        configureAudioDevices();

        mClient->setCallsign(callsign.toStdString());
        mClient->setCredentials(AppConfig::getInstance()->VatsimId.toStdString(), AppConfig::getInstance()->VatsimPasswordDecrypted.toStdString());
        mClient->connect();
        m_transceiverTimer->start();
        m_RxTxQueryTimer->start();
    }

    void AudioForVatsim::OnNetworkDisconnected()
    {
        emit radioRxChanged(0, false);
        emit radioRxChanged(0, false);

        mClient->disconnect();
        m_transceiverTimer->stop();
        m_RxTxQueryTimer->stop();
    }

    void AudioForVatsim::OnTransceiverTimer()
    {
        updateTransceivers();
    }

    void AudioForVatsim::configureAudioDevices()
    {
        mClient->setAudioApi(2);

        auto inputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(2);
        for(const auto& device: inputDevices)
        {
            qDebug() << "\"" << device.second.name.c_str() << "\"";
        }

        auto outputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(2);
        for(const auto& device: outputDevices)
        {
            qDebug() << "\"" << device.second.name.c_str() << "\"";
        }

        mClient->setAudioOutputDevice("Speakers (C-Media USB Audio Device   )");
        mClient->setAudioInputDevice("Microphone (C-Media USB Audio Device   )");
        mClient->startAudio();
    }

    void AudioForVatsim::updateTransceivers()
    {
        mClient->setRadioState(0, m_radioStackState.Com1ReceiveEnabled ? m_radioStackState.Com1Frequency * 1000 : 0);
        mClient->setRadioState(1, m_radioStackState.Com2ReceiveEnabled ? m_radioStackState.Com2Frequency * 1000 : 0);

        mClient->setRadioGain(0, 1.0f);
        mClient->setRadioGain(1, 1.0f);

        mClient->setClientPosition(m_userAircraftData.Latitude, m_userAircraftData.Longitude, m_userAircraftData.AltitudeMslM, m_userAircraftData.AltitudeAglM);
    }
}

#include "xplane_adapter.h"
#include "src/config/appconfig.h"
#include "src/common/build_config.h"
#include "src/common/utils.h"

#include <iostream>
#include <istream>
#include <iomanip>
#include <string>

#include <QTimer>
#include <QtEndian>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <cmath>

using namespace xpilot;

constexpr int HEARTBEAT_TIMEOUT_SECS = 15;

enum DataRef
{
    AvionicsPower,
    AudioComSelection,
    Com1AudioSelection,
    Com2AudioSelection,
    Com1Frequency,
    Com2Frequency,
    Com1Volume,
    Com2Volume,
    TransponderMode,
    TransponderIdent,
    TransponderCode,
    BeaconLights,
    LandingLights,
    TaxiLights,
    NavLights,
    StrobeLights,
    Latitude,
    Longitude,
    AltitudeMsl,
    AltitudeAgl,
    AltitudePressure,
    BarometerSeaLevel,
    GroundSpeed,
    Pitch,
    Heading,
    Bank,
    LatitudeVelocity,
    AltitudeVelocity,
    LongitudeVelocity,
    PitchVelocity,
    HeadingVelocity,
    BankVelocity,
    EngineCount,
    Engine1Running,
    Engine2Running,
    Engine3Running,
    Engine4Running,
    Engine1Reversing,
    Engine2Reversing,
    Engine3Reversing,
    Engine4Reversing,
    OnGround,
    GearDown,
    FlapRatio,
    SpeedbrakeRatio,
    NoseWheelAngle,
    ReplayMode,
    Paused,
    PushToTalk,
    SelcalMuteOverride,
    XplaneVersionNumber
};

QHostAddress m_hostAddress;

XplaneAdapter::XplaneAdapter(QObject* parent) : QObject(parent)
{
    m_lastUdpTimestamp = QDateTime::currentSecsSinceEpoch() - HEARTBEAT_TIMEOUT_SECS; // initialize timestamp in the past to prevent ghost connection status

    socket = new QUdpSocket(this);

    m_hostAddress = QHostAddress(AppConfig::getInstance()->XplaneNetworkAddress);
    if(AppConfig::getInstance()->XplaneNetworkAddress.toLower() == "localhost")
    {
        // udp socket doesn't work if the address is "localhost" so we need to convert it
        m_hostAddress = QHostAddress::LocalHost;
    }
    socket->bind(QHostAddress::AnyIPv4);

    connect(socket, &QUdpSocket::readyRead, this, &XplaneAdapter::OnDataReceived);

    nng_pair1_open(&m_socket);
    nng_setopt_int(m_socket, NNG_OPT_RECVBUF, 1024);
    nng_setopt_int(m_socket, NNG_OPT_SENDBUF, 1024);

    const QList<QString> localhostAddresses = {"127.0.0.1","localhost"};
    if(AppConfig::getInstance()->XplaneNetworkAddress.isEmpty() || localhostAddresses.contains(AppConfig::getInstance()->XplaneNetworkAddress.toLower())) {
        nng_dial(m_socket, "ipc:///tmp//xpilot.ipc", NULL, NNG_FLAG_NONBLOCK);
    } else {
        QString url = QString("tcp://%1:%2").arg(AppConfig::getInstance()->XplaneNetworkAddress).arg(AppConfig::getInstance()->XplanePluginPort);
        nng_dial(m_socket, url.toStdString().c_str(), NULL, NNG_FLAG_NONBLOCK);
    }

    m_keepSocketAlive = true;
    m_socketThread = std::make_unique<std::thread>([&]{
        while (m_keepSocketAlive) {
            char* buffer;
            size_t bufferLen;

            int err;
            err = nng_recv(m_socket, &buffer, &bufferLen, NNG_FLAG_ALLOC);

            if(err == 0)
            {
                BaseDto dto;
                auto obj = msgpack::unpack(reinterpret_cast<const char*>(buffer), bufferLen);

                try {
                    obj.get().convert(dto);
                    processPacket(dto);
                }
                catch(const msgpack::type_error&e) {
                    qDebug() << e.what();
                }

                nng_free(buffer, bufferLen);
            }
        }
    });

    for(const QString &machine : qAsConst(AppConfig::getInstance()->VisualMachines)) {

        nng_socket _visualSocket;

        int rv;
        if((rv = nng_pair1_open(&_visualSocket)) != 0) {
            continue;
        }

        QString url = QString("tcp://%1:%2").arg(machine).arg(AppConfig::getInstance()->XplanePluginPort);
        if((rv = nng_dial(_visualSocket, url.toStdString().c_str(), NULL, NNG_FLAG_NONBLOCK)) != 0) {
            continue;
        }

        if(rv == 0) {
            m_visualSockets.push_back(_visualSocket);
        }
    }

    connect(&m_heartbeatTimer, &QTimer::timeout, this, [&] {
        qint64 now = QDateTime::currentSecsSinceEpoch();

        if(!m_initialHandshake || (now - m_lastUdpTimestamp) > HEARTBEAT_TIMEOUT_SECS) {
            m_radioStackState = {};
            m_userAircraftData = {};
            m_userAircraftConfigData = {};

            SubscribeDataRefs();

            if(!m_initialHandshake)
            {
                // request plugin version
                requestPluginVersion();

                // validate csl
                validateCsl();
            }

            if(m_simConnected) {
                clearSimConnection();
            }
        } else {
            if(!m_simConnected && m_validPluginVersion && m_validCsl) {
                emit simConnectionStateChanged(true);
                m_simConnected = true;
            }
        }
    });
    m_heartbeatTimer.start(1000);

    connect(&m_xplaneDataTimer, &QTimer::timeout, this, [&]{
        emit userAircraftDataChanged(m_userAircraftData);
        emit userAircraftConfigDataChanged(m_userAircraftConfigData);
        emit radioStackStateChanged(m_radioStackState);
    });
    m_xplaneDataTimer.start(50);
}

XplaneAdapter::~XplaneAdapter()
{
    for(auto &visualSocket : m_visualSockets) {
        nng_close(visualSocket);
    }
    m_visualSockets.clear();

    m_keepSocketAlive = false;
    nng_close(m_socket);
    m_socket = NNG_SOCKET_INITIALIZER;

    if(m_socketThread) {
        m_socketThread->join();
        m_socketThread.reset();
    }
}

void XplaneAdapter::processPacket(const BaseDto &packet)
{
    if(packet.type == dto::PLUGIN_VER) {
        PluginVersionDto dto{};
        packet.dto.convert(dto);

        if(dto.version < BuildConfig::getVersionInt())
        {
            m_validPluginVersion = false;
            emit invalidPluginVersion();
        }
        m_initialHandshake = true;
    }
    if(packet.type == dto::VALIDATE_CSL) {
        ValidateCslDto dto{};
        packet.dto.convert(dto);

        if(!dto.isValid) {
            m_validCsl = false;
            if(!m_cslValidated) {
                emit invalidCslConfiguration(); // only show invalid CSL warning once
                m_cslValidated = true;
            }
        }
        m_initialHandshake = true;
    }
    if(packet.type == dto::AIRCRAFT_ADDED) {
        AircraftAddedDto dto{};
        packet.dto.convert(dto);
        if(!dto.callsign.empty()) {
            emit aircraftAddedToSim(dto.callsign.c_str());
        }
    }
    if(packet.type == dto::AIRCRAFT_DELETED) {
        AircraftAddedDto dto{};
        packet.dto.convert(dto);
        if(!dto.callsign.empty()) {
            emit aircraftRemovedFromSim(dto.callsign.c_str());
        }
    }
    if(packet.type == dto::REQUEST_STATION_INFO) {
        RequestStationInfoDto dto{};
        packet.dto.convert(dto);
        if(!dto.station.empty()) {
            emit requestStationInfo(dto.station.c_str());
        }
    }
    if(packet.type == dto::REQUEST_METAR) {
        RequestMetarDto dto;
        packet.dto.convert(dto);
        if(!dto.station.empty()) {
            emit requestMetar(dto.station.c_str());
        }
    }
    if(packet.type == dto::RADIO_MESSAGE_SENT) {
        RadioMessageSentDto dto;
        packet.dto.convert(dto);
        if(!dto.message.empty()) {
            emit radioMessageSent(dto.message.c_str());
        }
    }
    if(packet.type == dto::PRIVATE_MESSAGE_SENT) {
        PrivateMessageSentDto dto;
        packet.dto.convert(dto);
        if(!dto.to.empty() && !dto.message.empty()) {
            emit privateMessageSent(dto.to.c_str(), dto.message.c_str());
        }
    }
    if(packet.type == dto::WALLOP_SENT) {
        WallopSentDto dto;
        packet.dto.convert(dto);
        if(!dto.message.empty()) {
            emit sendWallop(dto.message.c_str());
        }
    }
    if(packet.type == dto::FORCE_DISCONNECT) {
        ForcedDisconnectDto dto;
        packet.dto.convert(dto);
        emit forceDisconnect(dto.reason.c_str());
    }
    if(packet.type == dto::SHUTDOWN) {
        clearSimConnection();
    }
}

void XplaneAdapter::clearSimConnection()
{
    emit simConnectionStateChanged(false);
    m_initialHandshake = false;
    m_simConnected = false;
    m_subscribedDataRefs.clear();
}

void XplaneAdapter::SubscribeDataRefs()
{
    SubscribeDataRef("sim/cockpit2/switches/avionics_power_on", DataRef::AvionicsPower, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_com_selection", DataRef::AudioComSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com1", DataRef::Com1AudioSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com2", DataRef::Com2AudioSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/com1_frequency_hz_833", DataRef::Com1Frequency, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/com2_frequency_hz_833", DataRef::Com2Frequency, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_volume_com1", DataRef::Com1Volume, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_volume_com2", DataRef::Com2Volume, 5);
    SubscribeDataRef("sim/cockpit/radios/transponder_mode", DataRef::TransponderMode, 5);
    SubscribeDataRef("sim/cockpit/radios/transponder_id", DataRef::TransponderIdent, 5);
    SubscribeDataRef("sim/cockpit/radios/transponder_code", DataRef::TransponderCode, 5);
    SubscribeDataRef("sim/cockpit2/switches/beacon_on", DataRef::BeaconLights, 5);
    SubscribeDataRef("sim/cockpit2/switches/landing_lights_on", DataRef::LandingLights, 5);
    SubscribeDataRef("sim/cockpit2/switches/taxi_light_on", DataRef::TaxiLights, 5);
    SubscribeDataRef("sim/cockpit2/switches/navigation_lights_on", DataRef::NavLights, 5);
    SubscribeDataRef("sim/cockpit2/switches/strobe_lights_on", DataRef::StrobeLights, 5);
    SubscribeDataRef("sim/flightmodel/position/latitude", DataRef::Latitude, 5);
    SubscribeDataRef("sim/flightmodel/position/longitude", DataRef::Longitude, 5);
    SubscribeDataRef("sim/flightmodel/position/elevation", DataRef::AltitudeMsl, 5);
    SubscribeDataRef("sim/flightmodel/position/y_agl", DataRef::AltitudeAgl, 5);
    SubscribeDataRef("sim/flightmodel2/position/pressure_altitude", DataRef::AltitudePressure, 5);
    SubscribeDataRef("sim/weather/barometer_sealevel_inhg", DataRef::BarometerSeaLevel, 5);
    SubscribeDataRef("sim/flightmodel/position/theta", DataRef::Pitch, 5);
    SubscribeDataRef("sim/flightmodel/position/psi", DataRef::Heading, 5);
    SubscribeDataRef("sim/flightmodel/position/phi", DataRef::Bank, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vx", DataRef::LongitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vy", DataRef::AltitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vz", DataRef::LatitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Qrad", DataRef::PitchVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Rrad", DataRef::HeadingVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Prad", DataRef::BankVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/groundspeed", DataRef::GroundSpeed, 5);
    SubscribeDataRef("sim/aircraft/engine/acf_num_engines", DataRef::EngineCount, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_running[0]", DataRef::Engine1Running, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_running[1]", DataRef::Engine2Running, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_running[2]", DataRef::Engine3Running, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_running[3]", DataRef::Engine4Running, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_propmode[0]", DataRef::Engine1Reversing, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_propmode[1]", DataRef::Engine2Reversing, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_propmode[2]", DataRef::Engine3Reversing, 5);
    SubscribeDataRef("sim/flightmodel/engine/ENGN_propmode[3]", DataRef::Engine4Reversing, 5);
    SubscribeDataRef("sim/flightmodel/failures/onground_any", DataRef::OnGround, 5);
    SubscribeDataRef("sim/cockpit/switches/gear_handle_status", DataRef::GearDown, 5);
    SubscribeDataRef("sim/flightmodel/controls/flaprat", DataRef::FlapRatio, 5);
    SubscribeDataRef("sim/cockpit2/controls/speedbrake_ratio", DataRef::SpeedbrakeRatio, 5);
    SubscribeDataRef("sim/flightmodel2/gear/tire_steer_actual_deg[0]", DataRef::NoseWheelAngle, 15);
    SubscribeDataRef("sim/operation/prefs/replay_mode", DataRef::ReplayMode, 5);
    SubscribeDataRef("sim/time/paused", DataRef::Paused, 5);
    SubscribeDataRef("xpilot/ptt", DataRef::PushToTalk, 15);
    SubscribeDataRef("xpilot/selcal_mute_override", DataRef::SelcalMuteOverride, 5);
    SubscribeDataRef("sim/version/xplane_internal_version", DataRef::XplaneVersionNumber, 1);
}

void XplaneAdapter::SubscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency)
{
    if(m_subscribedDataRefs.contains(dataRef.c_str())) {
        return; // already subscribed, skip
    }

    QByteArray data;

    data.fill(0, 413);
    data.insert(0, "RREF");
    data.insert(5, (const char*)&frequency);
    data.insert(9, (const char*)&id);
    data.insert(13, dataRef.c_str());
    data.resize(413);

    socket->writeDatagram(data.data(), data.size(), m_hostAddress, AppConfig::getInstance()->XplaneUdpPort);

    if(m_simConnected) {
        m_subscribedDataRefs.push_back(dataRef.c_str());
    }
}

void XplaneAdapter::setDataRefValue(std::string dataRef, float value)
{
    QByteArray data;

    data.fill(0, 509);
    data.insert(0, "DREF");
    data.insert(5, QByteArray::fromRawData(reinterpret_cast<char*>(&value), sizeof(float)));
    data.insert(9, dataRef.c_str());
    data.resize(509);

    socket->writeDatagram(data.data(), data.size(), m_hostAddress, AppConfig::getInstance()->XplaneUdpPort);
}

void XplaneAdapter::sendCommand(std::string command)
{
    QByteArray data;

    data.fill(0, command.length() + 6);
    data.insert(0, "CMND");
    data.insert(5, command.c_str());
    data.resize(command.length() + 6);

    socket->writeDatagram(data.data(), data.size(), m_hostAddress, AppConfig::getInstance()->XplaneUdpPort);
}

void XplaneAdapter::setTransponderCode(int code)
{
    setDataRefValue("sim/cockpit2/radios/actuators/transponder_code", code);
}

void XplaneAdapter::setCom1Frequency(float freq)
{
    setDataRefValue("sim/cockpit2/radios/actuators/com1_frequency_hz_833", freq);
}

void XplaneAdapter::setCom2Frequency(float freq)
{
    setDataRefValue("sim/cockpit2/radios/actuators/com2_frequency_hz_833", freq);
}

void XplaneAdapter::transponderIdent()
{
    sendCommand("sim/transponder/transponder_ident");
}

void XplaneAdapter::transponderModeToggle()
{
    if(m_radioStackState.SquawkingModeC) {
        setDataRefValue("sim/cockpit/radios/transponder_mode", 0);
        sendCommand("laminar/B738/knob/transponder_stby");
    } else {
        setDataRefValue("sim/cockpit/radios/transponder_mode", 2);
        sendCommand("laminar/B738/knob/transponder_alton");
    }
}

void XplaneAdapter::OnDataReceived()
{
    QByteArray buffer;
    QHostAddress fromAddress;

    buffer.resize(socket->pendingDatagramSize());
    socket->readDatagram(buffer.data(), buffer.size(), &fromAddress);

    if(fromAddress.toIPv4Address() != m_hostAddress.toIPv4Address())
        return;

    if(strncmp(buffer.constData(), "RREF", 4) == 0)
    {
        m_lastUdpTimestamp = QDateTime::currentSecsSinceEpoch();

        int num_structs = (buffer.size() - 5) / sizeof(rref_data_type);
        const rref_data_type *f = reinterpret_cast<const rref_data_type*>(buffer.constData() + 5);

        for(int i = 0; i < num_structs; i++)
        {
            float value = f[i].val;

            switch(f[i].idx)
            {
                case DataRef::AvionicsPower:
                    m_radioStackState.AvionicsPowerOn = value;
                    break;
                case DataRef::AudioComSelection:
                    if(value == 6) {
                        m_radioStackState.Com1TransmitEnabled = true;
                        m_radioStackState.Com2TransmitEnabled = false;
                    } else if(value == 7) {
                        m_radioStackState.Com1TransmitEnabled = false;
                        m_radioStackState.Com2TransmitEnabled = true;
                    }
                    break;
                case DataRef::Com1AudioSelection:
                    m_radioStackState.Com1ReceiveEnabled = value;
                    break;
                case DataRef::Com2AudioSelection:
                    m_radioStackState.Com2ReceiveEnabled = value;
                    break;
                case DataRef::Com1Frequency:
                    m_radioStackState.Com1Frequency = value;
                    break;
                case DataRef::Com2Frequency:
                    m_radioStackState.Com2Frequency = value;
                    break;
                case DataRef::Com1Volume:
                    {
                        int volume = qRound(value * 100);
                        volume = (volume > 100) ? 100 : volume;
                        volume = (volume < 0) ? 0 : volume;
                        m_radioStackState.Com1Volume = volume;
                    }
                    break;
                case DataRef::Com2Volume:
                    {
                        int volume = qRound(value * 100);
                        volume = (volume > 100) ? 100 : volume;
                        volume = (volume < 0) ? 0 : volume;
                        m_radioStackState.Com2Volume = volume;
                    }
                    break;
                case DataRef::TransponderIdent:
                    m_radioStackState.SquawkingIdent = value;
                    break;
                case DataRef::TransponderMode:
                    m_radioStackState.SquawkingModeC = value >= 2;
                    break;
                case DataRef::TransponderCode:
                    m_radioStackState.TransponderCode = value;
                    break;
                case DataRef::Latitude:
                    m_userAircraftData.Latitude = value;
                    break;
                case DataRef::Longitude:
                    m_userAircraftData.Longitude = value;
                    break;
                case DataRef::Heading:
                    m_userAircraftData.Heading = value;
                    break;
                case DataRef::Pitch:
                    m_userAircraftData.Pitch = value;
                    break;
                case DataRef::Bank:
                    m_userAircraftData.Bank = value;
                    break;
                case DataRef::AltitudeMsl:
                    m_userAircraftData.AltitudeMslM = value;
                    break;
                case DataRef::AltitudeAgl:
                    m_userAircraftData.AltitudeAglM = value;
                    break;
                case DataRef::AltitudePressure:
                    m_userAircraftData.AltitudePressure = value;
                    break;
                case DataRef::BarometerSeaLevel:
                    m_userAircraftData.BarometerSeaLevel = value * 33.8639; // inHg to millibar
                    break;
                case DataRef::LatitudeVelocity:
                    m_userAircraftData.LatitudeVelocity = value * -1.0;
                    break;
                case DataRef::AltitudeVelocity:
                    m_userAircraftData.AltitudeVelocity = value;
                    break;
                case DataRef::LongitudeVelocity:
                    m_userAircraftData.LongitudeVelocity = value;
                    break;
                case DataRef::PitchVelocity:
                    m_userAircraftData.PitchVelocity = value * -1.0;
                    break;
                case DataRef::HeadingVelocity:
                    m_userAircraftData.HeadingVelocity = value;
                    break;
                case DataRef::BankVelocity:
                    m_userAircraftData.BankVelocity = value * -1.0;
                    break;
                case DataRef::GroundSpeed:
                    m_userAircraftData.GroundSpeed = value * 1.94384; // mps -> knots
                    break;
                case DataRef::BeaconLights:
                    m_userAircraftConfigData.BeaconOn = value;
                    break;
                case DataRef::LandingLights:
                    m_userAircraftConfigData.LandingLightsOn = value;
                    break;
                case DataRef::TaxiLights:
                    m_userAircraftConfigData.TaxiLightsOn = value;
                    break;
                case DataRef::NavLights:
                    m_userAircraftConfigData.NavLightsOn = value;
                    break;
                case DataRef::StrobeLights:
                    m_userAircraftConfigData.StrobesOn = value;
                    break;
                case DataRef::EngineCount:
                    m_userAircraftConfigData.EngineCount = value;
                    break;
                case DataRef::Engine1Running:
                    m_userAircraftConfigData.Engine1Running = value;
                    break;
                case DataRef::Engine2Running:
                    m_userAircraftConfigData.Engine2Running = value;
                    break;
                case DataRef::Engine3Running:
                    m_userAircraftConfigData.Engine3Running = value;
                    break;
                case DataRef::Engine4Running:
                    m_userAircraftConfigData.Engine4Running = value;
                    break;
                case DataRef::Engine1Reversing:
                    m_userAircraftConfigData.Engine1Reversing = (value == 3);
                    break;
                case DataRef::Engine2Reversing:
                    m_userAircraftConfigData.Engine2Reversing = (value == 3);
                    break;
                case DataRef::Engine3Reversing:
                    m_userAircraftConfigData.Engine3Reversing = (value == 3);
                    break;
                case DataRef::Engine4Reversing:
                    m_userAircraftConfigData.Engine4Reversing = (value == 3);
                    break;
                case DataRef::OnGround:
                    m_userAircraftConfigData.OnGround = value;
                    break;
                case DataRef::GearDown:
                    m_userAircraftConfigData.GearDown = value;
                    break;
                case DataRef::FlapRatio:
                    m_userAircraftConfigData.FlapsRatio = value;
                    break;
                case DataRef::SpeedbrakeRatio:
                    m_userAircraftConfigData.SpeedbrakeRatio = value;
                    break;
                case DataRef::NoseWheelAngle:
                    m_userAircraftData.NoseWheelAngle = value;
                    break;
                case DataRef::ReplayMode:
                    if(value > 0) {
                        emit replayModeDetected();
                    }
                    break;
                case DataRef::Paused:
                    if((bool)value != m_simPaused) {
                        emit simPausedStateChanged(value > 0);
                        m_simPaused = (bool)value;
                    }
                    break;
                case DataRef::PushToTalk:
                    (value > 0) ? emit pttPressed() : emit pttReleased();
                    break;
                case DataRef::SelcalMuteOverride:
                    m_radioStackState.SelcalMuteOverride = value > 0;
                    break;
                case DataRef::XplaneVersionNumber:
                    SKIP_EMPTY(value);
                    m_xplaneVersion = value;
                    break;
            }
        }
    }
}

void XplaneAdapter::requestPluginVersion()
{
    PluginVersionDto dto{};
    SendDto(dto);
}

void XplaneAdapter::validateCsl()
{
    ValidateCslDto dto{};
    SendDto(dto);
}

void XplaneAdapter::setAudioComSelection(int radio)
{
    switch(radio)
    {
        case 1:
            setDataRefValue("sim/cockpit2/radios/actuators/audio_com_selection", 6);
            break;
        case 2:
            setDataRefValue("sim/cockpit2/radios/actuators/audio_com_selection", 7);
            break;
    }
}

void XplaneAdapter::setAudioSelection(int radio, bool status)
{
    switch(radio)
    {
        case 1:
            setDataRefValue("sim/cockpit2/radios/actuators/audio_selection_com1", (int)status);
            break;
        case 2:
            setDataRefValue("sim/cockpit2/radios/actuators/audio_selection_com2", (int)status);
            break;
    }
}

void XplaneAdapter::setComRxDataref(int radio, bool active)
{
    switch(radio)
    {
        case 0:
            setDataRefValue("xpilot/audio/com1_rx", (int)active);
            break;
        case 1:
            setDataRefValue("xpilot/audio/com2_rx", (int)active);
            break;
    }
}

void XplaneAdapter::setVuDataref(float vu)
{
    setDataRefValue("xpilot/audio/vu", vu);
}

void XplaneAdapter::ignoreAircraft(QString callsign)
{
    if(!m_ignoreList.contains(callsign.toUpper())) {
        m_ignoreList.push_back(callsign.toUpper());
        emit aircraftIgnored(callsign.toUpper());
        return;
    }

    emit aircraftAlreadyIgnored(callsign.toUpper()); // aircraft already ignored
}

void XplaneAdapter::unignoreAircraft(QString callsign)
{
    if(m_ignoreList.contains(callsign.toUpper())) {
        m_ignoreList.removeAll(callsign.toUpper());
        emit aircraftUnignored(callsign.toUpper());
        return;
    }

    emit aircraftNotIgnored(callsign.toUpper()); // aircraft not previously ignored
}

void XplaneAdapter::showIgnoreList()
{
    emit ignoreList(m_ignoreList);
}

void XplaneAdapter::selcalAlertReceived()
{
    setDataRefValue("xpilot/selcal_received", 1);
    QTimer::singleShot(1000, this, [&]{
        setDataRefValue("xpilot/selcal_received", 0);
    });
}

void XplaneAdapter::AddAircraftToSimulator(const NetworkAircraft &aircraft)
{
    AddAircraftDto dto{};
    dto.callsign = aircraft.Callsign.toStdString();
    dto.airline = aircraft.Airline.toStdString();
    dto.typeCode = aircraft.TypeCode.toStdString();
    dto.latitude = aircraft.RemoteVisualState.Latitude;
    dto.longitude = aircraft.RemoteVisualState.Longitude;
    dto.altitudeTrue = aircraft.RemoteVisualState.Altitude;
    dto.heading = aircraft.RemoteVisualState.Heading;
    dto.bank = aircraft.RemoteVisualState.Bank;
    dto.pitch = aircraft.RemoteVisualState.Pitch;
    SendDto(dto);
}

void XplaneAdapter::PlaneConfigChanged(const NetworkAircraft &aircraft)
{
    AircraftConfigDto dto{};
    dto.callsign = aircraft.Callsign.toStdString();

    if(!aircraft.Configuration->IsIncremental())
    {
        dto.fullConfig = true;
    }

    if(aircraft.Configuration->Engines.has_value())
    {
        if(aircraft.Configuration->HasEnginesRunning())
        {
            dto.enginesOn = aircraft.Configuration->IsAnyEngineRunning();
        }
        if(aircraft.Configuration->HasEnginesReversing())
        {
            dto.enginesReversing = aircraft.Configuration->IsAnyEngineReversing();
        }
    }

    if(aircraft.Configuration->OnGround.has_value())
    {
        dto.onGround = aircraft.Configuration->OnGround.value();
    }

    if(aircraft.Configuration->FlapsPercent.has_value())
    {
        dto.flaps = std::round((aircraft.Configuration->FlapsPercent.value() / 100.0f) * 100.0) / 100.0;
    }

    if(aircraft.Configuration->SpoilersDeployed.has_value())
    {
        dto.spoilersDeployed = aircraft.Configuration->SpoilersDeployed.value();
    }

    if(aircraft.Configuration->GearDown.has_value())
    {
        dto.gearDown = aircraft.Configuration->GearDown.value();
    }

    if(aircraft.Configuration->Lights->HasLights())
    {
        if(aircraft.Configuration->Lights->BeaconOn.has_value())
        {
            dto.beaconLightsOn = aircraft.Configuration->Lights->BeaconOn.value();
        }
        if(aircraft.Configuration->Lights->LandingOn.has_value())
        {
            dto.landingLightsOn = aircraft.Configuration->Lights->LandingOn.value();
        }
        if(aircraft.Configuration->Lights->NavOn.has_value())
        {
            dto.navLightsOn = aircraft.Configuration->Lights->NavOn.value();
        }
        if(aircraft.Configuration->Lights->StrobeOn.has_value())
        {
            dto.strobeLightsOn = aircraft.Configuration->Lights->StrobeOn.value();
        }
        if(aircraft.Configuration->Lights->TaxiOn.has_value())
        {
            dto.taxiLightsOn = aircraft.Configuration->Lights->TaxiOn.value();
        }
    }

    SendDto(dto);
}

void XplaneAdapter::DeleteAircraft(const NetworkAircraft &aircraft, QString reason)
{
    DeleteAircraftDto dto{};
    dto.callsign = aircraft.Callsign.toStdString();
    dto.reason = reason.toStdString();
    SendDto(dto);
}

void XplaneAdapter::DeleteAllAircraft()
{
    DeleteAllAircraftDto dto{};
    SendDto(dto);
}

void XplaneAdapter::UpdateControllers(QList<Controller> &controllers)
{
    NearbyAtcDto dto{};
    for(auto &controller : controllers)
    {
        if(controller.IsValid)
        {
            NearbyAtcStationDto station;
            station.callsign = controller.Callsign.toStdString();
            station.frequency = QString::number(controller.Frequency / 1000.0, 'f', 3).toStdString();
            station.xplaneFrequency = controller.Frequency;
            station.name = controller.RealName.toStdString();
            dto.stations.push_back(station);
        }
    }
    SendDto(dto);
}

void XplaneAdapter::DeleteAllControllers()
{
    NearbyAtcDto dto{};
    SendDto(dto);
}

void XplaneAdapter::SendFastPositionUpdate(const NetworkAircraft &aircraft, const AircraftVisualState &visualState, const VelocityVector &positionalVelocityVector, const VelocityVector &rotationalVelocityVector)
{
    FastPositionUpdateDto dto{};
    dto.callsign = aircraft.Callsign.toStdString();
    dto.latitude = visualState.Latitude;
    dto.longitude = visualState.Longitude;
    dto.altitudeTrue = visualState.Altitude;
    dto.altitudeAgl = visualState.AltitudeAgl;
    dto.heading = visualState.Heading;
    dto.bank = visualState.Bank;
    dto.pitch = visualState.Pitch;
    dto.vx = positionalVelocityVector.X;
    dto.vy = positionalVelocityVector.Y;
    dto.vz = positionalVelocityVector.Z;
    dto.vp = rotationalVelocityVector.X;
    dto.vh = rotationalVelocityVector.Y;
    dto.vb = rotationalVelocityVector.Z;
    dto.noseWheelAngle = visualState.NoseWheelAngle;
    dto.speed = aircraft.Speed;

    SendDto(dto);
}

void XplaneAdapter::SendHeartbeat(const QString callsign)
{
    HeartbeatDto dto{};
    dto.callsign = callsign.toStdString();

    SendDto(dto);
}

void XplaneAdapter::SendRadioMessage(const QString message)
{
    RadioMessageSentDto dto{};
    dto.message = message.toStdString();

    SendDto(dto);
}

void XplaneAdapter::RadioMessageReceived(const QString from, const QString message, bool isDirect)
{
    RadioMessageReceivedDto dto{};
    dto.from = from.toStdString();
    dto.message = message.toStdString();
    dto.isDirect = isDirect;

    SendDto(dto);
}

void XplaneAdapter::NotificationPosted(const QString message, qint64 color)
{
    NotificationPostedDto dto{};
    dto.message = message.toStdString();
    dto.color = color;

    SendDto(dto);
}

void XplaneAdapter::SendPrivateMessage(const QString to, const QString message)
{
    PrivateMessageSentDto dto{};
    dto.to = to.toStdString();
    dto.message = message.toStdString();

    SendDto(dto);
}

void XplaneAdapter::PrivateMessageReceived(const QString from, const QString message)
{
    PrivateMessageReceivedDto dto{};
    dto.from = from.toStdString();
    dto.message = message.toStdString();

    SendDto(dto);
}

void XplaneAdapter::NetworkConnected(QString callsign, QString selcal)
{
    ConnectedDto dto;
    dto.callsign = callsign.toStdString();
    dto.selcal = selcal.toStdString();

    SendDto(dto);
}

void XplaneAdapter::NetworkDisconnected()
{
    DisconnectedDto dto;
    SendDto(dto);
}

void XplaneAdapter::SetStationCallsign(int com, QString callsign)
{
    ComStationCallsign dto{};
    dto.callsign = callsign.toStdString();
    dto.com = com;
    SendDto(dto);
}

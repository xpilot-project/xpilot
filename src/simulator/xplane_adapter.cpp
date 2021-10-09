#include "xplane_adapter.h"

#include <QTimer>
#include <QtEndian>
#include <QDateTime>

enum DataRef
{
    AvionicsPower,
    AudioComSelection,
    Com1AudioSelection,
    Com2AudioSelection,
    Com1Frequency,
    Com2Frequency,
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
    OnGround,
    GearDown,
    FlapRatio,
    SpeedbrakeRatio,
    ReplayMode,
    PushToTalk
};

XplaneAdapter::XplaneAdapter(QObject* parent) : QObject(parent)
{
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::readyRead, this, &XplaneAdapter::OnDataReceived);

    QTimer *heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, [=] {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        if((now - m_lastUdpTimestamp) > 5) {
            emit simConnectionStateChanged(false);
            m_simConnected = false;
            m_radioStackState = {};
            m_userAircraftData = {};
            m_userAircraftConfigData = {};
            Subscribe();
        } else {
            if(!m_simConnected) {
                emit simConnectionStateChanged(true);
                m_simConnected = true;
            }
        }
    });
    heartbeatTimer->start(1000);

    QTimer *xplaneDataTimer = new QTimer(this);
    connect(xplaneDataTimer, &QTimer::timeout, this, [=]{
        emit userAircraftDataChanged(m_userAircraftData);
        emit userAircraftConfigDataChanged(m_userAircraftConfigData);
        emit radioStackStateChanged(m_radioStackState);
    });
    xplaneDataTimer->start(50);

    Subscribe();
}

void XplaneAdapter::Subscribe()
{
    SubscribeDataRef("sim/cockpit2/switches/avionics_power_on", DataRef::AvionicsPower, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_com_selection", DataRef::AudioComSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com1", DataRef::Com1AudioSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com2", DataRef::Com2AudioSelection, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/com1_frequency_hz_833", DataRef::Com1Frequency, 5);
    SubscribeDataRef("sim/cockpit2/radios/actuators/com2_frequency_hz_833", DataRef::Com2Frequency, 5);
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
    SubscribeDataRef("sim/flightmodel/position/theta", DataRef::Pitch, 5);
    SubscribeDataRef("sim/flightmodel/position/psi", DataRef::Heading, 5);
    SubscribeDataRef("sim/flightmodel/position/phi", DataRef::Bank, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vx", DataRef::LatitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vy", DataRef::AltitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/local_vz", DataRef::LongitudeVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Qrad", DataRef::PitchVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Rrad", DataRef::HeadingVelocity, 5);
    SubscribeDataRef("sim/flightmodel/position/Prad", DataRef::BankVelocity, 5);
    SubscribeDataRef("sim/aircraft/engine/acf_num_engines", DataRef::EngineCount, 5);
    SubscribeDataRef("sim/flightmodel/failures/onground_any", DataRef::OnGround, 5);
    SubscribeDataRef("sim/cockpit/switches/gear_handle_status", DataRef::GearDown, 5);
    SubscribeDataRef("sim/flightmodel/controls/flaprat", DataRef::FlapRatio, 5);
    SubscribeDataRef("sim/cockpit2/controls/speedbrake_ratio", DataRef::SpeedbrakeRatio, 5);
    SubscribeDataRef("sim/operation/prefs/replay_mode", DataRef::ReplayMode, 5);
    SubscribeDataRef("xpilot/ptt", DataRef::PushToTalk, 5);
}

void XplaneAdapter::SubscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency)
{
    QByteArray data;

    data.fill(0, 413);
    data.insert(0, "RREF");
    data.insert(5, (const char*)&frequency);
    data.insert(9, (const char*)&id);
    data.insert(13, dataRef.c_str());
    data.resize(413);

    socket->writeDatagram(data.data(), data.size(), QHostAddress::LocalHost, 49000);
}

void XplaneAdapter::setDataRefValue(std::string dataRef, float value)
{
    QByteArray data;

    data.fill(0, 509);
    data.insert(0, "DREF");
    data.insert(5, QByteArray::fromRawData(reinterpret_cast<char*>(&value), sizeof(float)));
    data.insert(9, dataRef.c_str());
    data.resize(509);

    socket->writeDatagram(data.data(), data.size(), QHostAddress::LocalHost, 49000);
}

void XplaneAdapter::sendCommand(std::string command)
{
    QByteArray data;

    data.fill(0, command.length() + 6);
    data.insert(0, "CMND");
    data.insert(5, command.c_str());
    data.resize(command.length() + 6);

    socket->writeDatagram(data.data(), data.size(), QHostAddress::LocalHost, 49000);
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
    } else {
        setDataRefValue("sim/cockpit/radios/transponder_mode", 2);
    }
}

void XplaneAdapter::OnDataReceived()
{
    while(socket->hasPendingDatagrams())
    {
        QByteArray buffer;

        buffer.resize(socket->pendingDatagramSize());
        socket->readDatagram(buffer.data(), buffer.size());

        int pos = 0;
        QString header = QString::fromUtf8(buffer.mid(pos, 4));
        pos += 5;

        if(header == "RREF")
        {
            m_lastUdpTimestamp = QDateTime::currentSecsSinceEpoch();

            while(pos < buffer.length())
            {
                int id = *(reinterpret_cast<const int*>(buffer.mid(pos, 4).constData()));
                pos += 4;

                float value = *(reinterpret_cast<const float*>(buffer.mid(pos, 4).constData()));
                pos += 4;

                switch(id) {
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
                case DataRef::AltitudeMsl:
                    m_userAircraftData.AltitudeMslM = value;
                    break;
                case DataRef::AltitudeAgl:
                    m_userAircraftData.AltitudeAglM = value;
                    break;
                case DataRef::LatitudeVelocity:
                    m_userAircraftData.LatitudeVelocity = value;
                    break;
                case DataRef::AltitudeVelocity:
                    m_userAircraftData.AltitudeVelocity = value;
                    break;
                case DataRef::LongitudeVelocity:
                    m_userAircraftData.LongitudeVelocity = value;
                    break;
                case DataRef::PitchVelocity:
                    m_userAircraftData.PitchVelocity = value;
                    break;
                case DataRef::HeadingVelocity:
                    m_userAircraftData.HeadingVelocity = value;
                    break;
                case DataRef::BankVelocity:
                    m_userAircraftData.BankVelocity = value;
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
                case DataRef::ReplayMode:
                    if(value > 0) {
                        emit replayModeDetected();
                    }
                    break;
                }
            }
        }
    }
}

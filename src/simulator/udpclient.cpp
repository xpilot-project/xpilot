#include "udpclient.h"

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
    Latitude,
    Longitude,
    Elevation,
    Heading,
    Pitch,
    Bank,
    PushToTalk
};

UdpClient::UdpClient(QObject* parent) : QObject(parent)
{
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::readyRead, this, &UdpClient::OnDataReceived);

    QTimer *heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, [=] {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        if((now - m_lastUdpTimestamp) > 15) {
            emit simConnectionStateChanged(false);
            m_simConnected = false;
        } else {
            if(!m_simConnected) {
                emit simConnectionStateChanged(true);
                m_simConnected = true;
            }
        }
    });
    heartbeatTimer->start(1000);

    subscribeDataRef("sim/cockpit2/switches/avionics_power_on", DataRef::AvionicsPower, 5);
    subscribeDataRef("sim/cockpit2/radios/actuators/audio_com_selection", DataRef::AudioComSelection, 5);
    subscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com1", DataRef::Com1AudioSelection, 5);
    subscribeDataRef("sim/cockpit2/radios/actuators/audio_selection_com2", DataRef::Com2AudioSelection, 5);
    subscribeDataRef("sim/cockpit2/radios/actuators/com1_frequency_hz_833", DataRef::Com1Frequency, 5);
    subscribeDataRef("sim/cockpit2/radios/actuators/com2_frequency_hz_833", DataRef::Com2Frequency, 5);
    subscribeDataRef("sim/flightmodel/position/latitude", DataRef::Latitude, 5);
    subscribeDataRef("sim/flightmodel/position/longitude", DataRef::Longitude, 5);
    subscribeDataRef("sim/flightmodel/position/elevation", DataRef::Elevation, 5);
    subscribeDataRef("sim/flightmodel/position/psi", DataRef::Heading, 5);
    subscribeDataRef("sim/flightmodel/position/theta", DataRef::Pitch, 5);
    subscribeDataRef("sim/flightmodel/position/phi", DataRef::Bank, 5);
    subscribeDataRef("xpilot/ptt", DataRef::PushToTalk, 5);
}

void UdpClient::subscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency)
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

void UdpClient::setDataRefValue(std::string dataRef, float value)
{
    QByteArray data;

    data.fill(0, 509);
    data.insert(0, "DREF");
    data.insert(5, QByteArray::fromRawData(reinterpret_cast<char*>(&value), sizeof(float)));
    data.insert(9, dataRef.c_str());
    data.resize(509);

    socket->writeDatagram(data.data(), data.size(), QHostAddress::LocalHost, 49000);
}

void UdpClient::setTransponderCode(int code)
{
    setDataRefValue("sim/cockpit2/radios/actuators/transponder_code", code);
}

void UdpClient::setCom1Frequency(float freq)
{
    setDataRefValue("sim/cockpit2/radios/actuators/com1_frequency_hz_833", freq);
}

void UdpClient::setCom2Frequency(float freq)
{
    setDataRefValue("sim/cockpit2/radios/actuators/com2_frequency_hz_833", freq);
}

void UdpClient::OnDataReceived()
{
    while(socket->hasPendingDatagrams())
    {
        QByteArray buffer;

        buffer.resize(socket->pendingDatagramSize());
        socket->readDatagram(buffer.data(), buffer.size());

        int pos = 0;
        QString header = QString::fromUtf8(buffer.mid(pos, 4));
        pos += 5;

        m_lastUdpTimestamp = QDateTime::currentSecsSinceEpoch();

        if(header == "RREF")
        {
            while(pos < buffer.length())
            {
                qint32 id = qFromLittleEndian<qint32>(buffer.mid(pos, 4).data());
                pos += 4;

                float value = qFromLittleEndian<float>(buffer.mid(pos, 4).data());
                pos += 4;

                switch(id) {
                case DataRef::AvionicsPower:
                    if(m_avionicsPower != value) {
                        emit avionicsPowerOnChanged(value);
                        m_avionicsPower = value;
                    }
                    break;
                case DataRef::AudioComSelection:
                    if(m_audioComSelection != value) {
                        emit audioComSelectionChanged(value);
                        m_audioComSelection = value;
                    }
                    break;
                case DataRef::Com1AudioSelection:
                    if(m_com1AudioSelection != value) {
                        emit com1AudioSelectionChanged(value);
                        m_com1AudioSelection = value;
                    }
                    break;
                case DataRef::Com2AudioSelection:
                    if(m_com2AudioSelection != value) {
                        emit com2AudioSelectionChanged(value);
                        m_com2AudioSelection = value;
                    }
                    break;
                case DataRef::Com1Frequency:
                    if(m_com1Frequency != value) {
                        emit com1FrequencyChanged(value * 1000);
                        m_com1Frequency = value * 1000;
                    }
                    break;
                case DataRef::Com2Frequency:
                    if(m_com1Frequency != value) {
                        emit com2FrequencyChanged(value * 1000);
                        m_com2Frequency = value * 1000;
                    }
                    break;
                case DataRef::Latitude:
                    break;
                }
            }
        }
    }
}

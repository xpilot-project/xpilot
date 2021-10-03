#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <map>
#include <string>
#include <cstdint>
#include <list>
#include <functional>
#include <string>
#include <mutex>
#include <vector>

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>

class UdpClient : public QObject
{
    Q_OBJECT

public:
    UdpClient(QObject* parent = nullptr);

    Q_INVOKABLE void setTransponderCode(int code);
    Q_INVOKABLE void setCom1Frequency(float freq);
    Q_INVOKABLE void setCom2Frequency(float freq);
    Q_INVOKABLE void transponderIdent();
    Q_INVOKABLE void transponderModeToggle();

private:
    void subscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency);
    void setDataRefValue(std::string dataRef, float value);
    void sendCommand(std::string command);

public slots:
    void OnDataReceived();

signals:
    void simConnectionStateChanged(bool connected);
    void avionicsPowerOnChanged(bool power);
    void audioComSelectionChanged(int radio);
    void com1AudioSelectionChanged(bool active);
    void com2AudioSelectionChanged(bool active);
    void com1FrequencyChanged(float freq);
    void com2FrequencyChanged(float freq);
    void transponderIdentChanged(bool active);
    void transponderModeChanged(int mode);

private:
    QUdpSocket* socket;
    qint64 m_lastUdpTimestamp;
    bool m_simConnected = false;

    bool m_avionicsPower;
    int m_audioComSelection;
    bool m_com1AudioSelection;
    bool m_com2AudioSelection;
    float m_com1Frequency;
    float m_com2Frequency;
    bool m_transponderIdent;
    int m_transponderMode;

    void resetValues()
    {
        // reset cached values so that if the connection is re-established, new values are forced
        m_avionicsPower = 0;
        m_audioComSelection = 0;
        m_com1AudioSelection = 0;
        m_com2AudioSelection = 0;
        m_com1Frequency = 0;
        m_com2Frequency = 0;
        m_transponderIdent = 0;
        m_transponderMode = 0;
    }
};

#endif

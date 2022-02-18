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
#include <thread>
#include <deque>

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>
#include <QHostAddress>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QTimer>

#include "zmq.hpp"

#include "src/config/appconfig.h"
#include "src/aircrafts/network_aircraft.h"
#include "src/aircrafts/velocity_vector.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/radio_stack_state.h"

#define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#include <boost/interprocess/ipc/message_queue.hpp>

namespace bip = boost::interprocess;

#define OUTBOUND_QUEUE "xpilot.outbound"
#define INBOUND_QUEUE "xpilot.inbound"
#define MAX_MESSAGES 500
#define MAX_MESSAGE_SIZE 2048

namespace boost {
    namespace interprocess {
        namespace ipcdetail {
            inline void get_shared_dir(std::string& shared_dir) {
                shared_dir = xpilot::AppConfig::getInstance()->dataRoot().toStdString();
            }
        }
    }
}

class XplaneAdapter : public QObject
{
    Q_OBJECT

public:
    XplaneAdapter(QObject* parent = nullptr);
    ~XplaneAdapter();

    Q_INVOKABLE void setTransponderCode(int code);
    Q_INVOKABLE void setCom1Frequency(float freq);
    Q_INVOKABLE void setCom2Frequency(float freq);
    Q_INVOKABLE void transponderIdent();
    Q_INVOKABLE void transponderModeToggle();
    Q_INVOKABLE void sendSocketMessage(const QString& message);
    Q_INVOKABLE void setAudioComSelection(int radio);
    Q_INVOKABLE void setAudioSelection(int radio, bool status);
    Q_INVOKABLE void setComRxDataref(int radio, bool active);
    Q_INVOKABLE void setVuDataref(float vu);
    Q_INVOKABLE void ignoreAircraft(QString callsign);
    Q_INVOKABLE void unignoreAircraft(QString callsign);
    Q_INVOKABLE void showIgnoreList();
    Q_INVOKABLE void selcalAlertReceived();

    void AddPlaneToSimulator(const NetworkAircraft& aircraft);
    void PlaneConfigChanged(const NetworkAircraft& aircraft);
    void PlaneModelChanged(const NetworkAircraft& aircraft);
    void DeleteAircraft(const NetworkAircraft& aircraft);
    void DeleteAllAircraft();
    void DeleteAllControllers();
    void SendSlowPositionUpdate(const NetworkAircraft& aircraft, const AircraftVisualState& visualState, const double& groundSpeed);
    void SendFastPositionUpdate(const NetworkAircraft& aircraft, const AircraftVisualState& visualState, const VelocityVector& positionalVelocityVector, const VelocityVector& rotationalVelocityVector);
    void SendRadioMessage(const QString message);
    void RadioMessageReceived(const QString from, const QString message, bool isDirect);
    void NotificationPosted(const QString message, qint64 color);
    void SendPrivateMessage(const QString to, const QString message);
    void PrivateMessageReceived(const QString from, const QString message);

private:
    void Subscribe();
    void SubscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency);
    void setDataRefValue(std::string dataRef, float value);
    void sendCommand(std::string command);

    void initializeMessageQueues();
    void clearSimConnection();

public slots:
    void OnDataReceived();

signals:
    void simConnectionStateChanged(bool connected);
    void userAircraftDataChanged(UserAircraftData data);
    void userAircraftConfigDataChanged(UserAircraftConfigData data);
    void radioStackStateChanged(RadioStackState radioStack);
    void replayModeDetected();
    void pttPressed();
    void pttReleased();
    void invalidPluginVersion();
    void invalidCslConfiguration();
    void requestStationInfo(QString callsign);
    void requestMetar(QString station);
    void radioMessageSent(QString message);
    void privateMessageSent(QString to, QString message);
    void forceDisconnect(QString reason);
    void aircraftIgnored(QString callsign);
    void aircraftAlreadyIgnored(QString callsign);
    void aircraftUnignored(QString callsign);
    void aircraftNotIgnored(QString callsign);
    void ignoreList(QStringList list);
    void sendWallop(QString message);
    void simPausedStateChanged(bool paused);

private:
    QUdpSocket* socket;
    qint64 m_lastUdpTimestamp;
    bool m_simConnected = false;
    bool m_initialHandshake = false;
    bool m_validPluginVersion = true;
    bool m_validCsl = true;
    bool m_simPaused = false;

    UserAircraftData m_userAircraftData{};
    UserAircraftConfigData m_userAircraftConfigData{};
    RadioStackState m_radioStackState{};

    QList<QString> m_ignoreList;

    std::unique_ptr<zmq::context_t> m_zmqContext;
    QList<zmq::socket_t*> m_visualSockets;
    QTimer m_heartbeatTimer;
    QTimer m_xplaneDataTimer;

    // message queue
    bool m_keepMessageQueueAlive = false;
    std::unique_ptr<std::thread> messageQueueThread;
    std::unique_ptr<bip::message_queue> outboundQueue;  // xpilot -> xplane
    std::unique_ptr<bip::message_queue> inboundQueue;   // xplane -> xpilot

    QFile m_pluginLog;
    QTextStream m_rawDataStream;
    QMutex mutex;
};

#endif

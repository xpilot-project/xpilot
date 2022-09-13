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

#include <msgpack.hpp>

#include <nng/nng.h>
#include <nng/protocol/pair1/pair.h>

#include "src/config/appconfig.h"
#include "src/aircrafts/network_aircraft.h"
#include "src/aircrafts/velocity_vector.h"
#include "src/aircrafts/user_aircraft_data.h"
#include "src/aircrafts/user_aircraft_config_data.h"
#include "src/aircrafts/radio_stack_state.h"
#include "src/controllers/controller.h"
#include "src/simulator/dto.h"

using namespace xpilot;

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
    Q_INVOKABLE void setAudioComSelection(int radio);
    Q_INVOKABLE void setAudioSelection(int radio, bool status);
    Q_INVOKABLE void setComRxDataref(int radio, bool active);
    Q_INVOKABLE void setVuDataref(float vu);
    Q_INVOKABLE void ignoreAircraft(QString callsign);
    Q_INVOKABLE void unignoreAircraft(QString callsign);
    Q_INVOKABLE void showIgnoreList();
    Q_INVOKABLE void selcalAlertReceived();

    void AddAircraftToSimulator(const NetworkAircraft& aircraft);
    void PlaneConfigChanged(const NetworkAircraft& aircraft);
    void DeleteAircraft(const NetworkAircraft& aircraft, QString reason);
    void DeleteAllAircraft();
    void UpdateControllers(QList<Controller>& controllers);
    void DeleteAllControllers();
    void SendFastPositionUpdate(const NetworkAircraft& aircraft, const AircraftVisualState& visualState, const VelocityVector& positionalVelocityVector, const VelocityVector& rotationalVelocityVector);
    void SendHeartbeat(const QString callsign);
    void SendRadioMessage(const QString message);
    void RadioMessageReceived(const QString from, const QString message, bool isDirect);
    void NotificationPosted(const QString message, qint64 color);
    void SendPrivateMessage(const QString to, const QString message);
    void PrivateMessageReceived(const QString from, const QString message);
    void NetworkConnected(QString callsign, QString selcal);
    void NetworkDisconnected();

    int XplaneVersion() const { return m_xplaneVersion; }

    void SetStationCallsign(int com, QString callsign);

private:
    void SubscribeDataRefs();
    void SubscribeDataRef(std::string dataRef, uint32_t id, uint32_t frequency);
    void setDataRefValue(std::string dataRef, float value);
    void sendCommand(std::string command);

    void processMessage(QString message);
    void processPacket(const BaseDto& packet);
    void clearSimConnection();

    void requestPluginVersion();
    void validateCsl();

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
    void aircraftAddedToSim(QString callsign);
    void aircraftRemovedFromSim(QString callsign);
    void ignoreList(QStringList list);
    void sendWallop(QString message);
    void simPausedStateChanged(bool paused);

private:
    QUdpSocket* socket;
    qint64 m_lastUdpTimestamp;
    bool m_simConnected = false;
    bool m_initialHandshake = false;
    bool m_validPluginVersion = true;
    bool m_cslValidated = false;
    bool m_validCsl = true;
    bool m_simPaused = false;

    UserAircraftData m_userAircraftData{};
    UserAircraftConfigData m_userAircraftConfigData{};
    RadioStackState m_radioStackState{};

    QList<QString> m_ignoreList;
    QTimer m_heartbeatTimer;
    QTimer m_xplaneDataTimer;

    QList<QString> m_subscribedDataRefs;

    typedef struct rref_data_type {
        int idx;
        float val;
    } rref_data_type;

    #define SKIP_EMPTY(value) if(value == 0) continue;

    int m_xplaneVersion;

    bool m_keepSocketAlive = false;
    std::unique_ptr<std::thread> m_socketThread;
    nng_socket m_socket;
    QList<nng_socket> m_visualSockets;

    template<class T>
    void SendDto(const T& dto)
    {
        msgpack::sbuffer dtoBuf;
        if (encodeDto(dtoBuf, dto))
        {
            if (dtoBuf.size() == 0)
                return;

            std::vector<unsigned char> dgBuffer(dtoBuf.data(), dtoBuf.data() + dtoBuf.size());
            nng_send(m_socket, reinterpret_cast<char*>(dgBuffer.data()), dgBuffer.size(), NNG_FLAG_NONBLOCK);

            for(auto &visualSocket : m_visualSockets) {
                nng_send(visualSocket, reinterpret_cast<char*>(dgBuffer.data()), dgBuffer.size(), NNG_FLAG_NONBLOCK);
            }
        }
    }
};

#endif

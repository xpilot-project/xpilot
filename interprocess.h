#ifndef INTERPROCESS_H
#define INTERPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariant>
#include "protobuf/Envelope.pb.h"

struct RadioStack {
    Q_GADGET
    Q_PROPERTY(bool avionicsPowerOn MEMBER avionicsPowerOn)
    Q_PROPERTY(int com1Frequency MEMBER com1Frequency)
    Q_PROPERTY(bool com1ReceiveEnabled MEMBER com1ReceiveEnabled)
    Q_PROPERTY(int com2Frequency MEMBER com2Frequency)
    Q_PROPERTY(bool com2ReceiveEnabled MEMBER com2ReceiveEnabled)
    Q_PROPERTY(int transmitComSelection MEMBER transmitComSelection)
public:
    bool avionicsPowerOn;
    int com1Frequency;
    bool com1ReceiveEnabled;
    int com2Frequency;
    bool com2ReceiveEnabled;
    int transmitComSelection;
};
Q_DECLARE_METATYPE(RadioStack)

struct AppConfig
{
    Q_GADGET
    Q_PROPERTY(QString vatsimId MEMBER vatsimId)
    Q_PROPERTY(QString vatsimPassword MEMBER vatsimPassword)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString homeAirport MEMBER homeAirport)
public:
    QString vatsimId;
    QString vatsimPassword;
    QString name;
    QString homeAirport;
};
Q_DECLARE_METATYPE(AppConfig)

class InterProcess : public QObject
{
    Q_OBJECT
    Q_ENUMS(NotificationType)
public:
    explicit InterProcess(QObject *parent = nullptr);
    ~InterProcess();
    void Tick();

    enum NotificationType {
        Info = 0,
        Warning = 1,
        Error = 2
    };

public slots:
    void onHandleSetTransponderCode(int code);
    void onHandleSetRadioStack(int radio, int frequency);
    void onHandleTransponderModeC(bool active);
    void onHandleTransponderIdent();
    void onHandleRequestConfig();
    void onHandleUpdateConfig(QVariant config);

private:
    QProcess* process;
    void sendEnvelope(const xpilot::Envelope& envelope);
    void restartProcess();

signals:
    void simulatorConnected(bool isConnected);
    void radioStackReceived(RadioStack stack);
    void appConfigReceived(AppConfig config);
    void notificationPosted(NotificationType type, QString message);
};

#endif // INTERPROCESS_H

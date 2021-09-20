#ifndef INTERPROCESS_H
#define INTERPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariant>
#include "protobuf/Envelope.pb.h"
#include "radiostack.h"
#include "appconfig.h"
#include "nearbyatc.h"

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
    void nearbyAtcReceived(QList<NearbyAtc> stations);
};

#endif // INTERPROCESS_H

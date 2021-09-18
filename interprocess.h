#ifndef INTERPROCESS_H
#define INTERPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include "protobuf/Envelope.pb.h"

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
    void onSetTransponderCode(int code);
    void onSetRadioStack(int radio, int frequency);

private:
    QProcess process;
    void sendEnvelope(const xpilot::Envelope& envelope);

signals:
    void simulatorConnected(bool isConnected);
    void notificationPosted(NotificationType type, QString message);
};

#endif // INTERPROCESS_H

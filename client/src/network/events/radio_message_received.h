#ifndef RADIO_MESSAGE_RECEIVED_H
#define RADIO_MESSAGE_RECEIVED_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>

struct RadioMessageReceived
{
    Q_GADGET

    Q_PROPERTY(QString From MEMBER From)
    Q_PROPERTY(QString Message MEMBER Message)
    Q_PROPERTY(QVariant Frequencies MEMBER Frequencies)
    Q_PROPERTY(bool IsDirect MEMBER IsDirect)
    Q_PROPERTY(bool DualReceiver MEMBER DualReceiver)

public:
    QString From;
    QString Message;
    QVariant Frequencies;
    bool IsDirect;
    bool DualReceiver;
};

Q_DECLARE_METATYPE(RadioMessageReceived)

#endif

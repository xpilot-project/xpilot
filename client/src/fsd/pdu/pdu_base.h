#ifndef PDUBASE_H
#define PDUBASE_H

#include <QString>
#include <QStringList>
#include <QStringBuilder>
#include <QDebug>

#include "pdu_format_exception.h"
#include "../serializer.h"

class PDUBase
{
public:
    PDUBase() {}
    PDUBase(QString from, QString to);

    static uint PackPitchBankHeading(double pitch, double bank, double heading);
    static void UnpackPitchBankHeading(uint pbh, double &pitch, double& bank, double& heading);

    inline static const QString ClientQueryBroadcastRecipient = "@94835";
    inline static const QString ClientQueryBroadcastRecipientPilots = "@94386";
    inline static const QChar Delimeter = ':';
    inline static const QString PacketDelimeter = "\r\n";
    inline static const QString ServerCallsign = "SERVER";

    static QString Reassemble(QStringList fields)
    {
        return fields.join(Delimeter);
    }

    QString From;
    QString To;
};

template <class T>
QString Serialize(const T &message)
{
    return message.pdu() % message.toTokens().join(':') % QStringLiteral("\r\n");
}

#endif

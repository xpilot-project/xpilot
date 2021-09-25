#ifndef PDUBASE_H
#define PDUBASE_H

#include <QString>
#include <QStringList>
#include <QStringBuilder>
#include <QDebug>

#include "../serializer.h"

class PDUBase
{
public:
    PDUBase(QString from, QString to);

    QString From;
    QString To;

    virtual QString Serialize() = 0;

    static QString Reassemble(QStringList const& fields)
    {
        return fields.join(Delimeter);
    }

    static uint PackPitchBankHeading(double pitch, double bank, double heading);

    static void UnpackPitchBankHeading(uint pbh, double &pitch, double& bank, double& heading);

    inline static const QString ClientQueryBroadcastRecipient = "@94835";
    inline static const QString ClientQueryBroadcastRecipientPilots = "@94386";
    inline static const QChar Delimeter = ':';
    inline static const QString PacketDelimeter = "\r\n";
    inline static const QString ServerCallsign = "SERVER";
};

#endif

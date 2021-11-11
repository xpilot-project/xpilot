#ifndef PDU_PING_H
#define PDU_PING_H

#include <QString>
#include "pdu_base.h"

class PDUPing: public PDUBase
{
public:
    PDUPing(QString from, QString to, QString timeStamp);

    QStringList toTokens() const;

    static PDUPing fromTokens(const QStringList& fields);

    static QString pdu() { return "$PI"; }

    QString Timestamp;

private:
    PDUPing();
};

#endif

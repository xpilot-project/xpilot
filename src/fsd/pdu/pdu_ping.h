#ifndef PDU_PING_H
#define PDU_PING_H

#include <QString>
#include "pdu_base.h"

class PDUPing: public PDUBase
{
public:
    PDUPing(QString from, QString to, QString timeStamp);

    QString Serialize() override;

    static PDUPing Parse(QStringList fields);

    QString Timestamp;
};

#endif

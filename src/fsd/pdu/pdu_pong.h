#ifndef PDU_PONG_H
#define PDU_PONG_H

#include <QString>
#include "pdu_base.h"

class PDUPong: public PDUBase
{
public:
    PDUPong(QString from, QString to, QString timeStamp);

    QString Serialize() override;

    static PDUPong Parse(QStringList fields);

    QString Timestamp;
};

#endif

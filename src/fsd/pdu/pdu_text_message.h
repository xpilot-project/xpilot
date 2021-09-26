#ifndef PDU_TEXT_MESSAGE_H
#define PDU_TEXT_MESSAGE_H

#include <QString>
#include "pdu_base.h"

class PDUTextMessage: public PDUBase
{
public:
    PDUTextMessage(QString from, QString to, QString message);

    QString Serialize() override;

    static PDUTextMessage Parse(QStringList fields);

    QString Message;
};

#endif

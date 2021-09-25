#ifndef PDU_BROADCASTMESSGE_H
#define PDU_BROADCASTMESSGE_H

#include <QString>
#include "pdu_base.h"

class PDUBroadcastMessage : public PDUBase
{
public:
    PDUBroadcastMessage(QString from, QString message);

    QString Serialize() override;

    static PDUBroadcastMessage Parse(QStringList fields);

    QString Message;
};

#endif

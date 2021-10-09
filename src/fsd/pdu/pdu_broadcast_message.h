#ifndef PDU_BROADCASTMESSGE_H
#define PDU_BROADCASTMESSGE_H

#include <QString>
#include "pdu_base.h"

class PDUBroadcastMessage : public PDUBase
{
public:
    PDUBroadcastMessage(QString from, QString message);

    QStringList toTokens() const;

    static PDUBroadcastMessage fromTokens(const QStringList& fields);

    static QString pdu() { return "#TM"; }

    QString Message;

private:
    PDUBroadcastMessage();
};

#endif

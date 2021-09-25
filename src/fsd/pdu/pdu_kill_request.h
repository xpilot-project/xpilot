#ifndef PDU_KILLREQUEST_H
#define PDU_KILLREQUEST_H

#include <QString>
#include "pdu_base.h"

class PDUKillRequest: public PDUBase
{
public:
    PDUKillRequest(QString from, QString victim, QString reason);

    QString Serialize() override;

    static PDUKillRequest Parse(QStringList fields);

    QString Reason;
};

#endif

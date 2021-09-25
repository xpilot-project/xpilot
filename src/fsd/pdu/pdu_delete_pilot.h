#ifndef PDU_DELETEPILOT_H
#define PDU_DELETEPILOT_H

#include <QString>
#include "pdu_base.h"

class PDUDeletePilot : public PDUBase
{
public:
    PDUDeletePilot(QString from, QString cid);

    QString Serialize() override;

    static PDUDeletePilot Parse(QStringList fields);

    QString CID;
};

#endif

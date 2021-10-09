#ifndef PDU_DELETEPILOT_H
#define PDU_DELETEPILOT_H

#include <QString>
#include "pdu_base.h"

class PDUDeletePilot : public PDUBase
{
public:
    PDUDeletePilot(QString from, QString cid);

    QStringList toTokens() const;

    static PDUDeletePilot fromTokens(const QStringList& fields);

    static QString pdu() { return "#DP"; }

    QString CID;

private:
    PDUDeletePilot();
};

#endif

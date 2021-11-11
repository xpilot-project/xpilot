#ifndef PDU_KILLREQUEST_H
#define PDU_KILLREQUEST_H

#include <QString>
#include "pdu_base.h"

class PDUKillRequest: public PDUBase
{
public:
    PDUKillRequest(QString from, QString victim, QString reason);

    QStringList toTokens() const;

    static PDUKillRequest fromTokens(const QStringList& fields);

    static QString pdu() { return "$!!"; }

    QString Reason;

private:
    PDUKillRequest();
};

#endif

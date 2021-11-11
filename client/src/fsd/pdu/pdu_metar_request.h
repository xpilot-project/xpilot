#ifndef PDU_METARREQUEST_H
#define PDU_METARREQUEST_H

#include <QString>
#include "pdu_base.h"

class PDUMetarRequest: public PDUBase
{
public:
    PDUMetarRequest(QString from, QString station);

    QStringList toTokens() const;

    static PDUMetarRequest fromTokens(const QStringList& fields);

    static QString pdu() { return "$AX"; }

    QString Station;

private:
    PDUMetarRequest();
};

#endif

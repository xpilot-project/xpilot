#ifndef PDU_CLIENTQUERYRESPONSE_H
#define PDU_CLIENTQUERYRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUClientQueryResponse : public PDUBase
{
public:
    PDUClientQueryResponse(QString from, QString to, ClientQueryType queryType, QStringList payload);

    QStringList toTokens() const;

    static PDUClientQueryResponse fromTokens(const QStringList& fields);

    static QString pdu() { return "$CR"; }

    ClientQueryType QueryType;
    QStringList Payload;

private:
    PDUClientQueryResponse();
};

#endif

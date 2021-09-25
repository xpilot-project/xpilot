#ifndef PDU_CLIENTQUERYRESPONSE_H
#define PDU_CLIENTQUERYRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUClientQueryResponse : public PDUBase
{
public:
    PDUClientQueryResponse(QString from, QString to, ClientQueryType queryType, QStringList payload);

    QString Serialize() override;

    static PDUClientQueryResponse Parse(QStringList fields);

    ClientQueryType QueryType;
    QStringList Payload;
};

#endif

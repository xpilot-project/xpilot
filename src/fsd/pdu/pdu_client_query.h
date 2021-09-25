#ifndef PDU_CLIENTQUERY_H
#define PDU_CLIENTQUERY_H

#include <QString>
#include "pdu_base.h"

class PDUClientQuery : public PDUBase
{
public:
    PDUClientQuery(QString from, QString to, ClientQueryType queryType);

    PDUClientQuery(QString from, QString to, ClientQueryType queryType, QStringList payload);

    QString Serialize() override;

    static PDUClientQuery Parse(QStringList fields);

    ClientQueryType QueryType;
    QStringList Payload;
};

#endif

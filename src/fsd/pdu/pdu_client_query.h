#ifndef PDU_CLIENTQUERY_H
#define PDU_CLIENTQUERY_H

#include <QString>
#include "pdu_base.h"

class PDUClientQuery : public PDUBase
{
public:
    PDUClientQuery(QString from, QString to, ClientQueryType queryType, QStringList payload = {});

    QStringList toTokens() const;

    static PDUClientQuery fromTokens(const QStringList& fields);

    static QString pdu() { return "$CQ"; }

    ClientQueryType QueryType;
    QStringList Payload;

private:
    PDUClientQuery();
};

#endif

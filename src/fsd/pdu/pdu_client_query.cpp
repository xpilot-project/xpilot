#include "pdu_client_query.h"

PDUClientQuery::PDUClientQuery(QString from, QString to, ClientQueryType queryType) :
    PDUBase(from, to)
{
    QueryType = queryType;
}

PDUClientQuery::PDUClientQuery(QString from, QString to, ClientQueryType queryType, QStringList payload) :
    PDUBase(from, to)
{
    QueryType = queryType;
    Payload = payload;
}

QString PDUClientQuery::Serialize()
{
    QStringList tokens;

    tokens.append("$CQ");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(toQString(QueryType));
    if(!Payload.isEmpty()) {
        for(const auto& payloadItem : Payload) {
            tokens.append(Delimeter);
            tokens.append(payloadItem);
        }
    }

    return tokens.join("");
}

PDUClientQuery PDUClientQuery::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    QStringList payload;
    if(fields.length() > 3) {
        for(int i = 3; i < fields.length(); i++) {
            payload.append(fields[i]);
        }
    }

    return PDUClientQuery(fields[0], fields[1], fromQString<ClientQueryType>(fields[2]), payload);
}

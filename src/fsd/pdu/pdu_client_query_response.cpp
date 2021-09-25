#include "pdu_client_query_response.h"

PDUClientQueryResponse::PDUClientQueryResponse(QString from, QString to, ClientQueryType queryType, QStringList payload) :
    PDUBase(from, to)
{
    QueryType = queryType;
    Payload = payload;
}

QString PDUClientQueryResponse::Serialize()
{
    QStringList tokens;

    tokens.append("$CR");
    tokens.append(From);
    tokens.append(To);
    tokens.append(toQString(QueryType));
    for(const auto& payloadItem : Payload) {
        tokens.append(payloadItem);
    }

    return tokens.join(Delimeter);
}

PDUClientQueryResponse PDUClientQueryResponse::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    QStringList payload;
    if(fields.length() > 3) {
        for(int i = 3; i < fields.length(); i++) {
            payload.append(fields[i]);
        }
    }

    return PDUClientQueryResponse(fields[0], fields[1], fromQString<ClientQueryType>(fields[2]), payload);
}

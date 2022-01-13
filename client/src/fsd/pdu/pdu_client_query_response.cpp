#include "pdu_client_query_response.h"

PDUClientQueryResponse::PDUClientQueryResponse() : PDUBase() {}

PDUClientQueryResponse::PDUClientQueryResponse(QString from, QString to, ClientQueryType queryType, QStringList payload) :
    PDUBase(from, to)
{
    QueryType = queryType;
    Payload = payload;
}

QStringList PDUClientQueryResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(toQString(QueryType));
    tokens.append(Payload);
    return tokens;
}

PDUClientQueryResponse PDUClientQueryResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList responseData;
    if (tokens.size() > 3) { responseData = tokens.mid(3); }
    return PDUClientQueryResponse(tokens[0], tokens[1], fromQString<ClientQueryType>(tokens[2]), responseData);
}

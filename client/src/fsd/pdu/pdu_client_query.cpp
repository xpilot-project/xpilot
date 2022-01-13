#include "pdu_client_query.h"

PDUClientQuery::PDUClientQuery() : PDUBase() {}

PDUClientQuery::PDUClientQuery(QString from, QString to, ClientQueryType queryType, QStringList payload) :
    PDUBase(from, to)
{
    QueryType = queryType;
    Payload = payload;
}

QStringList PDUClientQuery::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(toQString(QueryType));
    tokens.append(Payload);
    return tokens;
}

PDUClientQuery PDUClientQuery::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList payload;
    if (tokens.size() > 3) { payload = tokens.mid(3); }
    return PDUClientQuery(tokens[0], tokens[1], fromQString<ClientQueryType>(tokens[2]), payload);
}

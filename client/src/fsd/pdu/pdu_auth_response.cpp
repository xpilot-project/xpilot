#include "pdu_auth_response.h"

PDUAuthResponse::PDUAuthResponse() : PDUBase() {}

PDUAuthResponse::PDUAuthResponse(QString from, QString to, QString response) :
    PDUBase(from, to)
{
    Response = response;
}

QStringList PDUAuthResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Response);
    return tokens;
}

PDUAuthResponse PDUAuthResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUAuthResponse(tokens[0], tokens[1], tokens[2]);
}

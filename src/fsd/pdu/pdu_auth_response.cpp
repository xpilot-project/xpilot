#include "pdu_auth_response.h"

PDUAuthResponse::PDUAuthResponse(QString from, QString to, QString response) :
    PDUBase(from, to)
{
    Response = response;
}

QString PDUAuthResponse::Serialize()
{
    QStringList tokens;

    tokens.append("$ZR");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Response);

    return tokens.join("");
}

PDUAuthResponse PDUAuthResponse::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    return PDUAuthResponse(fields[0], fields[1], fields[2]);
}

#include "pdu_metar_response.h"

PDUMetarResponse::PDUMetarResponse() : PDUBase() {}

PDUMetarResponse::PDUMetarResponse(QString to, QString metar) :
    PDUBase(ServerCallsign, to)
{
    Metar = metar;
}

QStringList PDUMetarResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Metar);
    return tokens;
}

PDUMetarResponse PDUMetarResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 4) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUMetarResponse(tokens[1], tokens[3]);
}

#include "pdu_metar_request.h"

PDUMetarRequest::PDUMetarRequest() : PDUBase() {}

PDUMetarRequest::PDUMetarRequest(QString from, QString station) :
    PDUBase(from, PDUBase::ServerCallsign)
{
    Station = station;
}

QStringList PDUMetarRequest::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append("METAR");
    tokens.append(Station);
    return tokens;
}

PDUMetarRequest PDUMetarRequest::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 4) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUMetarRequest(tokens[0], tokens[3]);
}

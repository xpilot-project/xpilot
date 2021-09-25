#include "pdu_metar_request.h"

PDUMetarRequest::PDUMetarRequest(QString from, QString station) :
    PDUBase(from, PDUBase::ServerCallsign)
{
    Station = station;
}

QString PDUMetarRequest::Serialize()
{
    QStringList tokens;

    tokens.append("$AX");
    tokens.append(From);
    tokens.append(To);
    tokens.append("METAR");
    tokens.append(Station);

    return tokens.join(Delimeter);
}

PDUMetarRequest PDUMetarRequest::Parse(QStringList fields)
{
    if(fields.length() < 4) {

    }

    return PDUMetarRequest(fields[0], fields[3]);
}

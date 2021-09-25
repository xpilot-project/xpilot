#include "pdu_metar_response.h"

PDUMetarResponse::PDUMetarResponse(QString to, QString metar) :
    PDUBase(ServerCallsign, to)
{
    Metar = metar;
}

QString PDUMetarResponse::Serialize()
{
    QStringList tokens;

    tokens.append("$AR");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Metar);

    return tokens.join(Delimeter);
}

PDUMetarResponse PDUMetarResponse::Parse(QStringList fields)
{
    if(fields.length() < 4) {

    }

    return PDUMetarResponse(fields[1], fields[3]);
}

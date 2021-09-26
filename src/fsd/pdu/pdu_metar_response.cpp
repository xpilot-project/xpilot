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
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Metar);

    return tokens.join("");
}

PDUMetarResponse PDUMetarResponse::Parse(QStringList fields)
{
    if(fields.length() < 4) {

    }

    return PDUMetarResponse(fields[1], fields[3]);
}

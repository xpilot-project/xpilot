#include "pdu_plane_info_request.h"

PDUPlaneInfoRequest::PDUPlaneInfoRequest(QString from, QString to) :
    PDUBase(from, to)
{
    
}

QString PDUPlaneInfoRequest::Serialize()
{
    QStringList tokens;

    tokens.append("#SB");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append("PIR");

    return tokens.join("");
}

PDUPlaneInfoRequest PDUPlaneInfoRequest::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    return PDUPlaneInfoRequest(fields[0], fields[1]);
}

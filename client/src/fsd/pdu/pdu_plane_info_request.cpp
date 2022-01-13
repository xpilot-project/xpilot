#include "pdu_plane_info_request.h"

PDUPlaneInfoRequest::PDUPlaneInfoRequest() : PDUBase() {}

PDUPlaneInfoRequest::PDUPlaneInfoRequest(QString from, QString to) :
    PDUBase(from, to)
{
    
}

QStringList PDUPlaneInfoRequest::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append("PIR");
    return tokens;
}

PDUPlaneInfoRequest PDUPlaneInfoRequest::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUPlaneInfoRequest(tokens[0], tokens[1]);
}

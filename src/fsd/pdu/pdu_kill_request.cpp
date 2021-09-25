#include "pdu_kill_request.h"

PDUKillRequest::PDUKillRequest(QString from, QString victim, QString reason) :
    PDUBase(from, victim)
{
    Reason = reason;
}

QString PDUKillRequest::Serialize()
{
    QStringList tokens;

    tokens.append("$!!");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Reason);

    return tokens.join(Delimeter);
}

PDUKillRequest PDUKillRequest::Parse(QStringList fields)
{
    if(fields.length() < 2) {

    }

    return PDUKillRequest(fields[0], fields[1], fields.length() > 2 ? fields[2] : "");
}

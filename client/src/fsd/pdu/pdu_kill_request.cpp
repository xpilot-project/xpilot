#include "pdu_kill_request.h"

PDUKillRequest::PDUKillRequest() : PDUBase() {}

PDUKillRequest::PDUKillRequest(QString from, QString victim, QString reason) :
    PDUBase(from, victim)
{
    Reason = reason;
}

QStringList PDUKillRequest::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Reason);
    return tokens;
}

PDUKillRequest PDUKillRequest::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 2) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUKillRequest(tokens[0], tokens[1], tokens.length() > 2 ? tokens[2] : "");
}

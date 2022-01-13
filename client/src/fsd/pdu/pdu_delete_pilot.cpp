#include "pdu_delete_pilot.h"

PDUDeletePilot::PDUDeletePilot() : PDUBase() {}

PDUDeletePilot::PDUDeletePilot(QString from, QString cid) :
    PDUBase(from, "")
{
    CID = cid;
}

QStringList PDUDeletePilot::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(CID);
    return tokens;
}

PDUDeletePilot PDUDeletePilot::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 1) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUDeletePilot(tokens[0], tokens.length() >= 2 ? tokens[1] : "");
}

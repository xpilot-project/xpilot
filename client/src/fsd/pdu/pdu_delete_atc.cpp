#include "pdu_delete_atc.h"

PDUDeleteATC::PDUDeleteATC() : PDUBase() {}

PDUDeleteATC::PDUDeleteATC(QString from, QString cid) : PDUBase(from, "")
{
    CID = cid;
}

QStringList PDUDeleteATC::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(CID);
    return tokens;
}

PDUDeleteATC PDUDeleteATC::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 1) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUDeleteATC(tokens[0], tokens.length() >= 2 ? tokens[1] : "");
}

#include "pdu_ping.h"

PDUPing::PDUPing() : PDUBase() {}

PDUPing::PDUPing(QString from, QString to, QString timeStamp) :
    PDUBase(from, to)
{
    Timestamp = timeStamp;
}

QStringList PDUPing::toTokens() const
{
    QStringList tokens;
    tokens.append("$PI");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Timestamp);
    return tokens;
}

PDUPing PDUPing::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUPing(tokens[0], tokens[1], tokens[2]);
}

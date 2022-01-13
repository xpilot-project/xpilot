#include "pdu_pong.h"

PDUPong::PDUPong() : PDUBase() {}

PDUPong::PDUPong(QString from, QString to, QString timeStamp) :
    PDUBase(from, to)
{
    Timestamp = timeStamp;
}

QStringList PDUPong::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Timestamp);
    return tokens;
}

PDUPong PDUPong::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUPong(tokens[0], tokens[1], tokens[2]);
}

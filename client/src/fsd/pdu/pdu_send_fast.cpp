#include "pdu_send_fast.h"

PDUSendFast::PDUSendFast() : PDUBase() {}

PDUSendFast::PDUSendFast(QString from, QString to, bool sendFast) : PDUBase(from, to)
{
    DoSendFast = sendFast;
}

QStringList PDUSendFast::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number((int)DoSendFast));
    return tokens;
}

PDUSendFast PDUSendFast::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUSendFast(tokens[0], tokens[1], static_cast<bool>(tokens[2].toInt()));
}

#include "pdu_broadcast_message.h"

PDUBroadcastMessage::PDUBroadcastMessage() : PDUBase() {}

PDUBroadcastMessage::PDUBroadcastMessage(QString from, QString message) :
    PDUBase(from, "*")
{
    Message = message;
}

QStringList PDUBroadcastMessage::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Message);
    return tokens;
}

PDUBroadcastMessage PDUBroadcastMessage::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList messageTokens = tokens.mid(2);
    return PDUBroadcastMessage(tokens[0], messageTokens.join(":"));
}

#include "pdu_text_message.h"

PDUTextMessage::PDUTextMessage() : PDUBase() {}

PDUTextMessage::PDUTextMessage(QString from, QString to, QString message) :
    PDUBase(from, to),
    Message(message)
{

}

QStringList PDUTextMessage::toTokens() const
{
    QStringList tokens;
    tokens.push_back(From);
    tokens.push_back(To);
    tokens.push_back(Message);
    return tokens;
}

PDUTextMessage PDUTextMessage::fromTokens(const QStringList &tokens)
{
    if(tokens.size() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList msgTokens = tokens.mid(2);
    return PDUTextMessage(tokens[0], tokens[1], msgTokens.join(":"));
}

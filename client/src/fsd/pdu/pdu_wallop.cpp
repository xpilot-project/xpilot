#include "pdu_wallop.h"

PDUWallop::PDUWallop() : PDUBase() {}

PDUWallop::PDUWallop(QString from, QString message) :
    PDUBase(from, "*S")
{
    Message = message;
}

QStringList PDUWallop::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Message);
    return tokens;
}

PDUWallop PDUWallop::fromTokens(const QStringList &tokens)
{
    if(tokens.length() > 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList msgTokens = tokens.mid(2);
    return PDUWallop(tokens[0], msgTokens.join(":"));
}

#include "pdu_server_identification.h"

PDUServerIdentification::PDUServerIdentification() : PDUBase() {}

PDUServerIdentification::PDUServerIdentification(QString from, QString to, QString version, QString initialChallengeKey)
    : PDUBase(from, to)
{
    Version = version;
    InitialChallengeKey = initialChallengeKey;
}

QStringList PDUServerIdentification::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(Version);
    tokens.append(InitialChallengeKey);
    return tokens;
}

PDUServerIdentification PDUServerIdentification::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 4) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUServerIdentification(tokens[0], tokens[1], tokens[2], tokens[3]);
}

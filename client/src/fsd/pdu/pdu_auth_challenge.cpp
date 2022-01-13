#include "pdu_auth_challenge.h"

PDUAuthChallenge::PDUAuthChallenge() : PDUBase() {}

PDUAuthChallenge::PDUAuthChallenge(QString from, QString to, QString challenge) :
    PDUBase(from, to)
{
    ChallengeKey = challenge;
}

QStringList PDUAuthChallenge::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(ChallengeKey);
    return tokens;
}

PDUAuthChallenge PDUAuthChallenge::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUAuthChallenge(tokens[0], tokens[1], tokens[2]);
}

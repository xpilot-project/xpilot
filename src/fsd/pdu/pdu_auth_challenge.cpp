#include "pdu_auth_challenge.h"

PDUAuthChallenge::PDUAuthChallenge(QString from, QString to, QString challenge) :
    PDUBase(from, to)
{
    Challenge = challenge;
}

QString PDUAuthChallenge::Serialize()
{
    QStringList tokens;

    tokens.append("$ZC");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Challenge);

    return tokens.join("");
}

PDUAuthChallenge PDUAuthChallenge::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    return PDUAuthChallenge(fields[0], fields[1], fields[2]);
}

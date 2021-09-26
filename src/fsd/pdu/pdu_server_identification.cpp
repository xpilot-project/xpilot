#include "pdu_server_identification.h"

PDUServerIdentification::PDUServerIdentification(QString from, QString to, QString version, QString initialChallengeKey)
    : PDUBase(from, to)
{
    Version = version;
    InitialChallengeKey = initialChallengeKey;
}

QString PDUServerIdentification::Serialize()
{
    QStringList tokens;

    tokens.append("$DI");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Version);
    tokens.append(Delimeter);
    tokens.append(InitialChallengeKey);

    return tokens.join("");
}

PDUServerIdentification PDUServerIdentification::Parse(QStringList fields)
{
    if(fields.length() < 4) {

    }

    return PDUServerIdentification(fields[0], fields[1], fields[2], fields[3]);
}

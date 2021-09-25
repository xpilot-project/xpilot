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
    tokens.append(To);
    tokens.append(Version);
    tokens.append(InitialChallengeKey);

    return tokens.join(Delimeter);
}

PDUServerIdentification PDUServerIdentification::Parse(QStringList fields)
{
    if(fields.length() < 4) {

    }

    return PDUServerIdentification(fields[0], fields[1], fields[2], fields[3]);
}

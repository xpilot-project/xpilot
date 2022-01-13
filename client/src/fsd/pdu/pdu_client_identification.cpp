#include "pdu_client_identification.h"

PDUClientIdentification::PDUClientIdentification() : PDUBase() {}

PDUClientIdentification::PDUClientIdentification(QString from, ushort clientID, QString clientName, int majorVersion, int minorVersion, QString cid, QString sysUID, QString initialChallenge) : PDUBase(from, ServerCallsign)
{
    ClientId = clientID;
    ClientName = clientName;
    MajorVersion = majorVersion;
    MinorVersion = minorVersion;
    CID = cid;
    SystemUID = sysUID;
    InitialChallenge = initialChallenge;
}

QStringList PDUClientIdentification::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number(ClientId, 16).toLower());
    tokens.append(ClientName);
    tokens.append(QString::number(MajorVersion));
    tokens.append(QString::number(MinorVersion));
    tokens.append(CID);
    tokens.append(SystemUID);
    if(!InitialChallenge.isEmpty()) {
        tokens.append(InitialChallenge);
    }
    return tokens;
}

PDUClientIdentification PDUClientIdentification::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 8) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUClientIdentification(tokens[0],tokens[2].toUShort(nullptr, 16),tokens[3], tokens[4].toInt(),
            tokens[5].toInt(), tokens[6], tokens[7], tokens.size() > 8 ? tokens[8] : QString());
}

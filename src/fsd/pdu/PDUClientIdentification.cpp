#include "PDUClientIdentification.h"

PDUClientIdentification::PDUClientIdentification(QString from, ushort clientID, QString clientName, int majorVersion, int minorVersion, QString cid, QString sysUID, QString initialChallenge) : PDUBase(from, ServerCallsign)
{
    ClientID = clientID;
    ClientName = clientName;
    MajorVersion = majorVersion;
    MinorVersion = minorVersion;
    CID = cid;
    SysUID = sysUID;
    InitialChallenge = initialChallenge;
}

QString PDUClientIdentification::Serialize()
{
    QStringList tokens;

    tokens.append("$ID");
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number(ClientID, 16).toLower());
    tokens.append(ClientName);
    tokens.append(QString(MajorVersion));
    tokens.append(QString(MinorVersion));
    tokens.append(CID);
    tokens.append(SysUID);
    if(!InitialChallenge.isEmpty()) {
        tokens.append(InitialChallenge);
    }

    return tokens.join(Delimeter);
}

PDUClientIdentification PDUClientIdentification::Parse(QStringList fields)
{
    if(fields.length() < 8) {
        qDebug() << u"Invalid field count: " << Reassemble(fields);
    }

    return PDUClientIdentification( fields[0],fields[2].toUShort(nullptr, 16),fields[3], fields[4].toInt(),
            fields[5].toInt(), fields[6], fields[7], fields.size() > 8 ? fields[8] : QString());
}


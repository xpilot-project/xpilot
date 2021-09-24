#ifndef PDU_CLIENTIDENTIFICATION_H
#define PDU_CLIENTIDENTIFICATION_H

#include <QString>
#include "PDUBase.h"

class PDUClientIdentification: public PDUBase
{
public:
    PDUClientIdentification(QString from, ushort clientID, QString clientName, int majorVersion, int minorVersion, QString cid, QString sysUID, QString initialChallenge);

    QString Serialize() override;

    static PDUClientIdentification Parse(QStringList fields);

    ushort ClientID;
    QString ClientName;
    int MajorVersion;
    int MinorVersion;
    QString CID;
    QString SysUID;
    QString InitialChallenge;
};

#endif

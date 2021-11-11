#ifndef PDU_CLIENTIDENTIFICATION_H
#define PDU_CLIENTIDENTIFICATION_H

#include <QString>
#include "pdu_base.h"

class PDUClientIdentification: public PDUBase
{
public:
    PDUClientIdentification(QString from, ushort clientID, QString clientName, int majorVersion, int minorVersion, QString cid, QString sysUID, QString initialChallenge);

    QStringList toTokens() const;

    static PDUClientIdentification fromTokens(const QStringList& fields);

    static QString pdu() { return "$ID"; }

    ushort ClientId;
    QString ClientName;
    int MajorVersion;
    int MinorVersion;
    QString CID;
    QString SystemUID;
    QString InitialChallenge;

private:
    PDUClientIdentification();
};

#endif

#ifndef PDU_SERVERIDENTIFICATION_H
#define PDU_SERVERIDENTIFICATION_H

#include <QString>
#include "pdu_base.h"

class PDUServerIdentification: public PDUBase
{
public:
    PDUServerIdentification(QString from, QString to, QString version, QString initialChallengeKey);

    QStringList toTokens() const;

    static PDUServerIdentification fromTokens(const QStringList& fields);

    static QString pdu() { return "#DI"; }

    QString Version;
    QString InitialChallengeKey;

private:
    PDUServerIdentification();
};

#endif

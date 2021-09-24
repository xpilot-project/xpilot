#ifndef PDU_SERVERIDENTIFICATION_H
#define PDU_SERVERIDENTIFICATION_H

#include <QString>
#include "PDUBase.h"

class PDUServerIdentification: public PDUBase
{
public:
    PDUServerIdentification(QString from, QString to, QString version, QString initialChallengeKey);

    QString Serialize() override;

    static PDUServerIdentification Parse(QStringList fields);

    QString Version;
    QString InitialChallengeKey;
};

#endif

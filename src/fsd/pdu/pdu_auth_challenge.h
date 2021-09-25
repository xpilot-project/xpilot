#ifndef PDU_AUTHCHALLENGE_H
#define PDU_AUTHCHALLENGE_H

#include <QString>
#include "pdu_base.h"

class PDUAuthChallenge : public PDUBase
{
public:
    PDUAuthChallenge(QString from, QString to, QString challenge);

    QString Serialize() override;

    static PDUAuthChallenge Parse(QStringList fields);

    QString Challenge;
};

#endif

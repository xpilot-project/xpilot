#ifndef PDU_AUTHCHALLENGE_H
#define PDU_AUTHCHALLENGE_H

#include <QString>
#include "pdu_base.h"

class PDUAuthChallenge : public PDUBase
{
public:
    PDUAuthChallenge(QString from, QString to, QString challenge);

    QStringList toTokens() const;

    static PDUAuthChallenge fromTokens(const QStringList& fields);

    static QString pdu() { return "$ZC"; }

    QString ChallengeKey;

private:
    PDUAuthChallenge();
};

#endif

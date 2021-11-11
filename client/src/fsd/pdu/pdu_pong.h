#ifndef PDU_PONG_H
#define PDU_PONG_H

#include <QString>
#include "pdu_base.h"

class PDUPong: public PDUBase
{
public:
    PDUPong(QString from, QString to, QString timeStamp);

    QStringList toTokens() const;

    static PDUPong fromTokens(const QStringList& fields);

    static QString pdu() { return "$PO"; }

    QString Timestamp;

private:
    PDUPong();
};

#endif

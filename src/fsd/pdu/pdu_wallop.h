#ifndef PDU_WALLOP_H
#define PDU_WALLOP_H

#include <QString>
#include "pdu_base.h"

class PDUWallop: public PDUBase
{
public:
    PDUWallop(QString from, QString message);

    QStringList toTokens() const;

    static PDUWallop fromTokens(const QStringList& fields);

    static QString pdu() { return "#TM"; }

    QString Message;

private:
    PDUWallop();
};

#endif

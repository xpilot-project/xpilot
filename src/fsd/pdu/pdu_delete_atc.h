#ifndef PDU_DELETEATC_H
#define PDU_DELETEATC_H

#include <QString>
#include "pdu_base.h"

class PDUDeleteATC : public PDUBase
{
public:
    PDUDeleteATC(QString from, QString cid);

    QStringList toTokens() const;

    static PDUDeleteATC fromTokens(const QStringList& fields);

    static QString pdu() { return "#DA"; }

    QString CID;

private:
    PDUDeleteATC();
};

#endif

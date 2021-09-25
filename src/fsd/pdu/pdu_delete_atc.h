#ifndef PDU_DELETEATC_H
#define PDU_DELETEATC_H

#include <QString>
#include "pdu_base.h"

class PDUDeleteATC : public PDUBase
{
public:
    PDUDeleteATC(QString from, QString cid);

    QString Serialize() override;

    static PDUDeleteATC Parse(QStringList fields);

    QString CID;
};

#endif

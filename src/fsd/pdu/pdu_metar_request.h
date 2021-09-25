#ifndef PDU_METARREQUEST_H
#define PDU_METARREQUEST_H

#include <QString>
#include "pdu_base.h"

class PDUMetarRequest: public PDUBase
{
public:
    PDUMetarRequest(QString from, QString station);

    QString Serialize() override;

    static PDUMetarRequest Parse(QStringList fields);

    QString Station;
};

#endif

#ifndef PDU_METARRESPONSE_H
#define PDU_METARRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUMetarResponse: public PDUBase
{
public:
    PDUMetarResponse(QString to, QString metar);

    QString Serialize() override;

    static PDUMetarResponse Parse(QStringList fields);

    QString Metar;
};

#endif

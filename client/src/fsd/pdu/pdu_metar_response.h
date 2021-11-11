#ifndef PDU_METARRESPONSE_H
#define PDU_METARRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUMetarResponse: public PDUBase
{
public:
    PDUMetarResponse(QString to, QString metar);

    QStringList toTokens() const;

    static PDUMetarResponse fromTokens(const QStringList& fields);

    static QString pdu() { return "$AR"; }

    QString Metar;

private:
    PDUMetarResponse();
};

#endif

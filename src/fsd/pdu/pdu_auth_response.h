#ifndef PDU_AUTHRESPONSE_H
#define PDU_AUTHRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUAuthResponse : public PDUBase
{
public:
    PDUAuthResponse(QString from, QString to, QString response);

    QString Serialize() override;

    static PDUAuthResponse Parse(QStringList fields);

    QString Response;
};

#endif

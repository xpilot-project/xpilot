#ifndef PDU_AUTHRESPONSE_H
#define PDU_AUTHRESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUAuthResponse : public PDUBase
{
public:
    PDUAuthResponse(QString from, QString to, QString response);

    QStringList toTokens() const;

    static PDUAuthResponse fromTokens(const QStringList& fields);

    static QString pdu() { return "$ZR"; }

    QString Response;

private:
    PDUAuthResponse();
};

#endif

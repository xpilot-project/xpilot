#ifndef PDU_PLANEINFOREQ_H
#define PDU_PLANEINFOREQ_H

#include <QString>
#include "pdu_base.h"

class PDUPlaneInfoRequest: public PDUBase
{
public:
    PDUPlaneInfoRequest(QString from, QString to);

    QStringList toTokens() const;

    static PDUPlaneInfoRequest fromTokens(const QStringList& fields);

    static QString pdu() { return "#SB"; }

private:
    PDUPlaneInfoRequest();
};

#endif

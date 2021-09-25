#ifndef PDU_PLANEINFOREQ_H
#define PDU_PLANEINFOREQ_H

#include <QString>
#include "pdu_base.h"

class PDUPlaneInfoRequest: public PDUBase
{
public:
    PDUPlaneInfoRequest(QString from, QString to);

    QString Serialize() override;

    static PDUPlaneInfoRequest Parse(QStringList fields);
};

#endif

#ifndef PDU_WALLOP_H
#define PDU_WALLOP_H

#include <QString>
#include "pdu_base.h"

class PDUWallop: public PDUBase
{
public:
    PDUWallop(QString from, QString message);

    QString Serialize() override;

    static PDUWallop Parse(QStringList fields);

    QString Message;
};

#endif

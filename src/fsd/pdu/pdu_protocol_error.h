#ifndef PDU_PROTOCOL_ERROR_H
#define PDU_PROTOCOL_ERROR_H

#include <QString>
#include <QVector>
#include "pdu_base.h"

class PDUProtocolError: public PDUBase
{
public:
    PDUProtocolError(QString from, QString to, NetworkError type, QString param, QString msg, bool fatal);

    QString Serialize() override;

    static PDUProtocolError Parse(QStringList fields);

    NetworkError ErrorType;
    QString Param;
    QString Message;
    bool Fatal;
};

#endif

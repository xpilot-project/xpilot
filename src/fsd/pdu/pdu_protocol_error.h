#ifndef PDU_PROTOCOL_ERROR_H
#define PDU_PROTOCOL_ERROR_H

#include <QString>
#include <QVector>
#include "pdu_base.h"

class PDUProtocolError: public PDUBase
{
public:
    PDUProtocolError(QString from, QString to, NetworkError type, QString param, QString msg, bool fatal);

    QStringList toTokens() const;

    static PDUProtocolError fromTokens(const QStringList& fields);

    static QString pdu() { return "$ER"; }

    NetworkError ErrorType;
    QString Param;
    QString Message;
    bool Fatal;

private:
    PDUProtocolError();
};

#endif

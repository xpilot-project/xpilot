#ifndef PDU_SEND_FAST_H
#define PDU_SEND_FAST_H

#include <QString>
#include "pdu_base.h"

class PDUSendFast : public PDUBase
{
public:
    PDUSendFast(QString from, QString to, bool sendFast);

    QStringList toTokens() const;

    static PDUSendFast fromTokens(const QStringList& tokens);

    static QString pdu() { return "$SF"; }

    bool DoSendFast;

private:
    PDUSendFast();
};

#endif // PDU_SEND_FAST_H

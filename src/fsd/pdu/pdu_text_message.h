#ifndef PDU_TEXT_MESSAGE_H
#define PDU_TEXT_MESSAGE_H

#include "pdu_base.h"

#include <QString>
#include <QStringList>
#include <QVector>

class PDUTextMessage: public PDUBase
{
public:
    PDUTextMessage(QString from, QString to, QString message);

    QStringList toTokens() const;

    static PDUTextMessage fromTokens(const QStringList& fields);

    static QString pdu() { return "#TM"; }

    QString Message;

private:
    PDUTextMessage();
};

#endif

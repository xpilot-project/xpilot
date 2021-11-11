#ifndef PDU_RADIO_MESSAGE_H
#define PDU_RADIO_MESSAGE_H

#include <QString>
#include <QVector>
#include <QList>
#include "pdu_base.h"

class PDURadioMessage: public PDUBase
{
public:
    PDURadioMessage(QString from, QList<uint> freqs, QString message);

    QStringList toTokens() const;

    static PDURadioMessage fromTokens(const QStringList& fields);

    static QString pdu() { return "#TM"; }

    QList<uint> Frequencies;
    QString Messages;

private:
    PDURadioMessage();
};

#endif

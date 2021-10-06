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

    QString Serialize() override;

    static PDURadioMessage Parse(QStringList fields);

    QList<uint> Frequencies;
    QString Message;
};

#endif

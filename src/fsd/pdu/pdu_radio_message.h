#ifndef PDU_RADIO_MESSAGE_H
#define PDU_RADIO_MESSAGE_H

#include <QString>
#include <QVector>
#include "pdu_base.h"

class PDURadioMessage: public PDUBase
{
public:
    PDURadioMessage(QString from, QVector<int> freqs, QString message);

    QString Serialize() override;

    static PDURadioMessage Parse(QStringList fields);

    QVector<int> Frequencies;
    QString Message;
};

#endif

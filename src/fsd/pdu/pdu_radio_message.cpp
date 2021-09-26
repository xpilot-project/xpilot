#include "pdu_radio_message.h"

PDURadioMessage::PDURadioMessage(QString from, QVector<int> freqs, QString message) :
    PDUBase(from, "")
{
    Frequencies = freqs;
    Message = message;
}

QString PDURadioMessage::Serialize()
{
    QStringList freqs;
    for(auto & freq : Frequencies) {
        if(freqs.length() > 0) {
            freqs.append("&");
        }
        freqs.append("@" + QString::number(freq));
    }

    QStringList tokens;

    tokens.append("#TM");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(freqs);
    tokens.append(Delimeter);
    tokens.append(Message);

    return tokens.join("");
}

PDURadioMessage PDURadioMessage::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    QStringList freqs = fields[1].split("&");
    QVector<int> freqInts;
    for(int i = 0; i < freqs.length(); i++) {
        freqInts[i] = freqs[i].left(1).toInt();
    }

    QStringList msg;
    msg.append(fields[2]);
    for(int i = 3; i < fields.length(); i++) {
        msg.append(":" + fields[i]);
    }

    return PDURadioMessage(fields[0], freqInts, msg.join(""));
}

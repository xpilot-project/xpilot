#include "pdu_radio_message.h"

PDURadioMessage::PDURadioMessage(QString from, QList<uint> freqs, QString message) :
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

    QList<uint> freqInts;
    for(int i = 0; i < freqs.size(); i++) {
        freqInts.push_back(freqs[i].midRef(1, freqs[i].length() - 1).toUInt());
    }

    QStringList msg;
    msg.append(fields[2]);
    for(int i = 3; i < fields.length(); i++) {
        msg.append(":" + fields[i]);
    }

    return PDURadioMessage(fields[0], freqInts, msg.join(""));
}

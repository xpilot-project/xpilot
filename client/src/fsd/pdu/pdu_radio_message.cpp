#include "pdu_radio_message.h"

PDURadioMessage::PDURadioMessage() : PDUBase() {}

PDURadioMessage::PDURadioMessage(QString from, QList<uint> freqs, QString message) :
    PDUBase(from, "")
{
    Frequencies = freqs;
    Messages = message;
}

QStringList PDURadioMessage::toTokens() const
{
    QStringList freqs;
    for(auto & freq : Frequencies) {
        if(freqs.length() > 0) {
            freqs.append("&");
        }
        freqs.append("@" + QString::number(freq));
    }

    QStringList tokens;
    tokens.append(From);
    tokens.append(freqs.join(""));
    tokens.append(Messages);
    return tokens;
}

PDURadioMessage PDURadioMessage::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 3) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    QStringList freqs = tokens[1].split("&");
    QList<uint> freqInts;
    for(int i = 0; i < freqs.size(); i++) {
        freqInts.push_back(freqs[i].mid(1, freqs[i].length() - 1).toUInt());
    }

    QStringList messageTokens = tokens.mid(2);
    return PDURadioMessage(tokens[0], freqInts, messageTokens.join(":"));
}

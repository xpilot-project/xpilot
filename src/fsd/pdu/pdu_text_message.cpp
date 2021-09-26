#include "pdu_text_message.h"

PDUTextMessage::PDUTextMessage(QString from, QString to, QString message) :
    PDUBase(from, to)
{
    Message = message;
}

QString PDUTextMessage::Serialize()
{
    QStringList tokens;

    tokens.append("#TM");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Message);

    return tokens.join(Delimeter);
}

PDUTextMessage PDUTextMessage::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    QStringList msg;
    msg.append(fields[2]);
    for(int i = 3; i < fields.length(); i++) {
        msg.append(":" + fields[i]);
    }

    return PDUTextMessage(fields[0], fields[1], msg.join(""));
}

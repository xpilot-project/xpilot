#include "pdu_broadcast_message.h"

PDUBroadcastMessage::PDUBroadcastMessage(QString from, QString message) :
    PDUBase(from, "*")
{
    Message = message;
}

QString PDUBroadcastMessage::Serialize()
{
    QStringList tokens;

    tokens.append("#TM");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Message);

    return tokens.join(Delimeter);
}

PDUBroadcastMessage PDUBroadcastMessage::Parse(QStringList fields)
{
    if(fields.length() < 3) {
        qDebug() << u"Invalid field count: " << Reassemble(fields);
    }

    QStringList msg;
    msg.append(fields[2]);
    for(int i = 3; i < fields.length(); i++) {
        msg.append(":" + fields[i]);
    }

    return PDUBroadcastMessage(fields[0], msg.join(""));
}

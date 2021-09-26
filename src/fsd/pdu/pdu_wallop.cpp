#include "pdu_wallop.h"

PDUWallop::PDUWallop(QString from, QString message) :
    PDUBase(from, "*S")
{
    Message = message;
}

QString PDUWallop::Serialize()
{
    QStringList tokens;

    tokens.append("#TM");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Message);

    return tokens.join("");
}

PDUWallop PDUWallop::Parse(QStringList fields)
{
    if(fields.length() > 3) {

    }

    QStringList msg;
    msg.append(fields[2]);
    for(int i = 3; i < fields.length(); i++) {
        msg.append(":" + fields[i]);
    }

    return PDUWallop(fields[0], msg.join(""));
}

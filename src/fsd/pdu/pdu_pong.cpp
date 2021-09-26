#include "pdu_pong.h"

PDUPong::PDUPong(QString from, QString to, QString timeStamp) :
    PDUBase(from, to)
{
    Timestamp = timeStamp;
}

QString PDUPong::Serialize()
{
    QStringList tokens;

    tokens.append("$PO");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(Timestamp);

    return tokens.join("");
}

PDUPong PDUPong::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    return PDUPong(fields[0], fields[1], fields[2]);
}

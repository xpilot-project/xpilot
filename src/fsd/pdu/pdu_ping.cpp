#include "pdu_ping.h"

PDUPing::PDUPing(QString from, QString to, QString timeStamp) :
    PDUBase(from, to)
{
    Timestamp = timeStamp;
}

QString PDUPing::Serialize()
{
    QStringList tokens;

    tokens.append("$PI");
    tokens.append(From);
    tokens.append(To);
    tokens.append(Timestamp);

    return tokens.join(Delimeter);
}

PDUPing PDUPing::Parse(QStringList fields)
{
    if(fields.length() < 3) {

    }

    return PDUPing(fields[0], fields[1], fields[2]);
}

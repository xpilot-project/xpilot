#include "pdu_delete_atc.h"

PDUDeleteATC::PDUDeleteATC(QString from, QString cid) : PDUBase(from, "")
{
    CID = cid;
}

QString PDUDeleteATC::Serialize()
{
    QStringList tokens;

    tokens.append("#DA");
    tokens.append(From);
    tokens.append(CID);

    return tokens.join(Delimeter);
}

PDUDeleteATC PDUDeleteATC::Parse(QStringList fields)
{
    if(fields.length() < 1) {

    }

    return PDUDeleteATC(fields[0], fields.length() >= 2 ? fields[1] : "");
}

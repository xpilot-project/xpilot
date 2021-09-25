#include "pdu_delete_pilot.h"

PDUDeletePilot::PDUDeletePilot(QString from, QString cid) :
    PDUBase(from, "")
{
    CID = cid;
}

QString PDUDeletePilot::Serialize()
{
    QStringList tokens;

    tokens.append("#DP");
    tokens.append(From);
    tokens.append(CID);

    return tokens.join(Delimeter);
}

PDUDeletePilot PDUDeletePilot::Parse(QStringList fields)
{
    if(fields.length() < 1) {

    }

    return PDUDeletePilot(fields[0], fields.length() >= 2 ? fields[1] : "");
}

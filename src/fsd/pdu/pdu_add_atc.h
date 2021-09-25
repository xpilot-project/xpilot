#ifndef PDU_ADDATC_H
#define PDU_ADDATC_H

#include <QString>
#include "pdu_base.h"

class PDUAddATC : public PDUBase
{
public:
    PDUAddATC(QString callsign, QString realName, QString cid, QString password, NetworkRating rating, ProtocolRevision proto);

    QString Serialize() override;

    static PDUAddATC Parse(QStringList fields);

    QString RealName;
    QString CID;
    QString Password;
    NetworkRating Rating;
    ProtocolRevision Protocol;
};

#endif

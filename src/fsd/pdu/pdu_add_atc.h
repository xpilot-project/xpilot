#ifndef PDU_ADDATC_H
#define PDU_ADDATC_H

#include "pdu_base.h"

#include <QString>
#include <QStringList>
#include <QVector>

class PDUAddATC : public PDUBase
{
public:
    PDUAddATC(QString callsign, QString realName, QString cid, QString password, NetworkRating rating, ProtocolRevision proto);

    QStringList toTokens() const;

    static PDUAddATC fromTokens(const QStringList& fields);

    static QString pdu() { return "#AA"; }

    QString RealName;
    QString CID;
    QString Password;
    NetworkRating Rating;
    ProtocolRevision Protocol;

private:
    PDUAddATC();
};

#endif

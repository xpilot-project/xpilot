#ifndef PDU_ADDPILOT_H
#define PDU_ADDPILOT_H

#include <QString>
#include "pdu_base.h"

class PDUAddPilot : public PDUBase
{
public:
    PDUAddPilot(QString callsign, QString cid, QString password, NetworkRating rating, ProtocolRevision proto, SimulatorType simType, QString realName);

    QStringList toTokens() const;

    static PDUAddPilot fromTokens(const QStringList& fields);

    static QString pdu() { return "#AP"; }

    QString CID;
    QString Password;
    NetworkRating Rating;
    ProtocolRevision Protocol;
    SimulatorType SimType;
    QString RealName;

private:
    PDUAddPilot();
};

#endif

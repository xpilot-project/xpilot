#ifndef PDU_ATCPOSITION_H
#define PDU_ATCPOSITION_H

#include <QString>
#include <QList>
#include "pdu_base.h"

class PDUATCPosition : public PDUBase
{
public:
    PDUATCPosition(QString from, QList<int> freqs, NetworkFacility facility, int visRange, NetworkRating rating, double lat, double lon);

    QStringList toTokens() const;

    static PDUATCPosition fromTokens(const QStringList& fields);

    static QString pdu() { return "%"; }

    QList<int> Frequencies;
    NetworkFacility Facility;
    int VisibilityRange;
    NetworkRating Rating;
    double Lat;
    double Lon;

private:
    PDUATCPosition();
};

#endif

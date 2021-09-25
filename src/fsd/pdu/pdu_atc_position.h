#ifndef PDU_ATCPOSITION_H
#define PDU_ATCPOSITION_H

#include <QString>
#include "pdu_base.h"

class PDUATCPosition : public PDUBase
{
public:
    PDUATCPosition(QString from, int freq, NetworkFacility facility, int visRange, NetworkRating rating, double lat, double lon);

    QString Serialize() override;

    static PDUATCPosition Parse(QStringList fields);

    int Frequency;
    NetworkFacility Facility;
    int VisibilityRange;
    NetworkRating Rating;
    double Lat;
    double Lon;
};

#endif

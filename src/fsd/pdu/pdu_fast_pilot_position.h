#ifndef PDU_FASTPILOTPOSITION_H
#define PDU_FASTPILOTPOSITION_H

#include <QString>
#include "pdu_base.h"

class PDUFastPilotPosition : public PDUBase
{
public:
    PDUFastPilotPosition(QString from, double lat, double lon, double alt, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank);

    QString Serialize() override;

    static PDUFastPilotPosition Parse(QStringList fields);

    double Lat;
    double Lon;
    double Altitude;
    double Pitch;
    double Heading;
    double Bank;
    double VelocityLongitude;
    double VelocityAltitude;
    double VelocityLatitude;
    double VelocityPitch;
    double VelocityHeading;
    double VelocityBank;
};

#endif

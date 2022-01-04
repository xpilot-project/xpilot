#ifndef PDU_FASTPILOTPOSITION_H
#define PDU_FASTPILOTPOSITION_H

#include <QString>
#include "pdu_base.h"

class PDUFastPilotPosition : public PDUBase
{
public:
    PDUFastPilotPosition(QString from, double lat, double lon, double alt, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank, double noseWheelAngle);

    QStringList toTokens() const;

    static PDUFastPilotPosition fromTokens(const QStringList& fields);

    static QString pdu() { return "^"; }

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
    double NoseWheelAngle;

private:
    PDUFastPilotPosition();
};

#endif

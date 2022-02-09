#ifndef PDU_FASTPILOTPOSITION_H
#define PDU_FASTPILOTPOSITION_H

#include <QString>
#include "pdu_base.h"

enum class FastPilotPositionType
{
    Fast,
    Slow,
    Stopped
};

class PDUFastPilotPosition : public PDUBase
{
public:
    PDUFastPilotPosition(FastPilotPositionType type, QString from, double lat, double lon, double altTrue, double altAgl, double pitch, double heading, double bank, double velocityLongitude, double velocityAltitude, double velocityLatitude, double velocityPitch, double velocityHeading, double velocityBank, double noseGearAngle);

    QStringList toTokens() const;

    static PDUFastPilotPosition fromTokens(FastPilotPositionType type, const QStringList& fields);

    QString pdu() const
    {
        switch(Type) {
        case FastPilotPositionType::Fast:
            return "^";
        case FastPilotPositionType::Slow:
            return "#SL";
        case FastPilotPositionType::Stopped:
            return "#ST";
        default:
            return "^";
        }
    }

    FastPilotPositionType Type;
    double Lat;
    double Lon;
    double AltitudeTrue;
    double AltitudeAgl;
    double Pitch;
    double Heading;
    double Bank;
    double VelocityLongitude;
    double VelocityAltitude;
    double VelocityLatitude;
    double VelocityPitch;
    double VelocityHeading;
    double VelocityBank;
    double NoseGearAngle;

private:
    PDUFastPilotPosition();
};

#endif

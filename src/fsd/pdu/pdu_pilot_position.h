#ifndef PDU_PILOTPOS_H
#define PDU_PILOTPOS_H

#include <QString>
#include "pdu_base.h"

class PDUPilotPosition: public PDUBase
{
public:
    PDUPilotPosition(QString from, int txCode, bool squawkingModeC, bool identing, NetworkRating rating, double lat, double lon, int trueAlt, int pressureAlt, int gs, double pitch, double heading, double bank);

    QStringList toTokens() const;

    static PDUPilotPosition fromTokens(const QStringList& fields);

    static QString pdu() { return "@"; }

    int SquawkCode;
    bool SquawkingModeC;
    bool Identing;
    NetworkRating Rating;
    double Lat;
    double Lon;
    int TrueAltitude;
    int PressureAltitude;
    int GroundSpeed;
    double Pitch;
    double Heading;
    double Bank;

private:
    PDUPilotPosition();
};

#endif

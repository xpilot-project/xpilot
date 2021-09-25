#ifndef PDU_FIGHTPLAN_H
#define PDU_FIGHTPLAN_H

#include <QString>
#include "pdu_base.h"

class PDUFlightPlan : public PDUBase
{
public:
    PDUFlightPlan(QString from, QString to, FlightRules rules, QString equipment, QString tas,
                  QString depAirport, QString estimatedDepTime, QString actualDepTime, QString cruiseAlt,
                  QString destAirport, QString hoursEnroute, QString minutesEnroute, QString fuelAvailHours,
                  QString fuelAvailMinutes, QString altAirport, QString remarks, QString route);

    QString Serialize() override;

    static PDUFlightPlan Parse(QStringList fields);

    FlightRules Rules;
    QString Equipment;
    QString TAS;
    QString DepAirport;
    QString EstimatedDepTime;
    QString ActualDepTime;
    QString CruiseAlt;
    QString DestAirport;
    QString HoursEnroute;
    QString MinutesEnroute;
    QString FuelAvailHours;
    QString FuelAvailMinutes;
    QString AltAirport;
    QString Remarks;
    QString Route;
};

#endif

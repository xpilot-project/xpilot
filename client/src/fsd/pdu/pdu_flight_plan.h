#ifndef PDU_FLIGHT_PLAN_H
#define PDU_FLIGHT_PLAN_H

#include <QString>
#include "pdu_base.h"

class PDUFlightPlan: public PDUBase
{
public:
    PDUFlightPlan(QString from, QString to, FlightRules rules, QString equipment, QString tas,
                  QString depAirport, QString estimatedDepTime, QString actualDepTime, QString cruiseAlt,
                  QString destAirport, QString hoursEnroute, QString minutesEnroute, QString fuelAvailHours,
                  QString fuelAvailMinutes, QString altAirport, QString remarks, QString route);

    QStringList toTokens() const;

    static PDUFlightPlan fromTokens(const QStringList& tokens);

    static QString pdu() { return "$FP"; }

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

private:
    PDUFlightPlan();
};
#endif // PDU_FLIGHT_PLAN_H

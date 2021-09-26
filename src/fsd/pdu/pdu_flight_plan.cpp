#include "pdu_flight_plan.h"

PDUFlightPlan::PDUFlightPlan(QString from, QString to, FlightRules rules, QString equipment, QString tas, QString depAirport, QString estimatedDepTime, QString actualDepTime, QString cruiseAlt, QString destAirport, QString hoursEnroute, QString minutesEnroute, QString fuelAvailHours, QString fuelAvailMinutes, QString altAirport, QString remarks, QString route) : PDUBase(from, to)
{
    Rules = rules;
    Equipment = equipment;
    TAS = tas;
    DepAirport = depAirport;
    EstimatedDepTime = estimatedDepTime;
    ActualDepTime = actualDepTime;
    CruiseAlt = cruiseAlt;
    DestAirport = destAirport;
    HoursEnroute = hoursEnroute;
    MinutesEnroute = minutesEnroute;
    FuelAvailHours = fuelAvailHours;
    FuelAvailMinutes = fuelAvailMinutes;
    AltAirport = altAirport;
    Remarks = remarks.replace(":", " ");
    Route = route.replace(":", " ");
}

QString PDUFlightPlan::Serialize()
{
    QStringList tokens;

    tokens.append("$FP");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(toQString(Rules).mid(0, 1));
    tokens.append(Delimeter);
    tokens.append(Equipment);
    tokens.append(Delimeter);
    tokens.append(TAS);
    tokens.append(Delimeter);
    tokens.append(DepAirport);
    tokens.append(Delimeter);
    tokens.append(EstimatedDepTime);
    tokens.append(Delimeter);
    tokens.append(ActualDepTime);
    tokens.append(Delimeter);
    tokens.append(CruiseAlt);
    tokens.append(Delimeter);
    tokens.append(DestAirport);
    tokens.append(Delimeter);
    tokens.append(HoursEnroute);
    tokens.append(Delimeter);
    tokens.append(MinutesEnroute);
    tokens.append(Delimeter);
    tokens.append(FuelAvailHours);
    tokens.append(Delimeter);
    tokens.append(FuelAvailMinutes);
    tokens.append(Delimeter);
    tokens.append(AltAirport);
    tokens.append(Delimeter);
    tokens.append(Remarks);
    tokens.append(Delimeter);
    tokens.append(Route);

    return tokens.join("");
}

PDUFlightPlan PDUFlightPlan::Parse(QStringList fields)
{
    if(fields.length() < 17) {

    }

    return PDUFlightPlan(fields[0], fields[1], fromQString<FlightRules>(fields[2]), fields[3],
            fields[4], fields[5], fields[6], fields[7], fields[8], fields[9], fields[10], fields[11],
            fields[12], fields[13], fields[14], fields[15], fields[16]);
}

#include "pdu_flight_plan.h"

PDUFlightPlan::PDUFlightPlan() : PDUBase() {}

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

QStringList PDUFlightPlan::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(toQString(Rules).mid(0, 1));
    tokens.append(Equipment);
    tokens.append(TAS);
    tokens.append(DepAirport);
    tokens.append(EstimatedDepTime);
    tokens.append(ActualDepTime);
    tokens.append(CruiseAlt);
    tokens.append(DestAirport);
    tokens.append(HoursEnroute);
    tokens.append(MinutesEnroute);
    tokens.append(FuelAvailHours);
    tokens.append(FuelAvailMinutes);
    tokens.append(AltAirport);
    tokens.append(Remarks);
    tokens.append(Route);
    return tokens;
}

PDUFlightPlan PDUFlightPlan::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 17) {
        return {};
    }

    return PDUFlightPlan(tokens[0], tokens[1], fromQString<FlightRules>(tokens[2]), tokens[3], tokens[4],
            tokens[5], tokens[6], tokens[7], tokens[8], tokens[9], tokens[10], tokens[11],
            tokens[12], tokens[13], tokens[14], tokens[15], tokens[16]);
}


#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <QObject>

namespace xpilot
{
    struct FlightPlan
    {
        Q_GADGET

        Q_PROPERTY(QString FlightRules MEMBER FlightRules)
        Q_PROPERTY(QString Equipment MEMBER Equipment)
        Q_PROPERTY(int TAS MEMBER TAS)
        Q_PROPERTY(QString DepAirport MEMBER DepAirport)
        Q_PROPERTY(QString EstimatedDepTime MEMBER EstimatedDepTime)
        Q_PROPERTY(int CruiseAlt MEMBER CruiseAlt)
        Q_PROPERTY(QString DestAirport MEMBER DestAirport)
        Q_PROPERTY(int HoursEnroute MEMBER HoursEnroute)
        Q_PROPERTY(int MinutesEnroute MEMBER MinutesEnroute)
        Q_PROPERTY(int FuelAvailHours MEMBER FuelAvailHours)
        Q_PROPERTY(int FuelAvailMinutes MEMBER FuelAvailMinutes)
        Q_PROPERTY(QString AltAirport MEMBER AltAirport)
        Q_PROPERTY(QString Remarks MEMBER Remarks)
        Q_PROPERTY(QString Route MEMBER Route)
        Q_PROPERTY(QString VoiceCapability MEMBER VoiceCapability)

    public:
        QString FlightRules;
        QString Equipment;
        int TAS;
        QString DepAirport;
        QString EstimatedDepTime;
        int CruiseAlt;
        QString DestAirport;
        int HoursEnroute;
        int MinutesEnroute;
        int FuelAvailHours;
        int FuelAvailMinutes;
        QString AltAirport;
        QString Remarks;
        QString Route;
        QString VoiceCapability;

        bool operator==(FlightPlan& rhs) const
        {
            return FlightRules == rhs.FlightRules && Equipment == rhs.Equipment && TAS == rhs.TAS
                    && DepAirport == rhs.DepAirport && EstimatedDepTime == rhs.EstimatedDepTime
                    && CruiseAlt == rhs.CruiseAlt && DestAirport == rhs.DepAirport && HoursEnroute == rhs.HoursEnroute
                    && MinutesEnroute == rhs.MinutesEnroute && FuelAvailHours == rhs.FuelAvailHours
                    && FuelAvailMinutes == rhs.FuelAvailMinutes && AltAirport == rhs.AltAirport
                    && Remarks == rhs.Remarks && Route == rhs.Route && VoiceCapability == rhs.VoiceCapability;
        }

        bool operator!=(FlightPlan& rhs) const
        {
            return FlightRules != rhs.FlightRules || Equipment != rhs.Equipment || TAS != rhs.TAS
                    || DepAirport != rhs.DepAirport || EstimatedDepTime != rhs.EstimatedDepTime
                    || CruiseAlt != rhs.CruiseAlt || DestAirport != rhs.DepAirport || HoursEnroute != rhs.HoursEnroute
                    || MinutesEnroute != rhs.MinutesEnroute || FuelAvailHours != rhs.FuelAvailHours
                    || FuelAvailMinutes != rhs.FuelAvailMinutes || AltAirport != rhs.AltAirport
                    || Remarks != rhs.Remarks || Route != rhs.Route || VoiceCapability != rhs.VoiceCapability;
        }
    };
}

Q_DECLARE_METATYPE(xpilot::FlightPlan)

#endif // FLIGHTPLAN_H

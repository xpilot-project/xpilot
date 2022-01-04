#ifndef AIRCRAFT_VISUAL_STATE_H
#define AIRCRAFT_VISUAL_STATE_H

struct AircraftVisualState
{
    double Latitude;
    double Longitude;
    double Altitude;
    double Pitch;
    double Heading;
    double Bank;
    double NoseWheelAngle;

    bool operator==(const AircraftVisualState& rhs) const
    {
         return Latitude == rhs.Latitude && Longitude == rhs.Longitude && Altitude == rhs.Altitude
                 && Pitch == rhs.Pitch && Heading == rhs.Heading && Bank == rhs.Bank && NoseWheelAngle == rhs.NoseWheelAngle;
    }

    bool operator!=(const AircraftVisualState& rhs) const
    {
        return Latitude != rhs.Latitude || Longitude != rhs.Longitude || Altitude != rhs.Altitude
                || Pitch != rhs.Pitch || Heading != rhs.Heading || Bank != rhs.Bank || NoseWheelAngle != rhs.NoseWheelAngle;
    }
};

#endif

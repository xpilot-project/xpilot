#pragma once

#include <QObject>

struct UserAircraftData
{
    Q_GADGET

public:
    double Latitude;
    double Longitude;
    double AltitudeMslM;
    double AltitudeAglM;
    double AltitudePressure;
    double BarometerSeaLevel;
    double GroundSpeed;
    double Pitch;
    double Heading;
    double Bank;
    double LatitudeVelocity;
    double AltitudeVelocity;
    double LongitudeVelocity;
    double PitchVelocity;
    double HeadingVelocity;
    double BankVelocity;
    double NoseWheelAngle;

    bool operator!=(const UserAircraftData& other) const
    {
        return Latitude != other.Latitude ||
                Longitude != other.Longitude ||
                AltitudeMslM != other.AltitudeMslM ||
                AltitudeAglM != other.AltitudeAglM ||
                AltitudePressure != other.AltitudePressure ||
                BarometerSeaLevel != other.BarometerSeaLevel ||
                GroundSpeed != other.GroundSpeed ||
                Pitch != other.Pitch ||
                Heading != other.Heading ||
                Bank != other.Bank ||
                LatitudeVelocity != other.LatitudeVelocity ||
                AltitudeVelocity != other.AltitudeVelocity ||
                LongitudeVelocity != other.LongitudeVelocity ||
                PitchVelocity != other.PitchVelocity ||
                HeadingVelocity != other.HeadingVelocity ||
                BankVelocity != other.BankVelocity ||
                NoseWheelAngle != other.NoseWheelAngle;
    }

    bool operator==(const UserAircraftData& other) const
    {
        return Latitude == other.Latitude &&
                Longitude == other.Longitude &&
                AltitudeMslM == other.AltitudeMslM &&
                AltitudeAglM == other.AltitudeAglM &&
                AltitudePressure == other.AltitudePressure &&
                BarometerSeaLevel == other.BarometerSeaLevel &&
                GroundSpeed == other.GroundSpeed &&
                Pitch == other.Pitch &&
                Heading == other.Heading &&
                Bank == other.Bank &&
                LatitudeVelocity == other.LatitudeVelocity &&
                AltitudeVelocity == other.AltitudeVelocity &&
                LongitudeVelocity == other.LongitudeVelocity &&
                PitchVelocity == other.PitchVelocity &&
                HeadingVelocity == other.HeadingVelocity &&
                BankVelocity == other.BankVelocity &&
                NoseWheelAngle == other.NoseWheelAngle;
    }
};

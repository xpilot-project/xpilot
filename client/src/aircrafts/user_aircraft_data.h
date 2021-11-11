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

    bool operator!=(const UserAircraftData& other) const
    {
        return Latitude != other.Latitude ||
                Longitude != other.Longitude ||
                AltitudeMslM != other.AltitudeMslM ||
                AltitudeAglM != other.AltitudeAglM ||
                GroundSpeed != other.GroundSpeed ||
                Pitch != other.Pitch ||
                Heading != other.Heading ||
                Bank != other.Bank ||
                LatitudeVelocity != other.LatitudeVelocity ||
                AltitudeVelocity != other.AltitudeVelocity ||
                LongitudeVelocity != other.LongitudeVelocity ||
                PitchVelocity != other.PitchVelocity ||
                HeadingVelocity != other.HeadingVelocity ||
                BankVelocity != other.BankVelocity;
    }

    bool operator==(const UserAircraftData& other) const
    {
        return Latitude == other.Latitude &&
                Longitude == other.Longitude &&
                AltitudeMslM == other.AltitudeMslM &&
                AltitudeAglM == other.AltitudeAglM &&
                GroundSpeed == other.GroundSpeed &&
                Pitch == other.Pitch &&
                Heading == other.Heading &&
                Bank == other.Bank &&
                LatitudeVelocity == other.LatitudeVelocity &&
                AltitudeVelocity == other.AltitudeVelocity &&
                LongitudeVelocity == other.LongitudeVelocity &&
                PitchVelocity == other.PitchVelocity &&
                HeadingVelocity == other.HeadingVelocity &&
                BankVelocity == other.BankVelocity;
    }
};

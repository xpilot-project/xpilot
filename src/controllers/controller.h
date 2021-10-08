#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QtGlobal>
#include <QString>
#include <QObject>

struct Controller
{
    Q_GADGET

    Q_PROPERTY(QString Callsign MEMBER Callsign)
    Q_PROPERTY(uint Frequency MEMBER Frequency)
    Q_PROPERTY(uint NormalizedFrequency MEMBER NormalizedFrequency)
    Q_PROPERTY(double Latitude MEMBER Latitude)
    Q_PROPERTY(double Longitude MEMBER Longitude)
    Q_PROPERTY(QString RealName MEMBER RealName)

public:
    QString Callsign;
    uint Frequency;
    uint NormalizedFrequency;
    double Latitude;
    double Longitude;
    qint64 LastUpdate;
    QString RealName;
    bool IsValid;
    bool IsValidATC;

    bool operator==(const Controller& rhs)
    {
      return Callsign == rhs.Callsign
              && Frequency == rhs.Frequency
              && NormalizedFrequency == rhs.NormalizedFrequency
              && Latitude == rhs.Latitude
              && Longitude == rhs.Longitude
              && LastUpdate == rhs.LastUpdate
              && RealName == rhs.RealName
              && IsValid == rhs.IsValid
              && IsValidATC == rhs.IsValidATC;
    }

    bool operator!=(const Controller& rhs)
    {
        return Callsign != rhs.Callsign
                || Frequency != rhs.Frequency
                || NormalizedFrequency != rhs.NormalizedFrequency
                || Latitude != rhs.Latitude
                || Longitude != rhs.Longitude
                || LastUpdate != rhs.LastUpdate
                || RealName != rhs.RealName
                || IsValid != rhs.IsValid
                || IsValidATC != rhs.IsValidATC;
    }
};

Q_DECLARE_METATYPE(Controller)

#endif

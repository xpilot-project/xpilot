#include "pdu_atc_position.h"

PDUATCPosition::PDUATCPosition(QString from, qint32 freq, NetworkFacility facility, qint32 visRange, NetworkRating rating, double lat, double lon)
    : PDUBase(from, "")
{
    From = from;
    Frequency = freq;
    Facility = facility;
    VisibilityRange = visRange;
    Rating = rating;
    Lat = lat;
    Lon = lon;
}

QString PDUATCPosition::Serialize()
{
    QStringList tokens;

    tokens.append("%");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(QString::number(Frequency));
    tokens.append(Delimeter);
    tokens.append(toQString(Facility));
    tokens.append(Delimeter);
    tokens.append(QString::number(VisibilityRange));
    tokens.append(Delimeter);
    tokens.append(toQString(Rating));
    tokens.append(Delimeter);
    tokens.append(QString::number(Lat, 'f', 5));
    tokens.append(Delimeter);
    tokens.append(QString::number(Lon, 'f', 5));
    tokens.append(Delimeter);
    tokens.append(QString::number(0));

    return tokens.join("");
}

PDUATCPosition PDUATCPosition::Parse(QStringList fields)
{
    if(fields.length() < 7) {
        qDebug() << u"Invalid field count: " << Reassemble(fields);
    }

    return PDUATCPosition(fields[0], fields[1].toInt() + 100000, fromQString<NetworkFacility>(fields[2]),
            fields[3].toInt(), fromQString<NetworkRating>(fields[4]), fields[5].toDouble(), fields[6].toDouble());
}

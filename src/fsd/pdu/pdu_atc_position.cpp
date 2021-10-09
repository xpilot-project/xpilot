#include "pdu_atc_position.h"

PDUATCPosition::PDUATCPosition() : PDUBase() {}

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

QStringList PDUATCPosition::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(QString::number(Frequency));
    tokens.append(toQString(Facility));
    tokens.append(QString::number(VisibilityRange));
    tokens.append(toQString(Rating));
    tokens.append(QString::number(Lat, 'f', 5));
    tokens.append(QString::number(Lon, 'f', 5));
    tokens.append(QString::number(0));
    return tokens;
}

PDUATCPosition PDUATCPosition::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 7) {
        return {};
    }

    return PDUATCPosition(tokens[0], tokens[1].toInt() + 100000, fromQString<NetworkFacility>(tokens[2]),
            tokens[3].toInt(), fromQString<NetworkRating>(tokens[4]), tokens[5].toDouble(), tokens[6].toDouble());
}

#include "pdu_atc_position.h"

PDUATCPosition::PDUATCPosition() : PDUBase() {}

PDUATCPosition::PDUATCPosition(QString from, QList<int> freqs, NetworkFacility facility, qint32 visRange, NetworkRating rating, double lat, double lon)
    : PDUBase(from, "")
{
    From = from;
    Frequencies = freqs;
    Facility = facility;
    VisibilityRange = visRange;
    Rating = rating;
    Lat = lat;
    Lon = lon;
}

QStringList PDUATCPosition::toTokens() const
{
    QStringList freqs;
    for(auto &freq : Frequencies) {
        freqs.append(QString::number(freq));
    }

    QStringList tokens;
    tokens.append(From);
    tokens.append(freqs.join("&"));
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
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    auto freqs = tokens[1].split("&", Qt::SkipEmptyParts);
    QList<int> freqInts;
    for(int i = 0; i < freqs.length(); i++) {
        freqInts.push_back(freqs[i].toInt() + 100000);
    }

    return PDUATCPosition(tokens[0], freqInts, fromQString<NetworkFacility>(tokens[2]),
            tokens[3].toInt(), fromQString<NetworkRating>(tokens[4]), tokens[5].toDouble(), tokens[6].toDouble());
}

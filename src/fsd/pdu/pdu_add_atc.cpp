#include "pdu_add_atc.h"

PDUAddATC::PDUAddATC(QString callsign, QString realName, QString cid, QString password, NetworkRating rating, ProtocolRevision proto)
    : PDUBase(callsign, "")
{
    RealName = realName;
    CID = cid;
    Password = password;
    Rating = rating;
    Protocol = proto;
}

QString PDUAddATC::Serialize()
{
    QStringList tokens;

    tokens.append("#AA");
    tokens.append(From);
    tokens.append(PDUBase::ServerCallsign);
    tokens.append(RealName);
    tokens.append(CID);
    tokens.append(Password);
    tokens.append(toQString(Rating));
    tokens.append(toQString(Protocol));

    return tokens.join(Delimeter);
}

PDUAddATC PDUAddATC::Parse(QStringList fields)
{
    if(fields.length() < 6) {

    }

    return PDUAddATC(fields[0], fields[2], fields[3], fields[4], fromQString<NetworkRating>(fields[5]), fromQString<ProtocolRevision>(fields[6]));
}

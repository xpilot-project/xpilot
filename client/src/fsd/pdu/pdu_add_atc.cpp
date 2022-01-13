#include "pdu_add_atc.h"

PDUAddATC::PDUAddATC() : PDUBase() {}

PDUAddATC::PDUAddATC(QString callsign, QString realName, QString cid, QString password, NetworkRating rating, ProtocolRevision proto) : PDUBase(callsign, "")
{
    RealName = realName;
    CID = cid;
    Password = password;
    Rating = rating;
    Protocol = proto;
}

QStringList PDUAddATC::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(PDUBase::ServerCallsign);
    tokens.append(RealName);
    tokens.append(CID);
    tokens.append(Password);
    tokens.append(toQString(Rating));
    tokens.append(toQString(Protocol));
    return tokens;
}

PDUAddATC PDUAddATC::fromTokens(const QStringList &tokens)
{
    if(tokens.size() < 6) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUAddATC(tokens[0], tokens[2], tokens[3], tokens[4], fromQString<NetworkRating>(tokens[5]), fromQString<ProtocolRevision>(tokens[6]));
}

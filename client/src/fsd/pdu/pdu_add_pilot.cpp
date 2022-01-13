#include "pdu_add_pilot.h"

PDUAddPilot::PDUAddPilot() : PDUBase() {}

PDUAddPilot::PDUAddPilot(QString callsign, QString cid, QString password, NetworkRating rating, ProtocolRevision proto, SimulatorType simType, QString realName) :
    PDUBase(callsign, "")
{
    CID = cid;
    Password = password;
    Rating = rating;
    Protocol = proto;
    SimType = simType;
    RealName = realName;
}

QStringList PDUAddPilot::toTokens() const
{
    QStringList tokens;

    tokens.append(From);
    tokens.append(PDUBase::ServerCallsign);
    tokens.append(CID);
    tokens.append(Password);
    tokens.append(toQString(Rating));
    tokens.append(toQString(Protocol));
    tokens.append(toQString(SimType));
    tokens.append(RealName);

    return tokens;
}

PDUAddPilot PDUAddPilot::fromTokens(const QStringList &tokens)
{
    if(tokens.size() < 8) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUAddPilot(tokens[0], tokens[2], tokens[3], fromQString<NetworkRating>(tokens[4]),
            fromQString<ProtocolRevision>(tokens[5]), fromQString<SimulatorType>(tokens[6]), tokens[7]);
}

#include "pdu_change_server.h"

PDUChangeServer::PDUChangeServer() : PDUBase() {}

PDUChangeServer::PDUChangeServer(QString from, QString to, QString newServer) :
    PDUBase(from, to)
{
    NewServer = newServer;
}

QStringList PDUChangeServer::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(NewServer);
    return tokens;
}

PDUChangeServer PDUChangeServer::fromTokens(const QStringList &fields)
{
    if(fields.size() < 6) {
        throw PDUFormatException("Invalid field count.", Reassemble(fields));
    }

    return PDUChangeServer(fields[0], fields[1], fields[2]);
}

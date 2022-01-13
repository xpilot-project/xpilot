#include "pdu_plane_info_response.h"

PDUPlaneInfoResponse::PDUPlaneInfoResponse() : PDUBase() {}

PDUPlaneInfoResponse::PDUPlaneInfoResponse(QString from, QString to, QString equipment, QString airline, QString livery, QString csl) :
    PDUBase(from, to)
{
    Equipment = equipment;
    Airline = airline;
    Livery = livery;
    CSL = csl;
}

QStringList PDUPlaneInfoResponse::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append("PI");
    tokens.append("GEN");
    tokens.append("EQUIPMENT=" + Equipment);
    if(!Airline.isEmpty()) {
        tokens.append("AIRLINE=" + Airline);
    }
    if(!Livery.isEmpty()) {
        tokens.append("LIVERY=" + Livery);
    }
    if(!CSL.isEmpty()) {
        tokens.append("CSL=" + CSL);
    }
    return tokens;
}

PDUPlaneInfoResponse PDUPlaneInfoResponse::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 5) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    return PDUPlaneInfoResponse(tokens[0], tokens[1], FindValue(tokens, "EQUIPMENT"),
            FindValue(tokens, "AIRLINE"), FindValue(tokens, "LIVERY"), FindValue(tokens, "CSL"));
}

#include "pdu_plane_info_response.h"

PDUPlaneInfoResponse::PDUPlaneInfoResponse(QString from, QString to, QString equipment, QString airline, QString livery, QString csl) :
    PDUBase(from, to)
{
    Equipment = equipment;
    Airline = airline;
    Livery = livery;
    CSL = csl;
}

QString PDUPlaneInfoResponse::Serialize()
{
    QStringList tokens;

    tokens.append("#SB");
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

    return tokens.join(Delimeter);
}

PDUPlaneInfoResponse PDUPlaneInfoResponse::Parse(QStringList fields)
{
    if(fields.length() < 5) {

    }

    return PDUPlaneInfoResponse(fields[0], fields[1], FindValue(fields, "EQUIPMENT"), FindValue(fields, "AIRLINE"), FindValue(fields, "LIVERY"), FindValue(fields, "CSL"));
}

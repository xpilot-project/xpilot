#include "pdu_protocol_error.h"

PDUProtocolError::PDUProtocolError(QString from, QString to, NetworkError type, QString param, QString msg, bool fatal) :
    PDUBase(from, to)
{
    ErrorType = type;
    Param = param;
    Message = msg;
    Fatal = fatal;
}

QString PDUProtocolError::Serialize()
{
    QStringList tokens;

    tokens.append("$ER");
    tokens.append(From);
    tokens.append(Delimeter);
    tokens.append(To);
    tokens.append(Delimeter);
    tokens.append(QString::number(static_cast<int>(ErrorType)));
    tokens.append(Delimeter);
    tokens.append(Param);
    tokens.append(Delimeter);
    tokens.append(Message);

    return tokens.join("");
}

PDUProtocolError PDUProtocolError::Parse(QStringList fields)
{
    if(fields.length() < 5) {

    }

    NetworkError err = static_cast<NetworkError>(fields[2].toInt());
    bool fatal = ((err == NetworkError::CallsignInUse) ||
                  (err == NetworkError::CallsignInvalid) ||
                  (err == NetworkError::AlreadyRegistered) ||
                  (err == NetworkError::InvalidLogon) ||
                  (err == NetworkError::InvalidProtocolRevision) ||
                  (err == NetworkError::RequestedLevelTooHigh) ||
                  (err == NetworkError::ServerFull) ||
                  (err == NetworkError::CertificateSuspended) ||
                  (err == NetworkError::InvalidPositionForRating) ||
                  (err == NetworkError::UnauthorizedSoftware));

    return PDUProtocolError(fields[0], fields[1], err, fields[3], fields[4], fatal);
}

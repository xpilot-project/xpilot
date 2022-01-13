#include "pdu_protocol_error.h"

PDUProtocolError::PDUProtocolError() : PDUBase() {}

PDUProtocolError::PDUProtocolError(QString from, QString to, NetworkError type, QString param, QString msg, bool fatal) :
    PDUBase(from, to)
{
    ErrorType = type;
    Param = param;
    Message = msg;
    Fatal = fatal;
}

QStringList PDUProtocolError::toTokens() const
{
    QStringList tokens;
    tokens.append(From);
    tokens.append(To);
    tokens.append(QString::number(static_cast<int>(ErrorType)));
    tokens.append(Param);
    tokens.append(Message);
    return tokens;
}

PDUProtocolError PDUProtocolError::fromTokens(const QStringList &tokens)
{
    if(tokens.length() < 5) {
        throw PDUFormatException("Invalid field count.", Reassemble(tokens));
    }

    NetworkError err = static_cast<NetworkError>(tokens[2].toInt());
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

    return PDUProtocolError(tokens[0], tokens[1], err, tokens[3], tokens[4], fatal);
}

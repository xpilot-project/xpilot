#ifndef PDU_CHANGE_SERVER_H
#define PDU_CHANGE_SERVER_H

#include <QString>
#include "pdu_base.h"

class PDUChangeServer : public PDUBase
{
public:
    PDUChangeServer(QString from, QString to, QString newServer);

    QStringList toTokens() const;

    static PDUChangeServer fromTokens(const QStringList& fields);

    static QString pdu() { return "$XX"; }

    QString NewServer;

private:
    PDUChangeServer();
};


#endif // PDU_CHANGE_SERVER_H

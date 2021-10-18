#ifndef PDU_PLANEINFORESPONSE_H
#define PDU_PLANEINFORESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUPlaneInfoResponse: public PDUBase
{
public:
    PDUPlaneInfoResponse(QString from, QString to, QString equipment, QString airline, QString livery, QString csl);

    QStringList toTokens() const;

    static PDUPlaneInfoResponse fromTokens(const QStringList& fields);

    static QString pdu() { return "#SB"; }

    static inline QString FindValue(const QStringList& fields, QString key)
    {
        for(auto &field : fields) {
            if(field.toUpper().startsWith(key.toUpper() + "=")) {
                return field.mid(key.length() + 1);
            }
        }
        return "";
    }

    QString Equipment;
    QString Airline;
    QString Livery;
    QString CSL;

private:
    PDUPlaneInfoResponse();
};

#endif

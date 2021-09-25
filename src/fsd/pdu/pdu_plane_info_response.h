#ifndef PDU_PLANEINFORESPONSE_H
#define PDU_PLANEINFORESPONSE_H

#include <QString>
#include "pdu_base.h"

class PDUPlaneInfoResponse: public PDUBase
{
public:
    PDUPlaneInfoResponse(QString from, QString to, QString equipment, QString airline, QString livery, QString csl);

    QString Serialize() override;

    static PDUPlaneInfoResponse Parse(QStringList fields);

    static inline QString FindValue(QStringList& fields, QString key)
    {
        for(auto &field : fields) {
            if(field.toUpper().startsWith(key.toUpper() + "=")) {
                return field.left(key.length() + 1);
            }
        }
        return "";
    }

    QString Equipment;
    QString Airline;
    QString Livery;
    QString CSL;
};

#endif

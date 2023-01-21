/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

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

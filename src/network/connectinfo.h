#ifndef CONNECT_INFO_H
#define CONNECT_INFO_H

#include <QString>
#include <QObject>

namespace xpilot
{
    struct ConnectInfo
    {
        Q_GADGET
    public:
        QString Callsign;
        QString TypeCode;
        QString SelcalCode;
        bool ObserverMode;
        bool TowerViewMode;

        Q_PROPERTY(QString Callsign MEMBER Callsign)
        Q_PROPERTY(QString TypeCode MEMBER TypeCode)
        Q_PROPERTY(QString SelcalCode MEMBER SelcalCode)
    };
}

Q_DECLARE_METATYPE(xpilot::ConnectInfo)

#endif

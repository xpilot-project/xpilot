#ifndef CONNECT_INFO_H
#define CONNECT_INFO_H

#include <QString>
#include <QObject>

namespace xpilot
{
    struct ConnectInfo
    {
        Q_GADGET

        Q_PROPERTY(QString Callsign MEMBER Callsign)
        Q_PROPERTY(QString TypeCode MEMBER TypeCode)
        Q_PROPERTY(QString SelcalCode MEMBER SelcalCode)
        Q_PROPERTY(bool ObserverMode MEMBER ObserverMode)
        Q_PROPERTY(bool TowerViewMode MEMBER TowerViewMode)

    public:
        QString Callsign;
        QString TypeCode;
        QString SelcalCode;
        bool ObserverMode;
        bool TowerViewMode;

        bool operator==(const ConnectInfo& b) const
        {
            return Callsign == b.Callsign && TypeCode == b.TypeCode && SelcalCode == b.SelcalCode && ObserverMode == b.ObserverMode && TowerViewMode == b.TowerViewMode;
        }
        bool operator!=(const ConnectInfo& b) const
        {
            return !(*this == b);
        }
    };
}

Q_DECLARE_METATYPE(xpilot::ConnectInfo)

#endif

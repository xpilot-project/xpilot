#ifndef CONNECT_INFO_H
#define CONNECT_INFO_H

#include <QString>

namespace xpilot
{
    struct ConnectInfo
    {
        QString Callsign;
        QString TypeCode;
        QString SelcalCode;
        bool ObserverMode;
        bool TowerViewMode;
    };
}

#endif

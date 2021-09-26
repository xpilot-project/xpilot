#ifndef APP_CORE_H
#define APP_CORE_H

#include <QObject>
#include "network/networkmanager.h"

namespace xpilot
{
    enum class NotificationType
    {
        Info,
        Warning,
        Error
    };

    class AppCore : public QObject
    {
        Q_OBJECT

    public:
        AppCore(QObject *owner = nullptr);

    private:
        NetworkManager m_networkManager { this };
    };
}

#endif

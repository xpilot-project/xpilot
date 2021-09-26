#ifndef APP_CORE_H
#define APP_CORE_H

#include <QObject>
#include <QDir>
#include <QFile>

#include "network/networkmanager.h"
#include "appconfig.h"

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

    public slots:
        void SaveConfig();

    public:
        AppCore(QObject *owner = nullptr);
        AppConfig config;

    private:
        NetworkManager m_networkManager { this };
    };
}

#endif

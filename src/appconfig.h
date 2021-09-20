#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>

struct AppConfig
{
    Q_GADGET
    Q_PROPERTY(QString vatsimId MEMBER vatsimId)
    Q_PROPERTY(QString vatsimPassword MEMBER vatsimPassword)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString homeAirport MEMBER homeAirport)
public:
    QString vatsimId;
    QString vatsimPassword;
    QString name;
    QString homeAirport;
};
Q_DECLARE_METATYPE(AppConfig)

#endif // APPCONFIG_H

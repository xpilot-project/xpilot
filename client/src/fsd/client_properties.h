#ifndef CLIENT_PROPERTIES_H
#define CLIENT_PROPERTIES_H

#include <QString>

class ClientProperties
{
    QString Name;
    int VersionMajor;
    int VersionMinor;
    ushort ClientID;
    QString PrivateKey;

    ClientProperties(QString name, int versionMajor, int versionMinor, ushort clientId, QString key)
    {
        Name = name;
        VersionMajor = versionMajor;
        VersionMinor = versionMinor;
        ClientID = clientId;
        PrivateKey = key;
    }
};

#endif

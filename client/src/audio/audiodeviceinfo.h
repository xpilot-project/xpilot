#ifndef AUDIODEVICEINFO_H
#define AUDIODEVICEINFO_H

#include <QString>
#include <QObject>

struct AudioDeviceInfo
{
    Q_GADGET

    Q_PROPERTY(QString DeviceName MEMBER DeviceName)
    Q_PROPERTY(QString Id MEMBER Id)

public:
    QString DeviceName;
    QString Id;

    bool operator==(const AudioDeviceInfo& b) const {
        return DeviceName == b.DeviceName && Id == b.Id;
    }
    bool operator!=(const AudioDeviceInfo& b) const {
        return DeviceName != b.DeviceName || Id != b.Id;
    }
};
Q_DECLARE_METATYPE(AudioDeviceInfo)

#endif // AUDIODEVICEINFO_H

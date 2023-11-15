#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

namespace enums {

Q_NAMESPACE

enum class MessageType {
    Server,
    IncomingPrivate,
    OutgoingPrivate,
    TextOverride,
    IncomingRadioPrimary,
    IncomingRadioSecondary,
    OutgoingRadio,
    Broadcast,
    Wallop,
    Info,
    Error
};
Q_ENUM_NS(MessageType)

}

#endif // ENUMS_H

#ifndef FSD_SERIALIZER_H
#define FSD_SERIALIZER_H

#include <QtGlobal>
#include <QString>
#include "enums.h"

template<typename T>
inline QString toQString(const T& value);

template<typename T>
T fromQString(const QString &str);

template<>
QString toQString(const NetworkRating& value);

template<>
NetworkRating fromQString(const QString& str);

template<>
QString toQString(const NetworkFacility& value);

template<>
NetworkFacility fromQString(const QString& str);

template<>
QString toQString(const ProtocolRevision& value);

template<>
ProtocolRevision fromQString(const QString& str);

template<>
QString toQString(const SimulatorType& value);

template<>
SimulatorType fromQString(const QString& str);

template<>
QString toQString(const ClientQueryType& value);

template<>
ClientQueryType fromQString(const QString& str);

template<>
QString toQString(const FlightRules& value);

template<>
FlightRules fromQString(const QString& str);

#endif

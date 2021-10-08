#ifndef NEARBYATC_H
#define NEARBYATC_H

#include <QObject>

struct NearbyAtc {
    Q_GADGET
    Q_PROPERTY(QString callsign MEMBER callsign)
    Q_PROPERTY(QString realname MEMBER realname)
    Q_PROPERTY(QString frequency MEMBER frequency)
    Q_PROPERTY(int sim_frequency MEMBER sim_frequency)
public:
    QString callsign;
    QString realname;
    QString frequency;
    int sim_frequency;
};
Q_DECLARE_METATYPE(NearbyAtc)

//struct Station {
//    Q_GADGET
//    Q_PROPERTY(QString callsign MEMBER callsign)
//    Q_PROPERTY(QString realname MEMBER realname)
//    Q_PROPERTY(QString frequency MEMBER frequency)
//    Q_PROPERTY(int sim_frequency MEMBER sim_frequency)
//public:
//    QString callsign;
//    QString realname;
//    QString frequency;
//    int sim_frequency;
//};
//Q_DECLARE_METATYPE(Station)

//struct NearbyAtc {
//    Q_GADGET
//    Q_PROPERTY(QList<Station> center MEMBER center)
//    Q_PROPERTY(QList<Station> approach MEMBER approach)
//    Q_PROPERTY(QList<Station> tower MEMBER tower)
//    Q_PROPERTY(QList<Station> ground MEMBER ground)
//    Q_PROPERTY(QList<Station> delivery MEMBER delivery)
//    Q_PROPERTY(QList<Station> atis MEMBER atis)
//    Q_PROPERTY(QList<Station> observers MEMBER observers)
//public:
//    QList<Station> center;
//    QList<Station> approach;
//    QList<Station> tower;
//    QList<Station> ground;
//    QList<Station> delivery;
//    QList<Station> atis;
//    QList<Station> observers;
//};
//Q_DECLARE_METATYPE(NearbyAtc)

#endif // NEARBYATC_H

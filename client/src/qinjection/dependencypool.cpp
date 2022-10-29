#include "dependencypool.h"

#include <QDebug>
#include <QThread>

namespace QInjection {

struct Data {
    QMap<QString, CreatorBase *> creators;
    QList<SignalBase *> notifySignals;
};
Q_GLOBAL_STATIC(Data, d)

namespace Private {

QObject *create(const QString &key)
{
    if (!d->creators.contains(key)) {
        return nullptr;
    }

    auto creator = d->creators.value(key);

    switch (creator->_type) {
        case CreatorType::Unknown:
            return nullptr;
        case CreatorType::Scopped:
            return creator->create();
        case CreatorType::Singelton: {
                if (creator->_object) {
                    return creator->_object;
                }
                creator->_object = creator->create();
                return creator->_object;
            }
    }

    Q_UNREACHABLE();

    return nullptr;
}
bool remove(const QString &name)
{
    if (!d->creators.contains(name))
        return false;

    auto creator = d->creators.value(name);
    if (creator->_type == CreatorType::Singelton && creator->_object)
        creator->_object->deleteLater();
    return d->creators.remove(name) > 0;
}

bool contains(const QString &key)
{
    return d->creators.contains(key);
}

void addCreator(const QString &key, CreatorBase *creator)
{
    if (d->creators.contains(key)) {
        qWarning("Dependency pool already has a %s key", key.toLatin1().data());
        return;
    }
    d->creators.insert(key, creator);
}

void addSignel(SignalBase *signal)
{
    d->notifySignals.append(signal);
}

void removeSignel(SignalBase *signal)
{
    d->notifySignals.removeOne(signal);
}

int callSlots(QObject *o, bool sendNull)
{
    QString key{o->metaObject()->className()};
    int ret{0};

    for (auto &s : d->notifySignals)
        if (s->_key == key) {
            s->call(sendNull ? nullptr : o);
            ret++;
        }
    return ret;
}

CreatorType typeForKey(const QString &key)
{
    if (!d->creators.contains(key)) {
        return CreatorType::Unknown;
    }
    return d->creators.value(key)->_type;
}

void deleteObject(QObject *obj)
{
    obj->deleteLater();
}

} // namespace Impl

} // namespace QInjection

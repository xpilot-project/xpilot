#ifndef Pool_H
#define Pool_H

#include <QMap>
#include <QObject>
#include "dependencycreator.h"

#define dep ::QInjection
#define di_new(Type, ...) ::QInjection::create<Type, __VA_ARGS__>()
#define di_get(type) ::QInjection::get<type>()
#define di_add(type) ::QInjection::add<type>()
#define CLASS_NAME(T) T::staticMetaObject.className()

namespace QInjection {

namespace Private {

QObject *create(const QString &key);
bool remove(const QString &name);
bool contains(const QString &key);

void addCreator(const QString &key, CreatorBase *creator);
void addSignel(SignalBase *signal);
void removeSignel(SignalBase *signal);
int callSlots(QObject *o, bool sendNull = false);
CreatorType typeForKey(const QString &key);
void deleteObject(QObject *obj);

} // namespace Impl

#ifdef QOIJECTION_OLD
QObject *create(const QString &key);
bool remove(const QString &name);
bool contains(const QString &key);
#endif

template<class T>
Q_OUTOFLINE_TEMPLATE bool remove(){
    return Private::remove(CLASS_NAME(T));
}

template<class T>
Q_OUTOFLINE_TEMPLATE T *create()
{
    return qobject_cast<T*>(Private::create(CLASS_NAME(T)));
}

// Add Objects
template<class T>
Q_OUTOFLINE_TEMPLATE void addSingleton()
{
    auto creator = new OnceCreator<T>(CreatorType::Singelton);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<class T>
Q_OUTOFLINE_TEMPLATE void addSingleton(T *object) {
    auto creator = new SimpleCreator<T>(CreatorType::Singelton, object);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<class T>
Q_OUTOFLINE_TEMPLATE void addSingleton(T*(*slot)() ) {
    auto creator = new FunctionCreator<T>(CreatorType::Singelton, slot);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<class _Owner, class T>
Q_OUTOFLINE_TEMPLATE void addSingleton(_Owner *owner, void (_Owner::*slot)(T *))
{
    auto creator = new ClassFunctionCreator<_Owner, T>(CreatorType::Singelton, owner, slot);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<class T>
Q_OUTOFLINE_TEMPLATE void addScopped(T *(*slot)())
{
    auto creator = new FunctionCreator<T>(CreatorType::Scopped, slot);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<class _Owner, class T>
Q_OUTOFLINE_TEMPLATE void addScopped(_Owner *owner, void (_Owner::*slot)(T *))
{
    auto creator = new ClassFunctionCreator<_Owner, T>(CreatorType::Scopped, owner, slot);
    Private::addCreator(CLASS_NAME(T), creator);
}

template<typename T, class R>
Q_OUTOFLINE_TEMPLATE void registerObjectNotify(
    R *reciver,
    void(R::*slot)(T*))
{
    QString key = CLASS_NAME(T);
    auto s = new SignalPointer<T, R>();
    s->_key = key;
    s->slot = slot;
    s->reciver = reciver;
    Private::addSignel(s);

    QObject::connect(reciver, &QObject::destroyed, [s](QObject * = nullptr) {
        Private::removeSignel(s);
    });
}

template<typename T, class R>
Q_OUTOFLINE_TEMPLATE void registerObjectNotify(
    R *reciver,
    void(R::*slot)())
{
    QString key = CLASS_NAME(T);
    auto s = new SignalPointerWithoutParam<T, R>();
    s->_key = key;
    s->slot = slot;
    s->reciver = reciver;
    Private::addSignel(s);

    QObject::connect(reciver, &QObject::destroyed, [s](QObject * = nullptr) {
        Private::removeSignel(s);
    });
}

template<typename T, class R>
Q_OUTOFLINE_TEMPLATE void registerObjectNotify(
    R *reciver,
    std::function<void(T*)> cb)
{
    QString key = CLASS_NAME(T);
    auto s = new SignalPointerFunc<T, R>();
    s->_key = key;
    s->_cb = cb;
    Private::addSignel(s);

    QObject::connect(reciver, &QObject::destroyed, [s](QObject * = nullptr) {
        Private::removeSignel(s);
    });
}

}

#endif // Pool_H

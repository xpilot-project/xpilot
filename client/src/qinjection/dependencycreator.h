#ifndef DEPENDENCYCREATOR_H
#define DEPENDENCYCREATOR_H

#include <QObject>
#include <functional>
#define CLASS_NAME(T) T::staticMetaObject.className()

namespace QInjection {

enum class CreatorType {
    Unknown,
    Singelton,
    Scopped
};

struct CreatorBase
{
    CreatorType _type;
    QString _key;
    QObject *_object{nullptr};
    CreatorBase(CreatorType type, const QString &key);
    virtual QObject *create() = 0;
};

inline CreatorBase::CreatorBase(CreatorType type, const QString &key)
    : _type(type)
    , _key(key)
{}

template <class T>
struct FunctionCreator : CreatorBase
{
    T *(*_creatorFunction)();

    FunctionCreator (CreatorType type, T *(*creatorFunction)());

    QObject *create() override;
};

template <class T>
struct SimpleCreator : CreatorBase
{
    T *_object;

    SimpleCreator (CreatorType type, T *object);

    QObject *create() override;
};

template <class T>
struct OnceCreator : CreatorBase
{
    T *_object{nullptr};

    OnceCreator (CreatorType type);

    QObject *create() override;
};

template<class T>
Q_OUTOFLINE_TEMPLATE OnceCreator<T>::OnceCreator(CreatorType type)
    : CreatorBase(type, CLASS_NAME(T))
{

}

template<class T>
Q_OUTOFLINE_TEMPLATE QObject *OnceCreator<T>::create()
{
    if (!_object)
        _object = new T;
    return _object;
}

template<class T>
Q_OUTOFLINE_TEMPLATE SimpleCreator<T>::SimpleCreator(CreatorType type, T *object)
    : CreatorBase(type, CLASS_NAME(T))
    , _object(object)
{

}

template<class T>
Q_OUTOFLINE_TEMPLATE QObject *SimpleCreator<T>::create()
{
    return _object;
}

template<class T>
Q_OUTOFLINE_TEMPLATE FunctionCreator<T>::FunctionCreator(CreatorType type, T *(*creatorFunction)())
    : CreatorBase(type, CLASS_NAME(T))
    , _creatorFunction(creatorFunction)
{}

template<class T>
Q_OUTOFLINE_TEMPLATE QObject *FunctionCreator<T>::create()
{
    return _creatorFunction();
}

template <class OWNER, class T>
struct ClassFunctionCreator : CreatorBase
{
    T *(OWNER::*_creatorFunction)();
    OWNER *_owner;

    ClassFunctionCreator (CreatorType type, OWNER *owner, T *(OWNER::*creatorFunction)());

    QObject *create() override;
};

template<class OWNER, class T>
Q_OUTOFLINE_TEMPLATE ClassFunctionCreator<OWNER, T>::ClassFunctionCreator(
    CreatorType type, OWNER *owner, T *(OWNER::*creatorFunction)())
    : CreatorBase(type, CLASS_NAME(T)), _creatorFunction(creatorFunction), _owner(owner)
{
}

template<class OWNER, class T>
Q_OUTOFLINE_TEMPLATE QObject *ClassFunctionCreator<OWNER, T>::create()
{
    return (_owner->*_creatorFunction)();
}


struct SignalBase
{
    QString _key;
    void check(const QString &key, QObject *obj)
    {
        if (key == _key)
            call(obj);
    }
    virtual void call(QObject *obj) {
        Q_UNUSED(obj)
    }
};

template<typename T, class R>
struct SignalPointer : SignalBase
{
    R *reciver;
    void (R::*slot)(T *);

    void call(QObject *obj) override {
        if (obj)
            (reciver->*slot)(qobject_cast<T*>(obj));
        else
            (reciver->*slot)(nullptr);
    }
};

template<typename T, class R>
struct SignalPointerWithoutParam : SignalBase {
    R *reciver;
    void (R::*slot)();

    void call(QObject *obj) override
    {
        Q_UNUSED(obj)
        (reciver->*slot)();
    }
};

template<typename T, class R>
struct SignalPointerFunc : SignalBase {
    std::function<void(T *)> _cb;

    void call(QObject *obj) override
    {
        if (obj)
            _cb(qobject_cast<T *>(obj));
        else
            _cb(nullptr);
    }
};

} // namespace QInjection

#endif // DEPENDENCYCREATOR_H

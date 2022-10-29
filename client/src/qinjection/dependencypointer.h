#ifndef DEPENDENCYPOINTER_H
#define DEPENDENCYPOINTER_H

#include "dependencypool.h"

#include <QVariant>
#include <QObject>

namespace QInjection {

template<class T>
class Pointer : public QObject{
    Q_STATIC_ASSERT_X(!std::is_pointer<T>::value, "QPointer's template type must not be a pointer type");

    T *_data;
    bool _deleteOnExit{false};

public:
    Pointer();
    ~Pointer();

    T *data() const;
    T *operator->() const;
    T &operator*() const;
    inline operator T *() const
    {
        return data();
    }

    inline bool isNull() const
    {
        return _data == nullptr;
    }

    inline void clear()
    {
        _data = nullptr;
    }
};

template<class T>
Q_OUTOFLINE_TEMPLATE Pointer<T>::Pointer() : _data(create<T>())
{
    auto t = Private::typeForKey(CLASS_NAME(T));
    _deleteOnExit = t == CreatorType::Scopped;
    registerObjectNotify<T>(this, [this](T *t) {
        _data = t;
    });
}

template<class T>
Q_OUTOFLINE_TEMPLATE Pointer<T>::~Pointer()
{
    if (_deleteOnExit && _data)
        Private::deleteObject(static_cast<QObject*>(_data));
}

template<class T>
Q_OUTOFLINE_TEMPLATE T *Pointer<T>::data() const
{
    return _data;
}

template<class T>
Q_OUTOFLINE_TEMPLATE T *Pointer<T>::operator->() const
{
    return data();
}

template<class T>
Q_OUTOFLINE_TEMPLATE T &Pointer<T>::operator*() const
{
    return *data();
}

}

template<class T>
Q_DECLARE_TYPEINFO_BODY(QInjection::Pointer<T>, Q_MOVABLE_TYPE);

template<class T>
inline bool operator==(const T *o, const QInjection::Pointer<T> &p)
{
    return o == p.operator->();
}

template<class T>
inline bool operator==(const QInjection::Pointer<T> &p, const T *o)
{
    return p.operator->() == o;
}

template<class T>
inline bool operator==(T *o, const QInjection::Pointer<T> &p)
{
    return o == p.operator->();
}

template<class T>
inline bool operator==(const QInjection::Pointer<T> &p, T *o)
{
    return p.operator->() == o;
}

template<class T>
inline bool operator==(const QInjection::Pointer<T> &p1,
                       const QInjection::Pointer<T> &p2)
{
    return p1.operator->() == p2.operator->();
}

template<class T>
inline bool operator!=(const T *o, const QInjection::Pointer<T> &p)
{
    return o != p.operator->();
}

template<class T>
inline bool operator!=(const QInjection::Pointer<T> &p, const T *o)
{
    return p.operator->() != o;
}

template<class T>
inline bool operator!=(T *o, const QInjection::Pointer<T> &p)
{
    return o != p.operator->();
}

template<class T>
inline bool operator!=(const QInjection::Pointer<T> &p, T *o)
{
    return p.operator->() != o;
}

template<class T>
inline bool operator!=(const QInjection::Pointer<T> &p1,
                       const QInjection::Pointer<T> &p2)
{
    return p1.operator->() != p2.operator->();
}


#ifndef QINJECTION_BC
namespace Dependency {
template<class T>
class Pointr : public QInjection::Pointer<T>
{};

}
#endif
#endif // DEPENDENCYPOINTER_H

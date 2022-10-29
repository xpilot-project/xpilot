#ifndef DEPENDENCYINJECTOR_H
#define DEPENDENCYINJECTOR_H

#include "dependencypool.h"

namespace QInjection {

class Injecter
{
    const char *_key{nullptr};

public:
    Injecter();
    Injecter(const char *key);

    Injecter(const Injecter &) = delete;
    Injecter(Injecter &&) = delete;

    template<class T>
    operator T *()
    {
        T *tmp = nullptr;
        if (_key) {
            tmp = qobject_cast<T *>(Private::create(_key));
        }
        else {
            tmp = create<T>();
        }
        if (tmp && Private::typeForKey(CLASS_NAME(T)) == CreatorType::Scopped) {
            tmp->deleteLater();
        }
        return tmp;
    }
};

extern Injecter Inject;
} // namespace QInjection

#endif // DEPENDENCYINJECTOR_H

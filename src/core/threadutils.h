#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QtGlobal>
#include <QPointer>
#include <QTimer>
#include <thread>
#include <sstream>

class ThreadUtils
{
public:
    ThreadUtils() = delete;

    /**
     * @brief Is the current thread the object's thread?
     * @param toBeChecked
     * @return
     */
    static inline bool isInThisThread(const QObject* objectToTest)
    {
        return QThread::currentThread() == objectToTest->thread();
    }

    static inline bool isMainThread()
    {
        return qApp && QThread::currentThread() == qApp->thread();
    }
};

#endif

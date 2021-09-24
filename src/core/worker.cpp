#include "worker.h"
#include "threadutils.h"

#include <future>
#include <QTimer>
#include <QPointer>
#include <QCoreApplication>
#include <QDateTime>

#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

namespace xpilot::core
{
    QSet<WorkerBase*> WorkerBase::m_allWorkers;

    void RegularThread::run()
    {
#ifdef Q_OS_WIN32
        m_handle = GetCurrentThread();
        QThread::run();
        m_handle = nullptr;
#else
        QThread::run();
#endif
    }

    RegularThread::~RegularThread()
    {
        const QString name = this->objectName();

#ifdef Q_OS_WIN32
        auto handle = m_handle.load();
        if(handle)
        {
            const auto status = WaitForSingleObject(handle, 0);
            if(isRunning())
            {
                switch(status)
                {
                default:
                case WAIT_FAILED:
                    qWarning() << "Thread" << name << "unspecified error";
                    break;
                case WAIT_OBJECT_0:
                    qWarning() << "Thread" << name << "unsafely terminated by program shutdown";
                case WAIT_TIMEOUT:
                    break;
                }
            }
        }
#endif

        quit();

        // the wait avoids: QThread: Destroyed while thread is still running
        const unsigned long timeoutMs = 5 * 1000;
        wait(timeoutMs);
    }

    void Worker::ps_runTask()
    {
        m_result = m_task();

        this->setFinished();

        QThread* workerThread = this->thread();
        Q_ASSERT_X(workerThread->thread()->isRunning(), Q_FUNC_INFO, "Owner thread's event loop already ended");

        this->deleteLater();
        connect(this, &QObject::destroyed, workerThread, &QObject::deleteLater);

        this->moveToThread(workerThread->thread());
    }

    WorkerBase::WorkerBase()
    {
        m_allWorkers.insert(this);
    }

    WorkerBase::~WorkerBase()
    {
        m_allWorkers.remove(this);
    }

    void WorkerBase::waitForFinished() noexcept
    {
        std::promise<void> promise;
        then([&] { promise.set_value(); });
        promise.get_future().wait();
    }

    void WorkerBase::abandon() noexcept
    {
        if(thread() != thread()->thread())
        {
            thread()->requestInterruption();
        }
        quit();
    }

    void WorkerBase::abandonAndWait() noexcept
    {
        if(thread() != thread()->thread())
        {
            thread()->requestInterruption();
        }
        quitAndWait();
    }

    bool WorkerBase::isAbandoned() const
    {
        Q_ASSERT(thread() == QThread::currentThread());
        return thread()->isInterruptionRequested();
    }

    ContinuousWorker::ContinuousWorker(QObject* owner, const QString& name) :
        m_owner(owner),
        m_name(name)
    {
        Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Empty name");
        this->setObjectName(m_name);
        m_updateTimer.setObjectName(m_name + ":timer");
    }

    void ContinuousWorker::start(QThread::Priority priority)
    {
        if(hasStarted()) return;

        // avoid message "QObject: Cannot create children for a parent that is in a different thread"
        Q_ASSERT_X(ThreadUtils::isInThisThread(m_owner), Q_FUNC_INFO, "Needs to be started in owner thread");
        emit this->aboutToStart();
        setStarted();
        auto *thread = new RegularThread(m_owner);

        Q_ASSERT(m_owner);
        if(m_owner)
        {
            const QString ownerName = m_owner->objectName().isEmpty() ? m_owner->metaObject()->className() : m_owner->objectName();
            thread->setObjectName(ownerName + ": " + m_name);
        }

        moveToThread(thread);
        connect(thread, &QThread::started,  this, &ContinuousWorker::initialize);
        connect(thread, &QThread::finished, &m_updateTimer, &QTimer::stop);
        connect(thread, &QThread::finished, this, &ContinuousWorker::cleanup);
        connect(thread, &QThread::finished, this, &ContinuousWorker::finish);
        thread->start(priority);
    }

    void ContinuousWorker::quit() noexcept
    {
        this->setEnabled(false);

        // already in owner's thread? then return
        if (this->thread() == m_owner->thread()) { return; }

        // remark: cannot stop timer here, as I am normally not in the correct thread
        this->beforeQuit();
        thread()->quit();
    }

    void ContinuousWorker::quitAndWait() noexcept
    {
        this->setEnabled(false);

        // already in owner's thread? then return
        if (this->thread() == m_owner->thread()) { return; }

        // called by own thread, will deadlock, return
        if (ThreadUtils::isInThisThread(this)) { return; }

        QThread *workerThread = thread(); // must be before quit()
        this->quit();

        const unsigned long waitTimeoutMs = this->waitTimeoutMs();
        workerThread->wait(waitTimeoutMs);
    }

    void ContinuousWorker::startUpdating(int updateTimeSecs)
    {
        Q_ASSERT_X(this->hasStarted(), Q_FUNC_INFO, "Worker not yet started");
        if (!ThreadUtils::isInThisThread(this))
        {
            // shift in correct thread
            QPointer<ContinuousWorker> myself(this);
            QTimer::singleShot(0, this, [ = ]
            {
                if (!myself) { return; }
                this->doIfNotFinished([ = ] { startUpdating(updateTimeSecs); });
            });
            return;
        }

        // here in correct timer thread
        if (updateTimeSecs < 0)
        {
            this->setEnabled(false);
            m_updateTimer.stop();
        }
        else
        {
            this->setEnabled(true);
            m_updateTimer.start(1000 * updateTimeSecs);
        }
    }

    void ContinuousWorker::stopUpdateTimer()
    {
        if (!m_updateTimer.isActive()) { return; }

        // avoid "Timers cannot be stopped from another thread"
        if (ThreadUtils::isInThisThread(&m_updateTimer))
        {
            m_updateTimer.stop();
        }
        else
        {
            QPointer<ContinuousWorker> myself(this);
            QTimer::singleShot(0, &m_updateTimer, [ = ]
            {
                // stop timer in timer thread
                if (!myself) { return; }
                m_updateTimer.stop();
            });
        }
    }

    void ContinuousWorker::finish()
    {
        this->setFinished();

        Q_ASSERT_X(m_owner->thread()->isRunning(), Q_FUNC_INFO, "Owner thread's event loop already ended");

        // MS 2018-09 Now we post the DeferredDelete event from within the worker thread, but rely on it being dispatched
        //            by the owner thread. Posted events are moved along with the object when moveToThread is called.
        //            We also connect its destroyed signal to delete the worker thread at the same time.
        this->deleteLater();
        connect(this, &QObject::destroyed, this->thread(), &QObject::deleteLater);

        this->moveToThread(m_owner->thread()); // move worker back to the thread which constructed it, so there is no race on deletion
        // must not access the worker beyond this point, as it now lives in the owner's thread and could be deleted at any moment
    }
}

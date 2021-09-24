#ifndef XPILOT_WORKER_H
#define XPILOT_WORKER_H

#include <QFuture>
#include <QMetaObject>
#include <QMetaType>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QWeakPointer>
#include <QtGlobal>

namespace xpilot::core
{
    /**
     * @brief Subclass of QThread whose destructor waits for the thread to finish.
     */
    class RegularThread : public QThread
    {
        Q_OBJECT

    public:
        RegularThread(QObject*parent = nullptr) : QThread(parent) {}

        virtual ~RegularThread() override;

    protected:
        // QThread::run
        virtual void run() override;

    private:
        std::atomic<void*> m_handle { nullptr };
    };

    /**
     * @brief Base class for Worker and ContinousWorker
     */
    class WorkerBase : public QObject
    {
        Q_OBJECT

    public:

        /**
         * @brief Connects a functor or method which will be called when the task is finished.
         */
        template <typename T, typename F>
        void then(T* context, F functor)
        {
            Q_ASSERT(context->thread() == QThread::currentThread());
            QMutexLocker lock(&m_finishedMutex);
            connect(this, &WorkerBase::finished, context, functor);
            // if (m_finished)
        }

        /**
         * @brief Connects to a functor which will be called when the task is finished.
         */
        template <typename F>
        void then(F functor)
        {
            QMutexLocker lock(&m_finishedMutex);
            connect(this, &WorkerBase::finished, functor);
            if(m_finished) { functor(); }
        }

        /**
         * @brief Returns true if the task has finished
         */
        bool isFinished() const
        {
            QMutexLocker lock(&m_finishedMutex);
            return m_finished;
        }

        /**
         * @brief Executes some code (in the caller's thread) if the task has finished.
         */
        template <typename F>
        void doIfFinished(F functor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (m_finished) { functor(); }
        }

        /**
         * @brief Executes some code (in the caller's thread) if the tsak has not finished.
         */
        template <typename F>
        void doIfNotFinished(F functor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (! m_finished) { functor(); }
        }

        /**
         * @brief Executes some code (in the caller's thread) if the task has finished and some different code if it has not finished.
         */
        template <typename F1, typename F2>
        void doIfFinishedElse(F1 ifFunctor, F2 elseFunctor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (m_finished) { ifFunctor(); }
            else { elseFunctor(); }
        }

        /**
         * @brief Blocks until the task is finished.
         */
        void waitForFinished() noexcept;

        /**
         * @brief Notify the task that its result is no longer needed, so it can finish early.
         */
        void abandon() noexcept;

        /**
         * @brief Convenience to call abandon() followed by waitForFinished().
         */
        void abandonAndWait() noexcept;

        /**
         * @brief All workers that currently exist.
         */
        static const QSet<WorkerBase *> &allWorkers() { return m_allWorkers; }

    signals:
        void aboutToStart();
        void finished();

    protected:
        WorkerBase();
        virtual ~WorkerBase() override;

        bool isAbandoned() const;

        bool hasStarted() const { return m_started; }

        void setStarted() { m_started = true; }

        void setFinished()
        {
            QMutexLocker lock(&m_finishedMutex);
            m_finished = true;
            emit finished();
        }

    private:
        virtual void quit() noexcept {}
        virtual void quitAndWait() noexcept { waitForFinished(); }

        bool m_started = false;
        bool m_finished = false;
        mutable QMutex m_finishedMutex { QMutex::Recursive };
        static QSet<WorkerBase*> m_allWorkers;
    };

    /**
     * @brief Class for doing some arbitrary parcel of work in its own thread.
     *
     * The task is exposed as a function object, so could be a lambda or a hand-written closure.
     * Worker can not be subclassed, instead it can be extended with rich callable task objects.
     */
    class Worker final : public WorkerBase
    {
        Q_OBJECT

    private slots:
        /**
         * @brief Called when the worker has moved into its new thread.
         */
        void ps_runTask();

    private:
        Worker(const std::function<QVariant()> &task) : m_task(task) {}

        std::function<QVariant()> m_task;
        QVariant m_result;
    };

    /**
     * @brief Base class for a long-lived worker object which lives in its own thread.
     */
    class ContinuousWorker : public WorkerBase
    {
        Q_OBJECT

    public:
        /**
         * @brief Constructor
         * @param owner Will be the parent of the new thread (the worker has no parent).
         * @param name A name for the worker, which will be used to create a name for the thread.
         */
        ContinuousWorker(QObject* owner, const QString &name);

        /**
         * @brief Starts a new thread and moves the worker into it.
         */
        void start(QThread::Priority priority = QThread::InheritPriority);

        /**
         * @brief Stops the thread the next time around its event loop.
         * The thread and the worker will then be deleted.
         */
        virtual void quit() noexcept final override;

        /**
         * @brief Calls quit() and blocks until the thread is finished.
         * This will deadlock if called by the worker thread.
         */
        virtual void quitAndWait() noexcept final override;

        /**
         * @return Returns whether or not the worker is enabled (running)
         */
        bool isEnabled() const { return m_enabled; }

        /**
         * @brief Enabled (running)?
         */
        void setEnabled(bool enabled) { m_enabled = enabled; }

        /**
         * @brief Start updating (start/stop timer)
         */
        void startUpdating(int updateTimeSecs);

        /**
         * @brief Name of the worker
         * @return
         */
        const QString &getName() { return m_name; }

        /**
         * @brief Owner of the worker
         */
        const QObject* owner() const { return m_owner; }

    protected:
        /**
         * @brief Called when the thread is started.
         */
        virtual void initialize() {}

        /**
         * @brief Called when the thread is finished
         */
        virtual void cleanup() {}

        /**
         * @brief Called before quit is called
         * This can be used to "clean up things" or request work functions to stop.
         */
        virtual void beforeQuit() noexcept {}

        /**
         * @brief Wait time for quitandWait, 0 means no waiting
         */
        virtual unsigned long waitTimeoutMs() const { return 15 * 1000; }

        /**
         * @brief Safely stops update time.
         */
        void stopUpdateTimer();

        QTimer m_updateTimer { this }; // timer which can be used by implementing classes.

    private:
        /**
         * @brief Called after cleanup()
         */
        void finish();

        QObject *m_owner = nullptr; // owner, parent of the QThread
        QString m_name; // worker's name
        std::atomic<bool> m_enabled { true }; // whether or not the worker is enabled
    };
}

#endif

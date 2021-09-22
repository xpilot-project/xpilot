#ifndef Worker_h
#define Worker_h

#include <QThread>
#include <atomic>

namespace Xpilot
{
    class RegularThread : public QThread
    {
        Q_OBJECT

    public:
        RegularThread(QObject* parent = nullptr):QThread(parent) {}

        virtual ~RegularThread() override;

    protected:
        virtual void run() override;

    private:
        std::atomic<void*> m_handle{nullptr};
    };
}

#endif

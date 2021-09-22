#ifndef FsdSession_h
#define FsdSession_h

#include <QObject>
#include <QTcpSocket>

namespace Xpilot::Fsd
{
    class FsdSession
    {
        Q_OBJECT

    public:
        FsdSession(QObject* owner = nullptr);

    signals:
        void networkConnected();
        void networkDisconnected();

    private:
        QTcpSocket m_socket {this};

        static int constexpr m_serverAuthChallengeInterval = 60000;
        static int constexpr m_serverAuthChallengeResponseWindow = 30000;
    };
}

#endif

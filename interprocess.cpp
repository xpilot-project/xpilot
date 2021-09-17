#include "interprocess.h"
#include <QTimer>

InterProcess::InterProcess(QObject* parent) : QObject(parent)
{
    process.setProgram("XplaneBridge/XplaneBridge.exe");
    process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process.start();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &InterProcess::Tick);
    timer->start(10);
}

InterProcess::~InterProcess()
{
    qDebug("Terminating IPC process");
    process.kill();
}


void InterProcess::Tick()
{

}

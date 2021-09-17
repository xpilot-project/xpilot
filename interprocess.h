#ifndef INTERPROCESS_H
#define INTERPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>

class InterProcess : public QObject
{
    Q_OBJECT
public:
    explicit InterProcess(QObject *parent = nullptr);
    ~InterProcess();
    void Tick();

private:
    QProcess process;

signals:
    void inputReceived(QString data);
};

#endif // INTERPROCESS_H

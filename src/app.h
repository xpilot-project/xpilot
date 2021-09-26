#ifndef APP_H
#define APP_H

#include <QObject>

#include "fsd/fsd_client.h"

namespace xpilot
{
    class App : public QObject
    {
        Q_OBJECT

    public:
        App(QObject *owner = nullptr);

    private:
        FsdClient fsd;

        void HandleServerIdentification(PDUServerIdentification pdu);
    };
}

#endif

#ifndef RADIOSTACK_H
#define RADIOSTACK_H

#include <QObject>

struct RadioStack {
    Q_GADGET
    Q_PROPERTY(bool avionicsPowerOn MEMBER avionicsPowerOn)
    Q_PROPERTY(int com1Frequency MEMBER com1Frequency)
    Q_PROPERTY(bool com1ReceiveEnabled MEMBER com1ReceiveEnabled)
    Q_PROPERTY(int com2Frequency MEMBER com2Frequency)
    Q_PROPERTY(bool com2ReceiveEnabled MEMBER com2ReceiveEnabled)
    Q_PROPERTY(int transmitComSelection MEMBER transmitComSelection)
public:
    bool avionicsPowerOn;
    int com1Frequency;
    bool com1ReceiveEnabled;
    int com2Frequency;
    bool com2ReceiveEnabled;
    int transmitComSelection;
};
Q_DECLARE_METATYPE(RadioStack)

#endif // RADIOSTACK_H

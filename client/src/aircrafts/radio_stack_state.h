#pragma once

#include <QObject>
#include <QtGlobal>

struct RadioStackState
{
    Q_GADGET
public:
    bool AvionicsPowerOn;
    uint Com1Frequency;
    bool Com1TransmitEnabled;
    bool Com1ReceiveEnabled;
    uint Com1Volume;
    uint Com2Frequency;
    bool Com2TransmitEnabled;
    bool Com2ReceiveEnabled;
    uint Com2Volume;
    bool SquawkingModeC;
    bool SquawkingIdent;
    ushort TransponderCode;
    bool SelcalMuteOverride;

    Q_PROPERTY(bool AvionicsPowerOn MEMBER AvionicsPowerOn)

    Q_PROPERTY(bool Com1TransmitEnabled MEMBER Com1TransmitEnabled)
    Q_PROPERTY(bool Com1ReceiveEnabled MEMBER Com1ReceiveEnabled)
    Q_PROPERTY(uint Com1Frequency MEMBER Com1Frequency)
    Q_PROPERTY(uint Com1Volume MEMBER Com1Volume)

    Q_PROPERTY(bool Com2TransmitEnabled MEMBER Com2TransmitEnabled)
    Q_PROPERTY(bool Com2ReceiveEnabled MEMBER Com2ReceiveEnabled)
    Q_PROPERTY(uint Com2Frequency MEMBER Com2Frequency)
    Q_PROPERTY(uint Com2Volume MEMBER Com2Volume)

    Q_PROPERTY(bool SquawkingModeC MEMBER SquawkingModeC)
    Q_PROPERTY(bool SquawkingIdent MEMBER SquawkingIdent)

    Q_PROPERTY(bool SelcalMuteOverride MEMBER SelcalMuteOverride)

    bool operator==(RadioStackState &other) const
    {
        return AvionicsPowerOn == other.AvionicsPowerOn
                && Com1Frequency == other.Com1Frequency
                && Com1TransmitEnabled == other.Com1TransmitEnabled
                && Com1ReceiveEnabled == other.Com1ReceiveEnabled
                && Com1Volume == other.Com1Volume
                && Com2Frequency == other.Com2Frequency
                && Com2ReceiveEnabled == other.Com2ReceiveEnabled
                && Com2TransmitEnabled == other.Com2TransmitEnabled
                && Com2Volume == other.Com2Volume
                && SquawkingModeC == other.SquawkingModeC
                && SquawkingIdent == other.SquawkingIdent
                && TransponderCode == other.TransponderCode
                && SelcalMuteOverride == other.SelcalMuteOverride;
    }

    bool operator!=(RadioStackState &other) const
    {
        return AvionicsPowerOn != other.AvionicsPowerOn
                || Com1Frequency != other.Com1Frequency
                || Com1TransmitEnabled != other.Com1TransmitEnabled
                || Com1ReceiveEnabled != other.Com1ReceiveEnabled
                || Com1Volume != other.Com1Volume
                || Com2Frequency != other.Com2Frequency
                || Com2ReceiveEnabled != other.Com2ReceiveEnabled
                || Com2TransmitEnabled != other.Com2TransmitEnabled
                || Com2Volume != other.Com2Volume
                || SquawkingModeC != other.SquawkingModeC
                || SquawkingIdent != other.SquawkingIdent
                || TransponderCode != other.TransponderCode
                || SelcalMuteOverride != other.SelcalMuteOverride;
    }
};

Q_DECLARE_METATYPE(RadioStackState)

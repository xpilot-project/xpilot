/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

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
    bool OverrideRadioPower;

    Q_PROPERTY(bool AvionicsPowerOn MEMBER AvionicsPowerOn)
    Q_PROPERTY(bool OverrideRadioPower MEMBER OverrideRadioPower)

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
                && OverrideRadioPower == other.OverrideRadioPower
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
                || OverrideRadioPower != other.OverrideRadioPower
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

#ifndef FREQUENCY_UTILS_H
#define FREQUENCY_UTILS_H

#include <QtGlobal>
#include <QString>

static uint Normalize25KhzFsdFrequency(uint freq)
{
    if(freq % 100 == 20 || freq % 100 == 70)
    {
        freq += 5;
    }
    return freq;
}

static uint MatchFsdFormat(uint freq)
{
    QString tmp = QString::number(freq);
    tmp = tmp.mid(1, tmp.length() - 1);
    return Normalize25KhzFsdFrequency(tmp.toUInt());
}

#endif

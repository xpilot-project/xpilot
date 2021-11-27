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

static uint FromNetworkFormat(uint freq)
{
    return (freq + 100000);
}

static uint Denormalize25KhzFsdFrequency(uint freq)
{
    if((freq % 100) == 25 || (freq % 100) == 75)
    {
        freq -= 5;
    }
    return freq;
}

static uint MatchFsdFormat(uint freq)
{
    QString tmp = QString::number(freq);
    tmp = tmp.mid(1, tmp.length() - 1);
    return tmp.toUInt();
}

#endif

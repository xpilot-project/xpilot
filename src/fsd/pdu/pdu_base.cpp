#include "pdu_base.h"

PDUBase::PDUBase(QString from, QString to) :
    From(from),
    To(to)
{

}

uint PDUBase::PackPitchBankHeading(double pitch, double bank, double heading)
{
    double p = pitch / -360.0;
    if(p < 0)
    {
        p += 1.0;
    }
    p *= 1024.0;

    double b = bank / -360.0;
    if(b < 0)
    {
        b += 1.0;
    }
    b *= 1024.0;

    double h = heading / 360.0 * 1024.0;

    return ((uint)p << 22) | ((uint)b << 12) | ((uint)h << 2);
}

void PDUBase::UnpackPitchBankHeading(uint pbh, double &pitch, double &bank, double &heading)
{
    uint pitchInt = pbh >> 22;
    pitch = pitchInt / 1024.0 * -360.0;
    if(pitch > 180.0)
    {
        pitch -= 360.0;
    }
    else if (pitch <= -180.0)
    {
        pitch += 360.0;
    }

    uint bankInt = (pbh >> 12) & 0x3FF;
    bank = bankInt / 1024.0 * -360.0;
    if (bank > 180.0)
    {
        bank -= 360.0;
    }
    else if (bank <= -180.0)
    {
        bank += 360.0;
    }

    uint hdgInt = (pbh >> 2) & 0x3FF;
    heading = hdgInt / 1024.0 * 360.0;
    if (heading < 0.0)
    {
        heading += 360.0;
    }
    else if (heading >= 360.0)
    {
        heading -= 360.0;
    }
}

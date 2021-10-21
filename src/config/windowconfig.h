#ifndef WINDOW_CONFIG_H
#define WINDOW_CONFIG_H

#include <QObject>

namespace xpilot
{
    struct ClientWindowConfig
    {
        Q_GADGET

        Q_PROPERTY(int X MEMBER X)
        Q_PROPERTY(int Y MEMBER Y)
        Q_PROPERTY(int Width MEMBER Width)
        Q_PROPERTY(int Height MEMBER Height)
        Q_PROPERTY(bool Maximized MEMBER Maximized)

    public:
        int X;
        int Y;
        int Width;
        int Height;
        bool Maximized;

        bool operator==(const ClientWindowConfig& b)
        {
            return X == b.X && Y == b.Y && Width == b.Width && Height == b.Height && Maximized == b.Maximized;
        }
        bool operator!=(const ClientWindowConfig& b)
        {
            return X != b.X || Y != b.Y || Width != b.Width || Height != b.Height || Maximized != b.Maximized;
        }
    };
}

Q_DECLARE_METATYPE(xpilot::ClientWindowConfig)

#endif

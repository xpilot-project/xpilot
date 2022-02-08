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
        int X = 10;
        int Y = 10;
        int Width = 800;
        int Height = 250;
        bool Maximized = false;

        bool operator==(ClientWindowConfig& rhs) const
        {
            return X == rhs.X && Y == rhs.Y && Width == rhs.Width && Height == rhs.Height && Maximized == rhs.Maximized;
        }
        bool operator!=(ClientWindowConfig& rhs) const
        {
            return X != rhs.X || Y != rhs.Y || Width != rhs.Width || Height != rhs.Height || Maximized != rhs.Maximized;
        }
    };
}

Q_DECLARE_METATYPE(xpilot::ClientWindowConfig)

#endif

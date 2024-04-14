#pragma once

struct Vector3 
{
    union
    {
        struct
        {
            double X;
            double Y;
            double Z;
        };
    };

    inline Vector3() : X(0), Y(0), Z(0) 
    {}

    inline Vector3(double value) : X(value), Y(value), Z(value)
    {}

    inline Vector3(double x, double y) : X(x), Y(y), Z(0)
    {}

    inline Vector3(double x, double y, double z) : X(x), Y(y), Z(z)
    {}

    static inline Vector3 Zero()
    {
        return Vector3(0, 0, 0);
    }

    static inline Vector3 Cross(Vector3 lhs, Vector3 rhs)
    {
        double x = lhs.Y * rhs.Z - lhs.Z * rhs.Y;
        double y = lhs.Z * rhs.X - lhs.X * rhs.Z;
        double z = lhs.X * rhs.Y - lhs.Y * rhs.X;
        return Vector3(x, y, z);
    }

    static inline double Dot(Vector3 lhs, Vector3 rhs)
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
    }

    inline struct Vector3& operator+=(const double rhs);
    inline struct Vector3& operator-=(const double rhs);
    inline struct Vector3& operator*=(const double rhs);
    inline struct Vector3& operator/=(const double rhs);
    inline struct Vector3& operator+=(const Vector3 rhs);
    inline struct Vector3& operator-=(const Vector3 rhs);
};

inline Vector3 operator-(Vector3 rhs);
inline Vector3 operator+(Vector3 lhs, const double rhs);
inline Vector3 operator-(Vector3 lhs, const double rhs);
inline Vector3 operator*(Vector3 lhs, const double rhs);
inline Vector3 operator/(Vector3 lhs, const double rhs);
inline Vector3 operator+(const double lhs, Vector3 rhs);
inline Vector3 operator-(const double lhs, Vector3 rhs);
inline Vector3 operator*(const double lhs, Vector3 rhs);
inline Vector3 operator/(const double lhs, Vector3 rhs);
inline Vector3 operator+(Vector3 lhs, const Vector3 rhs);
inline Vector3 operator-(Vector3 lhs, const Vector3 rhs);
inline bool operator==(const Vector3 lhs, const Vector3 rhs);
inline bool operator!=(const Vector3 lhs, const Vector3 rhs);

struct Vector3& Vector3::operator+=(const double rhs)
{
    X += rhs;
    Y += rhs;
    Z += rhs;
    return *this;
}

struct Vector3& Vector3::operator-=(const double rhs)
{
    X -= rhs;
    Y -= rhs;
    Z -= rhs;
    return *this;
}

struct Vector3& Vector3::operator*=(const double rhs)
{
    X *= rhs;
    Y *= rhs;
    Z *= rhs;
    return *this;
}

struct Vector3& Vector3::operator/=(const double rhs)
{
    X /= rhs;
    Y /= rhs;
    Z /= rhs;
    return *this;
}

struct Vector3& Vector3::operator+=(const Vector3 rhs)
{
    X += rhs.X;
    Y += rhs.Y;
    Z += rhs.Z;
    return *this;
}

struct Vector3& Vector3::operator-=(const Vector3 rhs)
{
    X -= rhs.X;
    Y -= rhs.Y;
    Z -= rhs.Z;
    return *this;
}

Vector3 operator-(Vector3 rhs) { return rhs * -1; }
Vector3 operator+(Vector3 lhs, const double rhs) { return lhs += rhs; }
Vector3 operator-(Vector3 lhs, const double rhs) { return lhs -= rhs; }
Vector3 operator*(Vector3 lhs, const double rhs) { return lhs *= rhs; }
Vector3 operator/(Vector3 lhs, const double rhs) { return lhs /= rhs; }
Vector3 operator+(const double lhs, Vector3 rhs) { return rhs += lhs; }
Vector3 operator-(const double lhs, Vector3 rhs) { return rhs -= lhs; }
Vector3 operator*(const double lhs, Vector3 rhs) { return rhs *= lhs; }
Vector3 operator/(const double lhs, Vector3 rhs) { return rhs /= lhs; }
Vector3 operator+(Vector3 lhs, const Vector3 rhs) { return lhs += rhs; }
Vector3 operator-(Vector3 lhs, const Vector3 rhs) { return lhs -= rhs; }

bool operator==(const Vector3 lhs, const Vector3 rhs)
{
    return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
}

bool operator!=(const Vector3 lhs, const Vector3 rhs)
{
    return !(lhs == rhs);
}
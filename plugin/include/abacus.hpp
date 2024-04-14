#pragma once

#include "vector3.hpp"

#include <cmath>
#include <algorithm>

struct Quaternion
{
    union
    {
        struct
        {
            double I;
            double J;
            double K;
            double U;
        };
    };

    inline Quaternion();
    inline Quaternion(Vector3 vector, double scalar);
    inline Quaternion(double i, double j, double k, double u);

    static inline Quaternion Identity();

    static inline Quaternion Conjugate(Quaternion rotation);
    static inline double Dot(Quaternion lhs, Quaternion rhs);
    static inline double Norm(Quaternion rotation);
    static inline Quaternion Normalized(Quaternion rotation);
    static inline Quaternion Inverse(Quaternion rotation);
    static inline Quaternion Slerp(Quaternion a, Quaternion b, double t);
    static inline Quaternion SlerpUnclamped(Quaternion a, Quaternion b, double t);

    static inline Quaternion CreateFromEuler(double yaw, double pitch, double roll);
    static inline Vector3 ExtractEulerAngles(Quaternion q);

    inline struct Quaternion& operator+=(const double rhs);
    inline struct Quaternion& operator-=(const double rhs);
    inline struct Quaternion& operator*=(const double rhs);
    inline struct Quaternion& operator/=(const double rhs);
    inline struct Quaternion& operator+=(const Quaternion rhs);
    inline struct Quaternion& operator-=(const Quaternion rhs);
    inline struct Quaternion& operator*=(const Quaternion rhs);
};

inline Quaternion operator-(Quaternion rhs);
inline Quaternion operator+(Quaternion lhs, const double rhs);
inline Quaternion operator-(Quaternion lhs, const double rhs);
inline Quaternion operator*(Quaternion lhs, const double rhs);
inline Quaternion operator/(Quaternion lhs, const double rhs);
inline Quaternion operator+(const double lhs, Quaternion rhs);
inline Quaternion operator-(const double lhs, Quaternion rhs);
inline Quaternion operator*(const double lhs, Quaternion rhs);
inline Quaternion operator/(const double lhs, Quaternion rhs);
inline Quaternion operator+(Quaternion lhs, const Quaternion rhs);
inline Quaternion operator-(Quaternion lhs, const Quaternion rhs);
inline Quaternion operator*(Quaternion lhs, const Quaternion rhs);
inline Vector3 operator*(Quaternion lhs, const Vector3 rhs);
inline bool operator==(const Quaternion lhs, const Quaternion rhs);
inline bool operator!=(const Quaternion lhs, const Quaternion rhs);

Quaternion::Quaternion() : I(0), J(0), K(0), U(1)
{
}

Quaternion::Quaternion(Vector3 vector, double scalar) : I(vector.X),
J(vector.Y), K(vector.Z), U(scalar)
{
}

Quaternion::Quaternion(double i, double j, double k, double u) : I(i), J(j), K(k), U(u)
{
}

Quaternion Quaternion::Identity()
{
    return Quaternion(0, 0, 0, 1);
}

Quaternion Quaternion::Conjugate(Quaternion rotation)
{
    return Quaternion(-rotation.I, -rotation.J, -rotation.K, rotation.U);
}

double Quaternion::Dot(Quaternion lhs, Quaternion rhs)
{
    return lhs.I * rhs.I + lhs.J * rhs.J + lhs.K * rhs.K + lhs.U * rhs.U;
}

double Quaternion::Norm(Quaternion rotation)
{
    return sqrt(rotation.I * rotation.I +
        rotation.J * rotation.J +
        rotation.K * rotation.K +
        rotation.U * rotation.U);
}

Quaternion Quaternion::Normalized(Quaternion rotation)
{
    return rotation / Norm(rotation);
}

Quaternion Quaternion::Inverse(Quaternion rotation)
{
    double n = Norm(rotation);
    return Conjugate(rotation) / (n * n);
}

Quaternion Quaternion::Slerp(Quaternion a, Quaternion b, double t)
{
    if (t < 0) return Normalized(a);
    else if (t > 1) return Normalized(b);
    return SlerpUnclamped(a, b, t);
}

Quaternion Quaternion::SlerpUnclamped(Quaternion a, Quaternion b, double t)
{
    double n1;
    double n2;
    double n3 = Dot(a, b);
    bool flag = false;
    if (n3 < 0)
    {
        flag = true;
        n3 = -n3;
    }
    if (n3 > 0.999999)
    {
        n2 = 1 - t;
        n1 = flag ? -t : t;
    }
    else
    {
        double n4 = acos(n3);
        double n5 = 1 / sin(n4);
        n2 = sin((1 - t) * n4) * n5;
        n1 = flag ? -sin(t * n4) * n5 : sin(t * n4) * n5;
    }
    Quaternion quaternion;
    quaternion.I = (n2 * a.I) + (n1 * b.I);
    quaternion.J = (n2 * a.J) + (n1 * b.J);
    quaternion.K = (n2 * a.K) + (n1 * b.K);
    quaternion.U = (n2 * a.U) + (n1 * b.U);
    return Normalized(quaternion);
}

Quaternion Quaternion::CreateFromEuler(double yaw, double pitch, double roll)
{
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);

    Quaternion q;
    q.I = cy * cp * sr - sy * sp * cr;
    q.J = sy * cp * sr + cy * sp * cr;
    q.K = sy * cp * cr - cy * sp * sr;
    q.U = cy * cp * cr + sy * sp * sr;

    return q;
}

Vector3 Quaternion::ExtractEulerAngles(Quaternion rotation)
{
    double sqw = rotation.U * rotation.U;
    double sqx = rotation.I * rotation.I;
    double sqy = rotation.J * rotation.J;
    double sqz = rotation.K * rotation.K;

    double test = rotation.U * rotation.J - rotation.K * rotation.I;
    double yaw, pitch, roll;

    // Check for singularity at north pole
    if (test > 0.4999999999999999) {
        pitch = M_PI / 2;
        yaw = 2 * atan2(rotation.I, rotation.U);
        roll = 0.0;
    }
    // Check for singularity at south pole
    else if (test < -0.4999999999999999) {
        pitch = -M_PI / 2;
        yaw = -2 * atan2(rotation.I, rotation.U);
        roll = 0.0;
    }
    // Otherwise, compute pitch, yaw, and roll
    else {
        pitch = asin(2 * test);
        yaw = atan2(2 * (rotation.U * rotation.K + rotation.I * rotation.J), 1 - 2 * (sqy + sqz));
        roll = atan2(2 * (rotation.U * rotation.I + rotation.J * rotation.K), 1 - 2 * (sqx + sqy));
    }

    return Vector3(pitch, yaw, roll);
}

struct Quaternion& Quaternion::operator+=(const double rhs)
{
    I += rhs;
    J += rhs;
    K += rhs;
    U += rhs;
    return *this;
}

struct Quaternion& Quaternion::operator-=(const double rhs)
{
    I -= rhs;
    J -= rhs;
    K -= rhs;
    U -= rhs;
    return *this;
}

struct Quaternion& Quaternion::operator*=(const double rhs)
{
    I *= rhs;
    J *= rhs;
    K *= rhs;
    U *= rhs;
    return *this;
}

struct Quaternion& Quaternion::operator/=(const double rhs)
{
    I /= rhs;
    J /= rhs;
    K /= rhs;
    U /= rhs;
    return *this;
}

struct Quaternion& Quaternion::operator+=(const Quaternion rhs)
{
    I += rhs.I;
    J += rhs.J;
    K += rhs.K;
    U += rhs.U;
    return *this;
}

struct Quaternion& Quaternion::operator-=(const Quaternion rhs)
{
    I -= rhs.I;
    J -= rhs.J;
    K -= rhs.K;
    U -= rhs.U;
    return *this;
}

struct Quaternion& Quaternion::operator*=(const Quaternion rhs)
{
    Quaternion q;
    q.U = U * rhs.U - I * rhs.I - J * rhs.J - K * rhs.K;
    q.I = I * rhs.U + U * rhs.I + J * rhs.K - K * rhs.J;
    q.J = U * rhs.J - I * rhs.K + J * rhs.U + K * rhs.I;
    q.K = U * rhs.K + I * rhs.J - J * rhs.I + K * rhs.U;
    *this = q;
    return *this;
}

Quaternion operator-(Quaternion rhs)
{
    return rhs * -1;
}
Quaternion operator+(Quaternion lhs, const double rhs)
{
    return lhs += rhs;
}
Quaternion operator-(Quaternion lhs, const double rhs)
{
    return lhs -= rhs;
}
Quaternion operator*(Quaternion lhs, const double rhs)
{
    return lhs *= rhs;
}
Quaternion operator/(Quaternion lhs, const double rhs)
{
    return lhs /= rhs;
}
Quaternion operator+(const double lhs, Quaternion rhs)
{
    return rhs += lhs;
}
Quaternion operator-(const double lhs, Quaternion rhs)
{
    return rhs -= lhs;
}
Quaternion operator*(const double lhs, Quaternion rhs)
{
    return rhs *= lhs;
}
Quaternion operator/(const double lhs, Quaternion rhs)
{
    return rhs /= lhs;
}
Quaternion operator+(Quaternion lhs, const Quaternion rhs)
{
    return lhs += rhs;
}
Quaternion operator-(Quaternion lhs, const Quaternion rhs)
{
    return lhs -= rhs;
}
Quaternion operator*(Quaternion lhs, const Quaternion rhs)
{
    return lhs *= rhs;
}

Vector3 operator*(Quaternion lhs, const Vector3 rhs)
{
    Vector3 u = Vector3(lhs.I, lhs.J, lhs.K);
    double s = lhs.U;
    return u * (Vector3::Dot(u, rhs) * 2)
        + rhs * (s * s - Vector3::Dot(u, u))
        + Vector3::Cross(u, rhs) * (2.0 * s);
}

bool operator==(const Quaternion lhs, const Quaternion rhs)
{
    return lhs.I == rhs.I &&
        lhs.J == rhs.J &&
        lhs.K == rhs.K &&
        lhs.U == rhs.U;
}

bool operator!=(const Quaternion lhs, const Quaternion rhs)
{
    return !(lhs == rhs);
}
#ifndef UTILS_H
#define UTILS_H

#include <algorithm>

#include <QString>
#include <QtGlobal>
#include <QChar>
#include <QDir>

constexpr qint64 COLOR_GREEN = 0x85a664;
constexpr qint64 COLOR_ORANGE = 0xffa500;
constexpr qint64 COLOR_WHITE = 0xffffff;
constexpr qint64 COLOR_GRAY = 0xc0c0c0;
constexpr qint64 COLOR_YELLOW = 0xffff00;
constexpr qint64 COLOR_RED = 0xeb2f06;
constexpr qint64 COLOR_CYAN = 0x00ffff;
constexpr qint64 COLOR_BRIGHT_GREEN = 0x00c000;

// is 0-9 char?
inline bool is09(const QChar &c) { return c >= u'0' && c <= u'9'; }

// returns true if any character in the string matches the given predicate
template <class F> bool containsChar(const QString &s, F predicate)
{
    return std::any_of(s.begin(), s.end(), predicate);
}

inline QString pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

#endif // UTILS_H

#include <QtCore/qglobal.h>

#ifndef QOPERATINGSYSTEMVERSION_H
#define QOPERATINGSYSTEMVERSION_H

QT_BEGIN_NAMESPACE

class QString;
class QVersionNumber;

class QOperatingSystemVersion
{
public:
    enum OSType {
        Unknown = 0,
        Windows,
        MacOS,
        IOS,
        TvOS,
        WatchOS,
        Android
    };

    static const QOperatingSystemVersion Windows7;
    static const QOperatingSystemVersion Windows8;
    static const QOperatingSystemVersion Windows8_1;
    static const QOperatingSystemVersion Windows10;

    static const QOperatingSystemVersion OSXMavericks;
    static const QOperatingSystemVersion OSXYosemite;
    static const QOperatingSystemVersion OSXElCapitan;
    static const QOperatingSystemVersion MacOSSierra;
    static const QOperatingSystemVersion MacOSHighSierra;

    static const QOperatingSystemVersion AndroidJellyBean;
    static const QOperatingSystemVersion AndroidJellyBean_MR1;
    static const QOperatingSystemVersion AndroidJellyBean_MR2;
    static const QOperatingSystemVersion AndroidKitKat;
    static const QOperatingSystemVersion AndroidLollipop;
    static const QOperatingSystemVersion AndroidLollipop_MR1;
    static const QOperatingSystemVersion AndroidMarshmallow;
    static const QOperatingSystemVersion AndroidNougat;
    static const QOperatingSystemVersion AndroidNougat_MR1;
    static const QOperatingSystemVersion AndroidOreo;

    Q_DECL_CONSTEXPR QOperatingSystemVersion(OSType osType,
                                             int vmajor, int vminor = -1, int vmicro = -1)
        : m_os(osType),
          m_major(qMax(-1, vmajor)),
          m_minor(vmajor < 0 ? -1 : qMax(-1, vminor)),
          m_micro(vmajor < 0 || vminor < 0 ? -1 : qMax(-1, vmicro))
    { }

    static QOperatingSystemVersion current();

    Q_DECL_CONSTEXPR int majorVersion() const { return m_major; }
    Q_DECL_CONSTEXPR int minorVersion() const { return m_minor; }
    Q_DECL_CONSTEXPR int microVersion() const { return m_micro; }

    Q_DECL_CONSTEXPR int segmentCount() const
    { return m_micro >= 0 ? 3 : m_minor >= 0 ? 2 : m_major >= 0 ? 1 : 0; }

#ifdef Q_COMPILER_INITIALIZER_LISTS
    bool isAnyOfType(std::initializer_list<OSType> types) const;
#endif
    Q_DECL_CONSTEXPR OSType type() const { return m_os; }
    QString name() const;

    friend bool operator>(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) > 0; }

    friend bool operator>=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) >= 0; }

    friend bool operator<(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) < 0; }

    friend bool operator<=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) <= 0; }

private:
    QOperatingSystemVersion() = default;
    OSType m_os;
    int m_major;
    int m_minor;
    int m_micro;

    static int compare(const QOperatingSystemVersion &v1, const QOperatingSystemVersion &v2);
};
Q_DECLARE_TYPEINFO(QOperatingSystemVersion, QT_VERSION < QT_VERSION_CHECK(6, 0, 0) ? Q_RELOCATABLE_TYPE : Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QOPERATINGSYSTEMVERSION_H

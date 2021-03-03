#include "qplatformdefs.h"
#include "qstring.h"
#include "qvector.h"
#include "qlist.h"
#include "qthreadstorage.h"
#include "qdir.h"
#include "qdatetime.h"
#include "qoperatingsystemversion.h"
#include "qoperatingsystemversion_p.h"
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN) || defined(Q_OS_WINRT)
#include "qoperatingsystemversion_win_p.h"
#endif
#include <private/qlocale_tools_p.h>

#include <qmutex.h>

#ifndef QT_NO_QOBJECT
#include <private/qthread_p.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#ifndef QT_NO_EXCEPTIONS
#  include <string>
#  include <exception>
#endif

#include <errno.h>
#if defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif

#ifdef Q_OS_WINRT
#include <Ws2tcpip.h>
#endif // Q_OS_WINRT

#if defined(Q_OS_VXWORKS) && defined(_WRS_KERNEL)
#  include <envLib.h>
#endif

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#include <private/qjni_p.h>
#endif

#if defined(Q_OS_SOLARIS)
#  include <sys/systeminfo.h>
#endif

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#include <private/qcore_unix_p.h>
#endif

#ifdef Q_OS_BSD4
#include <sys/sysctl.h>
#endif


#include "archdetect.cpp"



QT_BEGIN_NAMESPACE

#if !QT_DEPRECATED_SINCE(5, 0)
// Make sure they're defined to be exported
Q_CORE_EXPORT void *qMemCopy(void *dest, const void *src, size_t n);
Q_CORE_EXPORT void *qMemSet(void *dest, int c, size_t n);
#endif

// Statically check assumptions about the environment we're running
// in. The idea here is to error or warn if otherwise implicit Qt
// assumptions are not fulfilled on new hardware or compilers
// (if this list becomes too long, consider factoring into a separate file)
Q_STATIC_ASSERT_X(sizeof(int) == 4, "Qt assumes that int is 32 bits");
Q_STATIC_ASSERT_X(UCHAR_MAX == 255, "Qt assumes that char is 8 bits");
Q_STATIC_ASSERT_X(QT_POINTER_SIZE == sizeof(void *), "QT_POINTER_SIZE defined incorrectly");


const char *qVersion() Q_DECL_NOTHROW
{
    return QT_VERSION_STR;
}

bool qSharedBuild() Q_DECL_NOTHROW
{
#ifdef QT_SHARED
    return true;
#else
    return false;
#endif
}

#if defined(QT_BUILD_QMAKE)
// needed to bootstrap qmake
static const unsigned int qt_one = 1;
const int QSysInfo::ByteOrder = ((*((unsigned char *) &qt_one) == 0) ? BigEndian : LittleEndian);
#endif


QString QSysInfo::buildCpuArchitecture()
{
    return QStringLiteral(ARCH_PROCESSOR);
}

QString QSysInfo::currentCpuArchitecture()
{
#if defined(Q_OS_WIN)
    // We don't need to catch all the CPU architectures in this function;
    // only those where the host CPU might be different than the build target
    // (usually, 64-bit platforms).
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
#  ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
        return QStringLiteral("x86_64");
#  endif
#  ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#  endif
    case PROCESSOR_ARCHITECTURE_IA64:
        return QStringLiteral("ia64");
    }
#elif defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)
    // iOS-based OSes do not return the architecture on uname(2)'s result.
    return buildCpuArchitecture();
#elif defined(Q_OS_UNIX)
    long ret = -1;
    struct utsname u;

#  if defined(Q_OS_SOLARIS)
    // We need a special call for Solaris because uname(2) on x86 returns "i86pc" for
    // both 32- and 64-bit CPUs. Reference:
    // http://docs.oracle.com/cd/E18752_01/html/816-5167/sysinfo-2.html#REFMAN2sysinfo-2
    // http://fxr.watson.org/fxr/source/common/syscall/systeminfo.c?v=OPENSOLARIS
    // http://fxr.watson.org/fxr/source/common/conf/param.c?v=OPENSOLARIS;im=10#L530
    if (ret == -1)
        ret = sysinfo(SI_ARCHITECTURE_64, u.machine, sizeof u.machine);
#  endif

    if (ret == -1)
        ret = uname(&u);

    // we could use detectUnixVersion() above, but we only need a field no other function does
    if (ret != -1) {
        // the use of QT_BUILD_INTERNAL here is simply to ensure all branches build
        // as we don't often build on some of the less common platforms
#  if defined(Q_PROCESSOR_ARM) || defined(QT_BUILD_INTERNAL)
        if (strcmp(u.machine, "aarch64") == 0)
            return QStringLiteral("arm64");
        if (strncmp(u.machine, "armv", 4) == 0)
            return QStringLiteral("arm");
#  endif
#  if defined(Q_PROCESSOR_POWER) || defined(QT_BUILD_INTERNAL)
        // harmonize "powerpc" and "ppc" to "power"
        if (strncmp(u.machine, "ppc", 3) == 0)
            return QLatin1String("power") + QLatin1String(u.machine + 3);
        if (strncmp(u.machine, "powerpc", 7) == 0)
            return QLatin1String("power") + QLatin1String(u.machine + 7);
        if (strcmp(u.machine, "Power Macintosh") == 0)
            return QLatin1String("power");
#  endif
#  if defined(Q_PROCESSOR_SPARC) || defined(QT_BUILD_INTERNAL)
        // Solaris sysinfo(2) (above) uses "sparcv9", but uname -m says "sun4u";
        // Linux says "sparc64"
        if (strcmp(u.machine, "sun4u") == 0 || strcmp(u.machine, "sparc64") == 0)
            return QStringLiteral("sparcv9");
        if (strcmp(u.machine, "sparc32") == 0)
            return QStringLiteral("sparc");
#  endif
#  if defined(Q_PROCESSOR_X86) || defined(QT_BUILD_INTERNAL)
        // harmonize all "i?86" to "i386"
        if (strlen(u.machine) == 4 && u.machine[0] == 'i'
                && u.machine[2] == '8' && u.machine[3] == '6')
            return QStringLiteral("i386");
        if (strcmp(u.machine, "amd64") == 0) // Solaris
            return QStringLiteral("x86_64");
#  endif
        return QString::fromLatin1(u.machine);
    }
#endif
    return buildCpuArchitecture();
}

QString QSysInfo::buildAbi()
{
#ifdef Q_COMPILER_UNICODE_STRINGS
    // ARCH_FULL is a concatenation of strings (incl. ARCH_PROCESSOR), which breaks
    // QStringLiteral on MSVC. Since the concatenation behavior we want is specified
    // the same C++11 paper as the Unicode strings, we'll use that macro and hope
    // that Microsoft implements the new behavior when they add support for Unicode strings.
    return QStringLiteral(ARCH_FULL);
#else
    return QLatin1String(ARCH_FULL);
#endif
}

static QString unknownText()
{
    return QStringLiteral("unknown");
}

QString QSysInfo::kernelType()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("winnt");
#elif defined(Q_OS_UNIX)
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.sysname).toLower();
#endif
    return unknownText();
}

QString QSysInfo::kernelVersion()
{
#ifdef Q_OS_WIN
    const auto osver = QOperatingSystemVersion::current();
    return QString::number(osver.majorVersion()) + QLatin1Char('.') + QString::number(osver.minorVersion())
            + QLatin1Char('.') + QString::number(osver.microVersion());
#else
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.release);
    return QString();
#endif
}


QString QSysInfo::productType()
{
    // similar, but not identical to QFileSelectorPrivate::platformSelectors
#if defined(Q_OS_WINRT)
    return QStringLiteral("winrt");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");

#elif defined(Q_OS_QNX)
    return QStringLiteral("qnx");

#elif defined(Q_OS_ANDROID)
    return QStringLiteral("android");

#elif defined(Q_OS_IOS)
    return QStringLiteral("ios");
#elif defined(Q_OS_TVOS)
    return QStringLiteral("tvos");
#elif defined(Q_OS_WATCHOS)
    return QStringLiteral("watchos");
#elif defined(Q_OS_MACOS)
    // ### Qt6: remove fallback
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QStringLiteral("macos");
#  else
    return QStringLiteral("osx");
#  endif
#elif defined(Q_OS_DARWIN)
    return QStringLiteral("darwin");

#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.productType.isEmpty())
        return unixOsVersion.productType;
#endif
    return unknownText();
}

QString QSysInfo::productVersion()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN)
    const auto version = QOperatingSystemVersion::current();
    return QString::number(version.majorVersion()) + QLatin1Char('.') + QString::number(version.minorVersion());
#elif defined(Q_OS_WIN)
    const char *version = osVer_helper();
    if (version) {
        const QLatin1Char spaceChar(' ');
        return QString::fromLatin1(version).remove(spaceChar).toLower() + winSp_helper().remove(spaceChar).toLower();
    }
    // fall through

#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.productVersion.isEmpty())
        return unixOsVersion.productVersion;
#endif

    // fallback
    return unknownText();
}

QString QSysInfo::prettyProductName()
{
#if (defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)) || defined(Q_OS_DARWIN) || defined(Q_OS_WIN)
    const auto version = QOperatingSystemVersion::current();
    const char *name = osVer_helper(version);
    if (name)
        return version.name() + QLatin1Char(' ') + QLatin1String(name)
#    if defined(Q_OS_WIN)
            + winSp_helper()
#    endif
            + QLatin1String(" (") + QString::number(version.majorVersion())
            + QLatin1Char('.') + QString::number(version.minorVersion())
            + QLatin1Char(')');
      else
        return version.name() + QLatin1Char(' ')
            + QString::number(version.majorVersion()) + QLatin1Char('.')
            + QString::number(version.minorVersion());
#elif defined(Q_OS_HAIKU)
    return QLatin1String("Haiku ") + productVersion();
#elif defined(Q_OS_UNIX)
#  ifdef USE_ETC_OS_RELEASE
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.prettyName.isEmpty())
        return unixOsVersion.prettyName;
#  endif
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.sysname) + QLatin1Char(' ') + QString::fromLatin1(u.release);
#endif
    return unknownText();
}


QString QSysInfo::machineHostName()
{
#if defined(Q_OS_LINUX)
    // gethostname(3) on Linux just calls uname(2), so do it ourselves
    // and avoid a memcpy
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLocal8Bit(u.nodename);
#else
#  ifdef Q_OS_WIN
    // Important: QtNetwork depends on machineHostName() initializing ws2_32.dll
    winsockInit();
#  endif

    char hostName[512];
    if (gethostname(hostName, sizeof(hostName)) == -1)
        return QString();
    hostName[sizeof(hostName) - 1] = '\0';
    return QString::fromLocal8Bit(hostName);
#endif
    return QString();
}


void qt_check_pointer(const char *n, int l) Q_DECL_NOTHROW
{
    // make separate printing calls so that the first one may flush;
    // the second one could want to allocate memory (fputs prints a
    // newline and stderr auto-flushes).
    fputs("Out of memory", stderr);
    fprintf(stderr, "  in %s, line %d\n", n, l);

    std::terminate();
}

void qBadAlloc()
{
    QT_THROW(std::bad_alloc());
}

#ifndef QT_NO_EXCEPTIONS
/*
   \internal
   Allows you to call std::terminate() without including <exception>.
   Called internally from QT_TERMINATE_ON_EXCEPTION
*/
Q_NORETURN void qTerminate() Q_DECL_NOTHROW
{
    std::terminate();
}
#endif

/*
  The Q_ASSERT macro calls this function when the test fails.
*/
void qt_assert(const char *assertion, const char *file, int line) Q_DECL_NOTHROW
{
    QMessageLogger(file, line, nullptr).fatal("ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
}

/*
  The Q_ASSERT_X macro calls this function when the test fails.
*/
void qt_assert_x(const char *where, const char *what, const char *file, int line) Q_DECL_NOTHROW
{
    QMessageLogger(file, line, nullptr).fatal("ASSERT failure in %s: \"%s\", file %s, line %d", where, what, file, line);
}


/*
    Dijkstra's bisection algorithm to find the square root of an integer.
    Deliberately not exported as part of the Qt API, but used in both
    qsimplerichtext.cpp and qgfxraster_qws.cpp
*/
Q_CORE_EXPORT unsigned int qt_int_sqrt(unsigned int n)
{
    // n must be in the range 0...UINT_MAX/2-1
    if (n >= (UINT_MAX>>2)) {
        unsigned int r = 2 * qt_int_sqrt(n / 4);
        unsigned int r2 = r + 1;
        return (n >= r2 * r2) ? r2 : r;
    }
    uint h, p= 0, q= 1, r= n;
    while (q <= n)
        q <<= 2;
    while (q != 1) {
        q >>= 2;
        h= p + q;
        p >>= 1;
        if (r >= h) {
            p += q;
            r -= h;
        }
    }
    return p;
}

void *qMemCopy(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void *qMemSet(void *dest, int c, size_t n) { return memset(dest, c, n); }

// In the C runtime on all platforms access to the environment is not thread-safe. We
// add thread-safety for the Qt wrappers.
static QBasicMutex environmentMutex;

QByteArray qgetenv(const char *varName)
{
    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    QByteArray buffer;
    getenv_s(&requiredSize, 0, 0, varName);
    if (requiredSize == 0)
        return buffer;
    buffer.resize(int(requiredSize));
    getenv_s(&requiredSize, buffer.data(), requiredSize, varName);
    // requiredSize includes the terminating null, which we don't want.
    Q_ASSERT(buffer.endsWith('\0'));
    buffer.chop(1);
    return buffer;
#else
    return QByteArray(::getenv(varName));
#endif
}

bool qEnvironmentVariableIsEmpty(const char *varName) Q_DECL_NOEXCEPT
{
    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    // we provide a buffer that can only hold the empty string, so
    // when the env.var isn't empty, we'll get an ERANGE error (buffer
    // too small):
    size_t dummy;
    char buffer = '\0';
    return getenv_s(&dummy, &buffer, 1, varName) != ERANGE;
#else
    const char * const value = ::getenv(varName);
    return !value || !*value;
#endif
}

/*!
    \relates <QtGlobal>
    \since 5.5

    Returns the numerical value of the environment variable \a varName.
    If \a ok is not null, sets \c{*ok} to \c true or \c false depending
    on the success of the conversion.

    Equivalent to
    \code
    qgetenv(varName).toInt(ok, 0)
    \endcode
    except that it's much faster, and can't throw exceptions.

    \note there's a limit on the length of the value, which is sufficient for
    all valid values of int, not counting leading zeroes or spaces. Values that
    are too long will either be truncated or this function will set \a ok to \c
    false.

    \sa qgetenv(), qEnvironmentVariableIsSet()
*/
int qEnvironmentVariableIntValue(const char *varName, bool *ok) Q_DECL_NOEXCEPT
{
    static const int NumBinaryDigitsPerOctalDigit = 3;
    static const int MaxDigitsForOctalInt =
        (std::numeric_limits<uint>::digits + NumBinaryDigitsPerOctalDigit - 1) / NumBinaryDigitsPerOctalDigit;

    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    // we provide a buffer that can hold any int value:
    char buffer[MaxDigitsForOctalInt + 2]; // +1 for NUL +1 for optional '-'
    size_t dummy;
    if (getenv_s(&dummy, buffer, sizeof buffer, varName) != 0) {
        if (ok)
            *ok = false;
        return 0;
    }
#else
    const char * const buffer = ::getenv(varName);
    if (!buffer || strlen(buffer) > MaxDigitsForOctalInt + 2) {
        if (ok)
            *ok = false;
        return 0;
    }
#endif
    bool ok_ = true;
    const char *endptr;
    const qlonglong value = qstrtoll(buffer, &endptr, 0, &ok_);
    if (int(value) != value || *endptr != '\0') { // this is the check in QByteArray::toInt(), keep it in sync
        if (ok)
            *ok = false;
        return 0;
    } else if (ok) {
        *ok = ok_;
    }
    return int(value);
}

/*!
    \relates <QtGlobal>
    \since 5.1

    Returns whether the environment variable \a varName is set.

    Equivalent to
    \code
    !qgetenv(varName).isNull()
    \endcode
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariableIsEmpty()
*/
bool qEnvironmentVariableIsSet(const char *varName) Q_DECL_NOEXCEPT
{
    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    (void)getenv_s(&requiredSize, 0, 0, varName);
    return requiredSize != 0;
#else
    return ::getenv(varName) != 0;
#endif
}

/*!
    \relates <QtGlobal>

    This function sets the \a value of the environment variable named
    \a varName. It will create the variable if it does not exist. It
    returns 0 if the variable could not be set.

    Calling qputenv with an empty value removes the environment variable on
    Windows, and makes it set (but empty) on Unix. Prefer using qunsetenv()
    for fully portable behavior.

    \note qputenv() was introduced because putenv() from the standard
    C library was deprecated in VC2005 (and later versions). qputenv()
    uses the replacement function in VC, and calls the standard C
    library's implementation on all other platforms.

    \sa qgetenv()
*/
bool qputenv(const char *varName, const QByteArray& value)
{
    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return _putenv_s(varName, value.constData()) == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_HAIKU)
    // POSIX.1-2001 has setenv
    return setenv(varName, value.constData(), true) == 0;
#else
    QByteArray buffer(varName);
    buffer += '=';
    buffer += value;
    char* envVar = qstrdup(buffer.constData());
    int result = putenv(envVar);
    if (result != 0) // error. we have to delete the string.
        delete[] envVar;
    return result == 0;
#endif
}

/*!
    \relates <QtGlobal>

    This function deletes the variable \a varName from the environment.

    Returns \c true on success.

    \since 5.1

    \sa qputenv(), qgetenv()
*/
bool qunsetenv(const char *varName)
{
    QMutexLocker locker(&environmentMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return _putenv_s(varName, "") == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_BSD4) || defined(Q_OS_HAIKU)
    // POSIX.1-2001, BSD and Haiku have unsetenv
    return unsetenv(varName) == 0;
#elif defined(Q_CC_MINGW)
    // On mingw, putenv("var=") removes "var" from the environment
    QByteArray buffer(varName);
    buffer += '=';
    return putenv(buffer.constData()) == 0;
#else
    // Fallback to putenv("var=") which will insert an empty var into the
    // environment and leak it
    QByteArray buffer(varName);
    buffer += '=';
    char *envVar = qstrdup(buffer.constData());
    return putenv(envVar) == 0;
#endif
}

void qsrand(uint seed)
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED) && (__ANDROID_API__ < 21)
    if (randomTLS->hasLocalData()) {
        randomTLS->localData().callMethod<void>("setSeed", "(J)V", jlong(seed));
        return;
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(seed));
    if (!random.isValid()) {
        srand(seed);
        return;
    }

    randomTLS->setLocalData(random);
#elif defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed)
            seedStorage->setLocalData(pseed = new SeedStorageType);
        *pseed = seed;
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to srand(seed)
        srand(seed);
    }
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    srand(seed);
#endif
}


int qrand()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED) && (__ANDROID_API__ < 21)
    AndroidRandomStorage *randomStorage = randomTLS();
    if (!randomStorage)
        return rand();

    if (randomStorage->hasLocalData()) {
        return randomStorage->localData().callMethod<jint>("nextInt",
                                                           "(I)I",
                                                           RAND_MAX);
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(1));

    if (!random.isValid())
        return rand();

    randomStorage->setLocalData(random);
    return random.callMethod<jint>("nextInt", "(I)I", RAND_MAX);
#elif defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed) {
            seedStorage->setLocalData(pseed = new SeedStorageType);
            *pseed = 1;
        }
        return rand_r(pseed);
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to rand()
        return rand();
    }
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    return rand();
#endif
}


struct QInternal_CallBackTable {
    QVector<QList<qInternalCallback> > callbacks;
};

Q_GLOBAL_STATIC(QInternal_CallBackTable, global_callback_table)

bool QInternal::registerCallback(Callback cb, qInternalCallback callback)
{
    if (cb >= 0 && cb < QInternal::LastCallback) {
        QInternal_CallBackTable *cbt = global_callback_table();
        cbt->callbacks.resize(cb + 1);
        cbt->callbacks[cb].append(callback);
        return true;
    }
    return false;
}

bool QInternal::unregisterCallback(Callback cb, qInternalCallback callback)
{
    if (cb >= 0 && cb < QInternal::LastCallback) {
        if (global_callback_table.exists()) {
            QInternal_CallBackTable *cbt = global_callback_table();
            return (bool) cbt->callbacks[cb].removeAll(callback);
        }
    }
    return false;
}

bool QInternal::activateCallbacks(Callback cb, void **parameters)
{
    Q_ASSERT_X(cb >= 0, "QInternal::activateCallback()", "Callback id must be a valid id");

    if (!global_callback_table.exists())
        return false;

    QInternal_CallBackTable *cbt = &(*global_callback_table);
    if (cbt && cb < cbt->callbacks.size()) {
        QList<qInternalCallback> callbacks = cbt->callbacks[cb];
        bool ret = false;
        for (int i=0; i<callbacks.size(); ++i)
            ret |= (callbacks.at(i))(parameters);
        return ret;
    }
    return false;
}

QT_END_NAMESPACE

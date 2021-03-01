#ifndef QEVENTDISPATCHER_UNIX_P_H
#define QEVENTDISPATCHER_UNIX_P_H

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qlist.h"
#include "private/qabstracteventdispatcher_p.h"
#include "private/qcore_unix_p.h"
#include "private/qpodlist_p.h"
#include "QtCore/qvarlengtharray.h"
#include "private/qtimerinfo_unix_p.h"

QT_BEGIN_NAMESPACE

class QEventDispatcherUNIXPrivate;

struct Q_CORE_EXPORT QSocketNotifierSetUNIX Q_DECL_FINAL
{
    inline QSocketNotifierSetUNIX() Q_DECL_NOTHROW;

    inline bool isEmpty() const Q_DECL_NOTHROW;
    inline short events() const Q_DECL_NOTHROW;

    QSocketNotifier *notifiers[3];
};

Q_DECLARE_TYPEINFO(QSocketNotifierSetUNIX, Q_PRIMITIVE_TYPE);

struct QThreadPipe
{
    QThreadPipe();
    ~QThreadPipe();

    bool init();
    pollfd prepare() const;

    void wakeUp();
    int check(const pollfd &pfd);

    // note for eventfd(7) support:
    // if fds[1] is -1, then eventfd(7) is in use and is stored in fds[0]
    int fds[2];
    QAtomicInt wakeUps;

#if defined(Q_OS_VXWORKS)
    static const int len_name = 20;
    char name[len_name];
#endif
};

class Q_CORE_EXPORT QEventDispatcherUNIX : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherUNIX)

public:
    explicit QEventDispatcherUNIX(QObject *parent = 0);
    ~QEventDispatcherUNIX();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) Q_DECL_OVERRIDE;
    bool hasPendingEvents() Q_DECL_OVERRIDE;

    void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;
    void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_FINAL;
    bool unregisterTimer(int timerId) Q_DECL_FINAL;
    bool unregisterTimers(QObject *object) Q_DECL_FINAL;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_FINAL;

    int remainingTime(int timerId) Q_DECL_FINAL;

    void wakeUp() Q_DECL_FINAL;
    void interrupt() Q_DECL_FINAL;
    void flush() Q_DECL_OVERRIDE;

protected:
    QEventDispatcherUNIX(QEventDispatcherUNIXPrivate &dd, QObject *parent = 0);
};

class Q_CORE_EXPORT QEventDispatcherUNIXPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherUNIX)

public:
    QEventDispatcherUNIXPrivate();
    ~QEventDispatcherUNIXPrivate();

    int activateTimers();

    void markPendingSocketNotifiers();
    int activateSocketNotifiers();
    void setSocketNotifierPending(QSocketNotifier *notifier);

    QThreadPipe threadPipe;
    QVector<pollfd> pollfds;

    QHash<int, QSocketNotifierSetUNIX> socketNotifiers;
    QVector<QSocketNotifier *> pendingNotifiers;

    QTimerInfoList timerList;
    QAtomicInt interrupt; // bool
};

inline QSocketNotifierSetUNIX::QSocketNotifierSetUNIX() Q_DECL_NOTHROW
{
    notifiers[0] = 0;
    notifiers[1] = 0;
    notifiers[2] = 0;
}

inline bool QSocketNotifierSetUNIX::isEmpty() const Q_DECL_NOTHROW
{
    return !notifiers[0] && !notifiers[1] && !notifiers[2];
}

inline short QSocketNotifierSetUNIX::events() const Q_DECL_NOTHROW
{
    short result = 0;

    if (notifiers[0])
        result |= POLLIN;

    if (notifiers[1])
        result |= POLLOUT;

    if (notifiers[2])
        result |= POLLPRI;

    return result;
}

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_UNIX_P_H

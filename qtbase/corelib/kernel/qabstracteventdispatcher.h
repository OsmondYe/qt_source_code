#ifndef QABSTRACTEVENTDISPATCHER_H
#define QABSTRACTEVENTDISPATCHER_H

#include <QtCore/qobject.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

class QAbstractNativeEventFilter;
class QAbstractEventDispatcherPrivate;
class QSocketNotifier;

class QWinEventNotifier;


//for windows, using QEventDispatcherWin32 
class QAbstractEventDispatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractEventDispatcher)

public:
    struct TimerInfo
    {
        int timerId;
        int interval;
        Qt::TimerType timerType;

        inline TimerInfo(int id, int i, Qt::TimerType t)
            : timerId(id), interval(i), timerType(t)
        { }
    };

    explicit QAbstractEventDispatcher(QObject *parent = Q_NULLPTR);
    ~QAbstractEventDispatcher();

    static QAbstractEventDispatcher *instance(QThread *thread = Q_NULLPTR);

    virtual bool processEvents(QEventLoop::ProcessEventsFlags flags) = 0;
    virtual bool hasPendingEvents() = 0; // ### Qt6: remove, mark final or make protected

    virtual void registerSocketNotifier(QSocketNotifier *notifier) = 0;
    virtual void unregisterSocketNotifier(QSocketNotifier *notifier) = 0;

    int registerTimer(int interval, Qt::TimerType timerType, QObject *object);
    virtual void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) = 0;
    virtual bool unregisterTimer(int timerId) = 0;
    virtual bool unregisterTimers(QObject *object) = 0;
    virtual QList<TimerInfo> registeredTimers(QObject *object) const = 0;

    virtual int remainingTime(int timerId) = 0;


    virtual bool registerEventNotifier(QWinEventNotifier *notifier) = 0;
    virtual void unregisterEventNotifier(QWinEventNotifier *notifier) = 0;


    virtual void wakeUp() = 0;
    virtual void interrupt() = 0;
    virtual void flush() = 0; // ### Qt6: remove, mark final or make protected

    virtual void startingUp();
    virtual void closingDown();

    void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    bool filterNativeEvent(const QByteArray &eventType, void *message, long *result);

Q_SIGNALS:
    void aboutToBlock();
    void awake();

protected:
    QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &,
                             QObject *parent);
};

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_H

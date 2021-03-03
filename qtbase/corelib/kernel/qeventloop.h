#ifndef QEVENTLOOP_H
#define QEVENTLOOP_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QEventLoopPrivate : public QObjectPrivate
{
    //Q_DECLARE_PUBLIC(QEventLoop)
public:

	QBasicAtomicInteger<int> exit; 
	QBasicAtomicInteger<int> returnCode;
	bool inExec;
	QBasicAtomicInteger<int> quitLockRef;
//////
public:

    QEventLoopPrivate() : inExec(false)
    {
        returnCode.store(-1);
        exit.store(true);
    }

    void ref()    {        quitLockRef.ref();    }

    void deref()
    {
        if (!quitLockRef.deref() && inExec) {
            QCoreApplication::instance()->postEvent(q_ptr, new QEvent(QEvent::Quit));
        }
    }
};


class  QEventLoop : public QObject
{
    //Q_OBJECT
    //Q_DECLARE_PRIVATE(QEventLoop)

public:
    explicit QEventLoop(QObject *parent = NULL);
    ~QEventLoop();

    enum ProcessEventsFlag {
        AllEvents = 0x00,
        ExcludeUserInputEvents = 0x01,
        ExcludeSocketNotifiers = 0x02,
        WaitForMoreEvents = 0x04,
        X11ExcludeTimers = 0x08,
        EventLoopExec = 0x20,
        DialogExec = 0x40
    };
    //Q_DECLARE_FLAGS(ProcessEventsFlags, ProcessEventsFlag)

    bool processEvents(ProcessEventsFlags flags = AllEvents);
    void processEvents(ProcessEventsFlags flags, int maximumTime);

    int exec(ProcessEventsFlags flags = AllEvents);
    void exit(int returnCode = 0);
    bool isRunning() const;

    void wakeUp();

    bool event(QEvent *event) ; // override from QObject

public Q_SLOTS:
    void quit();
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(QEventLoop::ProcessEventsFlags)


class QEventLoopLockerPrivate;

class  QEventLoopLocker
{
	QEventLoopLockerPrivate *d_ptr;

public:
    QEventLoopLocker();
    explicit QEventLoopLocker(QEventLoop *loop);
    explicit QEventLoopLocker(QThread *thread);
    ~QEventLoopLocker();

private:
    Q_DISABLE_COPY(QEventLoopLocker)

};

QT_END_NAMESPACE

#endif // QEVENTLOOP_H

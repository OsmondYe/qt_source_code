#include "qeventloop.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qelapsedtimer.h"

#include "qobject_p.h"
#include "qeventloop_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE




QEventLoop::QEventLoop(QObject *parent)
    : QObject(*new QEventLoopPrivate, parent)
{
    QEventLoopPrivate * const d = d_func();
    if (!QCoreApplication::instance() && QCoreApplicationPrivate::threadRequiresCoreApplication()) {
        qWarning("QEventLoop: Cannot be used without QApplication");
    } else if (!d->threadData->eventDispatcher.load()) {
        QThreadPrivate::createEventDispatcher(d->threadData);
    }
}

/*!
    Destroys the event loop object.
*/
QEventLoop::~QEventLoop()
{ }



bool QEventLoop::processEvents(ProcessEventsFlags flags)
{
    QEventLoopPrivate * const d = d_func();
    if (!d->threadData->eventDispatcher.load())
        return false;
    return d->threadData->eventDispatcher.load()->processEvents(flags);
}

/*!
    Enters the main event loop and waits until exit() is called.
    Returns the value that was passed to exit().

    If  flags are specified, only events of the types allowed by
    the  flags will be processed.

    It is necessary to call this function to start event handling. The
    main event loop receives events from the window system and
    dispatches these to the application widgets.

    Generally speaking, no user interaction can take place before
    calling exec(). As a special case, modal widgets like QMessageBox
    can be used before calling exec(), because modal widgets
    use their own local event loop.

    To make your application perform idle processing , use a
    QTimer with 0 timeout. More sophisticated idle processing schemes
    can be achieved using processEvents().

    \sa QCoreApplication::quit(), exit(), processEvents()
*/
int QEventLoop::exec(ProcessEventsFlags flags)
{
    QEventLoopPrivate * const d = d_func();
    //we need to protect from race condition with QThread::exit
    QMutexLocker locker(&static_cast<QThreadPrivate *>(QObjectPrivate::get(d->threadData->thread))->mutex);
    if (d->threadData->quitNow)
        return -1;

    if (d->inExec) {
        qWarning("QEventLoop::exec: instance %p has already called exec()", this);
        return -1;
    }

    struct LoopReference {
        QEventLoopPrivate *d;
        QMutexLocker &locker;

        bool exceptionCaught;
        LoopReference(QEventLoopPrivate *d, QMutexLocker &locker) : d(d), locker(locker), exceptionCaught(true)
        {
            d->inExec = true;
            d->exit.storeRelease(false);
            ++d->threadData->loopLevel;
            d->threadData->eventLoops.push(d->q_func());
            locker.unlock();
        }

        ~LoopReference()
        {
            if (exceptionCaught) {
                qWarning("Qt has caught an exception thrown from an event handler. Throwing\n"
                         "exceptions from an event handler is not supported in Qt.\n"
                         "You must not let any exception whatsoever propagate through Qt code.\n"
                         "If that is not possible, in Qt 5 you must at least reimplement\n"
                         "QCoreApplication::notify() and catch all exceptions there.\n");
            }
            locker.relock();
            QEventLoop *eventLoop = d->threadData->eventLoops.pop();
            Q_ASSERT_X(eventLoop == d->q_func(), "QEventLoop::exec()", "internal error");
            Q_UNUSED(eventLoop); // --release warning
            d->inExec = false;
            --d->threadData->loopLevel;
        }
    };
    LoopReference ref(d, locker);

    // remove posted quit events when entering a new event loop
    QCoreApplication *app = QCoreApplication::instance();
    if (app && app->thread() == thread())
        QCoreApplication::removePostedEvents(app, QEvent::Quit);

    while (!d->exit.loadAcquire())
        processEvents(flags | WaitForMoreEvents | EventLoopExec);

    ref.exceptionCaught = false;
    return d->returnCode.load();
}

/*!
    Process pending events that match \a flags for a maximum of \a
    maxTime milliseconds, or until there are no more events to
    process, whichever is shorter.
    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input, i.e. by using the \l ExcludeUserInputEvents flag.

    \b{Notes:}
    \list
    \li This function does not process events continuously; it
       returns after all available events are processed.
    \li Specifying the \l WaitForMoreEvents flag makes no sense
       and will be ignored.
    \endlist
*/
void QEventLoop::processEvents(ProcessEventsFlags flags, int maxTime)
{
    QEventLoopPrivate * const d = d_func();
    if (!d->threadData->eventDispatcher.load())
        return;

    QElapsedTimer start;
    start.start();
    while (processEvents(flags & ~WaitForMoreEvents)) {
        if (start.elapsed() > maxTime)
            break;
    }
}

/*!
    Tells the event loop to exit with a return code.

    After this function has been called, the event loop returns from
    the call to exec(). The exec() function returns \a returnCode.

    By convention, a \a returnCode of 0 means success, and any non-zero
    value indicates an error.

    Note that unlike the C library function of the same name, this
    function \e does return to the caller -- it is event processing that
    stops.

    \sa QCoreApplication::quit(), quit(), exec()
*/
void QEventLoop::exit(int returnCode)
{
    QEventLoopPrivate * const d = d_func();
    if (!d->threadData->eventDispatcher.load())
        return;

    d->returnCode.store(returnCode);
    d->exit.storeRelease(true);
    d->threadData->eventDispatcher.load()->interrupt();
}

/*!
    Returns \c true if the event loop is running; otherwise returns
    false. The event loop is considered running from the time when
    exec() is called until exit() is called.

    \sa exec(), exit()
 */
bool QEventLoop::isRunning() const
{
    Q_D(const QEventLoop);
    return !d->exit.loadAcquire();
}

/*!
    Wakes up the event loop.

    \sa QAbstractEventDispatcher::wakeUp()
*/
void QEventLoop::wakeUp()
{
    QEventLoopPrivate * const d = d_func();
    if (!d->threadData->eventDispatcher.load())
        return;
    d->threadData->eventDispatcher.load()->wakeUp();
}


/*!
    \reimp
*/
bool QEventLoop::event(QEvent *event)
{
    if (event->type() == QEvent::Quit) {
        quit();
        return true;
    } else {
        return QObject::event(event);
    }
}

/*!
    Tells the event loop to exit normally.

    Same as exit(0).

    \sa QCoreApplication::quit(), exit()
*/
void QEventLoop::quit()
{ exit(0); }


class QEventLoopLockerPrivate
{
public:
    explicit QEventLoopLockerPrivate(QEventLoopPrivate *loop)
      : loop(loop), type(EventLoop)
    {
        loop->ref();
    }

    explicit QEventLoopLockerPrivate(QThreadPrivate *thread)
      : thread(thread), type(Thread)
    {
        thread->ref();
    }

    explicit QEventLoopLockerPrivate(QCoreApplicationPrivate *app)
      : app(app), type(Application)
    {
        app->ref();
    }

    ~QEventLoopLockerPrivate()
    {
        switch (type)
        {
        case EventLoop:
            loop->deref();
            break;
        case Thread:
            thread->deref();
            break;
        default:
            app->deref();
            break;
        }
    }

private:
    union {
        QEventLoopPrivate * loop;
        QThreadPrivate * thread;
        QCoreApplicationPrivate * app;
    };
    enum Type {
        EventLoop,
        Thread,
        Application
    };
    const Type type;
};

/*!
    \class QEventLoopLocker
    \inmodule QtCore
    \brief The QEventLoopLocker class provides a means to quit an event loop when it is no longer needed.
    \since 5.0

    The QEventLoopLocker operates on particular objects - either a QCoreApplication
    instance, a QEventLoop instance or a QThread instance.

    This makes it possible to, for example, run a batch of jobs with an event loop
    and exit that event loop after the last job is finished. That is accomplished
    by keeping a QEventLoopLocker with each job instance.

    The variant which operates on QCoreApplication makes it possible to finish
    asynchronously running jobs after the last gui window has been closed. This
    can be useful for example for running a job which uploads data to a network.

    \sa QEventLoop, QCoreApplication
*/

/*!
    Creates an event locker operating on the QCoreApplication.

    The application will quit when there are no more QEventLoopLockers operating on it.

    \sa QCoreApplication::quit(), QCoreApplication::isQuitLockEnabled()
 */
QEventLoopLocker::QEventLoopLocker()
  : d_ptr(new QEventLoopLockerPrivate(static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()))))
{

}

/*!
    Creates an event locker operating on the \a loop.

    This particular QEventLoop will quit when there are no more QEventLoopLockers operating on it.

    \sa QEventLoop::quit()
 */
QEventLoopLocker::QEventLoopLocker(QEventLoop *loop)
  : d_ptr(new QEventLoopLockerPrivate(static_cast<QEventLoopPrivate*>(QObjectPrivate::get(loop))))
{

}

/*!
    Creates an event locker operating on the \a thread.

    This particular QThread will quit when there are no more QEventLoopLockers operating on it.

    \sa QThread::quit()
 */
QEventLoopLocker::QEventLoopLocker(QThread *thread)
  : d_ptr(new QEventLoopLockerPrivate(static_cast<QThreadPrivate*>(QObjectPrivate::get(thread))))
{

}

/*!
    Destroys this event loop locker object
 */
QEventLoopLocker::~QEventLoopLocker()
{
    delete d_ptr;
}

QT_END_NAMESPACE

#include "moc_qeventloop.cpp"

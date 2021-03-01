#include "qthread.h"
#include "qthreadstorage.h"
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qabstracteventdispatcher.h"

#include <qeventloop.h>

#include "qthread_p.h"
#include "private/qcoreapplication_p.h"

QT_BEGIN_NAMESPACE

/*
  QThreadData
*/

QThreadData::QThreadData(int initialRefCount)
    : _ref(initialRefCount), loopLevel(0), scopeLevel(0),
      eventDispatcher(0),
      quitNow(false), canWait(true), isAdopted(false), requiresCoreApplication(true)
{
    // fprintf(stderr, "QThreadData %p created\n", this);
}

QThreadData::~QThreadData()
{
    Q_ASSERT(_ref.load() == 0);

    // In the odd case that Qt is running on a secondary thread, the main
    // thread instance will have been dereffed asunder because of the deref in
    // QThreadData::current() and the deref in the pthread_destroy. To avoid
    // crashing during QCoreApplicationData's global static cleanup we need to
    // safeguard the main thread here.. This fix is a bit crude, but it solves
    // the problem...
    if (this->thread == QCoreApplicationPrivate::theMainThread) {
       QCoreApplicationPrivate::theMainThread = 0;
       QThreadData::clearCurrentThreadData();
    }

    QThread *t = thread;
    thread = 0;
    delete t;

    for (int i = 0; i < postEventList.size(); ++i) {
        const QPostEvent &pe = postEventList.at(i);
        if (pe.event) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
        }
    }

    // fprintf(stderr, "QThreadData %p destroyed\n", this);
}

void QThreadData::ref()
{
#ifndef QT_NO_THREAD
    (void) _ref.ref();
    Q_ASSERT(_ref.load() != 0);
#endif
}

void QThreadData::deref()
{
#ifndef QT_NO_THREAD
    if (!_ref.deref())
        delete this;
#endif
}

/*
  QAdoptedThread
*/

QAdoptedThread::QAdoptedThread(QThreadData *data)
    : QThread(*new QThreadPrivate(data))
{
    // thread should be running and not finished for the lifetime
    // of the application (even if QCoreApplication goes away)
#ifndef QT_NO_THREAD
    d_func()->running = true;
    d_func()->finished = false;
    init();
#endif

    // fprintf(stderr, "new QAdoptedThread = %p\n", this);
}

QAdoptedThread::~QAdoptedThread()
{
    // fprintf(stderr, "~QAdoptedThread = %p\n", this);
}

void QAdoptedThread::run()
{
    // this function should never be called
    qFatal("QAdoptedThread::run(): Internal error, this implementation should never be called.");
}



QThreadPrivate::QThreadPrivate(QThreadData *d)
    : QObjectPrivate(), running(false), finished(false),
      isInFinish(false), interruptionRequested(false),
      exited(false), returnCode(-1),
      stackSize(0), priority(QThread::InheritPriority), data(d)
{

// INTEGRITY doesn't support self-extending stack. The default stack size for
// a pthread on INTEGRITY is too small so we have to increase the default size
// to 128K.

    stackSize = 128 * 1024;


#if defined (Q_OS_WIN)
    handle = 0;
    id = 0;
#endif
    waiters = 0;
    terminationEnabled = true;
    terminatePending = false;
#endif

    if (!data)
        data = new QThreadData;
}

QThreadPrivate::~QThreadPrivate()
{
    data->deref();
}

QThread *QThread::currentThread()
{
    QThreadData *data = QThreadData::current();
    return data->thread;
}


QThread::QThread(QObject *parent)
    : QObject(*(new QThreadPrivate), parent)
{
    QThreadPrivate * const d = d_func();
    d->data->thread = this;
}


QThread::QThread(QThreadPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    QThreadPrivate * const d = d_func();    
    d->data->thread = this;
}


QThread::~QThread()
{
    QThreadPrivate * const d = d_func();
    {
        QMutexLocker locker(&d->mutex);
        if (d->isInFinish) {
            locker.unlock();
            wait();
            locker.relock();
        }
        if (d->running && !d->finished && !d->data->isAdopted)
            qFatal("QThread: Destroyed while thread is still running");

        d->data->thread = 0;
    }
}


bool QThread::isFinished() const
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    return d->finished || d->isInFinish;
}


bool QThread::isRunning() const
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    return d->running && !d->isInFinish;
}


void QThread::setStackSize(uint stackSize)
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    d->stackSize = stackSize;
}


uint QThread::stackSize() const
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    return d->stackSize;
}


int QThread::exec()
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    d->data->quitNow = false;
    if (d->exited) {
        d->exited = false;
        return d->returnCode;
    }
    locker.unlock();

    QEventLoop eventLoop;
    int returnCode = eventLoop.exec();

    locker.relock();
    d->exited = false;
    d->returnCode = -1;
    return returnCode;
}


void QThread::exit(int returnCode)
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    d->exited = true;
    d->returnCode = returnCode;
    d->data->quitNow = true;
    for (int i = 0; i < d->data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = d->data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}


void QThread::quit()
{ exit(); }


void QThread::run()
{
    (void) exec();
}


void QThread::setPriority(Priority priority)
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    if (!d->running) {
        qWarning("QThread::setPriority: Cannot set priority, thread is not running");
        return;
    }
    d->setPriority(priority);
}


QThread::Priority QThread::priority() const
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);

    // mask off the high bits that are used for flags
    return Priority(d->priority & 0xffff);
}


int QThread::loopLevel() const
{
    QThreadPrivate * const d = d_func();
    return d->data->eventLoops.size();
}



/*!
    \since 5.0

    Returns a pointer to the event dispatcher object for the thread. If no event
    dispatcher exists for the thread, this function returns 0.
*/
QAbstractEventDispatcher *QThread::eventDispatcher() const
{
    QThreadPrivate * const d = d_func();
    return d->data->eventDispatcher.load();
}

/*!
    \since 5.0

    Sets the event dispatcher for the thread to \a eventDispatcher. This is
    only possible as long as there is no event dispatcher installed for the
    thread yet. That is, before the thread has been started with start() or, in
    case of the main thread, before QCoreApplication has been instantiated.
    This method takes ownership of the object.
*/
void QThread::setEventDispatcher(QAbstractEventDispatcher *eventDispatcher)
{
    QThreadPrivate * const d = d_func();
    if (d->data->hasEventDispatcher()) {
        qWarning("QThread::setEventDispatcher: An event dispatcher has already been created for this thread");
    } else {
        eventDispatcher->moveToThread(this);
        if (eventDispatcher->thread() == this) // was the move successful?
            d->data->eventDispatcher = eventDispatcher;
        else
            qWarning("QThread::setEventDispatcher: Could not move event dispatcher to target thread");
    }
}

/*!
    \reimp
*/
bool QThread::event(QEvent *event)
{
    if (event->type() == QEvent::Quit) {
        quit();
        return true;
    } else {
        return QObject::event(event);
    }
}


void QThread::requestInterruption()
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    if (!d->running || d->finished || d->isInFinish)
        return;
    if (this == QCoreApplicationPrivate::theMainThread) {
        qWarning("QThread::requestInterruption has no effect on the main thread");
        return;
    }
    d->interruptionRequested = true;
}


bool QThread::isInterruptionRequested() const
{
    QThreadPrivate * const d = d_func();
    QMutexLocker locker(&d->mutex);
    if (!d->running || d->finished || d->isInFinish)
        return false;
    return d->interruptionRequested;
}


QDaemonThread::QDaemonThread(QObject *parent)
    : QThread(parent)
{
    // QThread::started() is emitted from the thread we start
    connect(this, &QThread::started,
            [](){ QThreadData::current()->requiresCoreApplication = false; });
}


QT_END_NAMESPACE

#include "moc_qthread.cpp"

#include "qcoreapplication.h"
#include "qcoreapplication_p.h"


#include "qabstracteventdispatcher.h"
#include "qcoreevent.h"
#include "qeventloop.h"
#include "qcorecmdlineargs_p.h"
#include <qdatastream.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmutex.h>
#include <private/qloggingregistry_p.h>
#include <qstandardpaths.h>
#include <qtextcodec.h>
#include <qthread.h>
#include <qthreadpool.h>
#include <qthreadstorage.h>
#include <private/qthread_p.h>
#include <qelapsedtimer.h>
#include <qlibraryinfo.h>
#include <qvarlengtharray.h>
#include <private/qfactoryloader_p.h>
#include <private/qfunctions_p.h>
#include <private/qlocale_p.h>
#include <private/qhooks_p.h>


#include "qeventdispatcher_win_p.h"



#include <stdlib.h>
#include <algorithm>



class QMutexUnlocker
{
public:
    inline explicit QMutexUnlocker(QMutex *m)
        : mtx(m)
    { }
    inline ~QMutexUnlocker() { unlock(); }
    inline void unlock() { if (mtx) mtx->unlock(); mtx = 0; }

private:
    Q_DISABLE_COPY(QMutexUnlocker)

    QMutex *mtx;
};


extern QString qAppFileName();

int QCoreApplicationPrivate::app_compile_version = 0x050000; //we don't know exactly, but it's at least 5.0.0

bool QCoreApplicationPrivate::setuidAllowed = false;

QString *QCoreApplicationPrivate::cachedApplicationFilePath = 0;


// oye, static functions exist in class, how to make sure they are called after the Singleton instanciated
bool QCoreApplicationPrivate::checkInstance(const char *function)
{
    bool b = (QCoreApplication::self != 0);
    if (!b)
        qWarning("QApplication::%s: Please instantiate the QApplication object first", function);
    return b;
}

void QCoreApplicationPrivate::processCommandLineArguments()
{
    int j = argc ? 1 : 0;
    for (int i = 1; i < argc; ++i) {
        if (!argv[i])
            continue;
        if (*argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        const char *arg = argv[i];
        if (arg[1] == '-') // startsWith("--")
            ++arg;
        if (strncmp(arg, "-qmljsdebugger=", 15) == 0) {
            qmljs_debug_arguments = QString::fromLocal8Bit(arg + 15);
        } else if (strcmp(arg, "-qmljsdebugger") == 0 && i < argc - 1) {
            ++i;
            qmljs_debug_arguments = QString::fromLocal8Bit(argv[i]);
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < argc) {
        argv[j] = 0;
        argc = j;
    }
}

// Support for introspection


QSignalSpyCallbackSet  qt_signal_spy_callback_set = { 0, 0, 0, 0 };

void qt_register_signal_spy_callbacks(const QSignalSpyCallbackSet &callback_set)
{
    qt_signal_spy_callback_set = callback_set;
}


extern "C" void  qt_startup_hook()
{
}

typedef QList<QtStartUpFunction> QStartUpFuncList;
Q_GLOBAL_STATIC(QStartUpFuncList, preRList)
typedef QList<QtCleanUpFunction> QVFuncList;
Q_GLOBAL_STATIC(QVFuncList, postRList)
	

static QBasicMutex globalPreRoutinesMutex;


/*!
    Adds a global routine that will be called from the QCoreApplication
    constructor. The public API is Q_COREAPP_STARTUP_FUNCTION.
*/
void qAddPreRoutine(QtStartUpFunction p)
{
    QStartUpFuncList *list = preRList();
    if (!list)
        return;

    QMutexLocker locker(&globalPreRoutinesMutex);
    if (QCoreApplication::instance())
        p();
    list->prepend(p); // in case QCoreApplication is re-created, see qt_call_pre_routines
}

void qAddPostRoutine(QtCleanUpFunction p)
{
    QVFuncList *list = postRList();
    if (!list)
        return;
    list->prepend(p);
}

void qRemovePostRoutine(QtCleanUpFunction p)
{
    QVFuncList *list = postRList();
    if (!list)
        return;
    list->removeAll(p);
}

static void qt_call_pre_routines()
{
    if (!preRList.exists())
        return;


    QMutexLocker locker(&globalPreRoutinesMutex);

    QVFuncList *list = &(*preRList);
    // Unlike qt_call_post_routines, we don't empty the list, because
    // Q_COREAPP_STARTUP_FUNCTION is a macro, so the user expects
    // the function to be executed every time QCoreApplication is created.
    for (int i = 0; i < list->count(); ++i)
        list->at(i)();
}

void  qt_call_post_routines()
{
    if (!postRList.exists())
        return;

    QVFuncList *list = &(*postRList);
    while (!list->isEmpty())
        (list->takeFirst())();
}


// initialized in qcoreapplication and in qtextstream autotest when setlocale is called.
static bool qt_locale_initialized = false;


// app starting up if false
bool QCoreApplicationPrivate::is_app_running = false;
 // app closing down if true
bool QCoreApplicationPrivate::is_app_closing = false;


uint qGlobalPostedEventsCount()
{
    QThreadData *currentThreadData = QThreadData::current();
    return currentThreadData->postEventList.size() - currentThreadData->postEventList.startOffset;
}

QAbstractEventDispatcher *QCoreApplicationPrivate::eventDispatcher = 0;


// oye singleton
QCoreApplication *QCoreApplication::self = 0;

uint QCoreApplicationPrivate::attribs =
    (1 << Qt::AA_SynthesizeMouseForUnhandledTouchEvents) |
    (1 << Qt::AA_SynthesizeMouseForUnhandledTabletEvents);

struct QCoreApplicationData {
    QCoreApplicationData()  {
        applicationNameSet = false;
        applicationVersionSet = false;
    }
    ~QCoreApplicationData() {
        // cleanup the QAdoptedThread created for the main() thread
        if (QCoreApplicationPrivate::theMainThread) {
            QThreadData *data = QThreadData::get2(QCoreApplicationPrivate::theMainThread);
            data->deref(); // deletes the data and the adopted thread
        }
    }

    QString orgName, orgDomain;
    QString application; // application name, initially from argv[0], can then be modified.
    QString applicationVersion;
    bool applicationNameSet; // true if setApplicationName was called
    bool applicationVersionSet; // true if setApplicationVersion was called

    QScopedPointer<QStringList> app_libpaths;
    QScopedPointer<QStringList> manual_libpaths;

};

//Q_GLOBAL_STATIC(QCoreApplicationData, coreappdata)


static bool quitLockRefEnabled = true;


static inline bool contains(int argc, char **argv, const char *needle)
{
    for (int a = 0; a < argc; ++a) {
        if (!strcmp(argv[a], needle))
            return true;
    }
    return false;
}


QCoreApplicationPrivate::QCoreApplicationPrivate(int &aargc, char **aargv, uint flags)
    :
      QObjectPrivate(),
      argc(aargc)
    , argv(aargv)
    , origArgc(0)
    , origArgv(Q_NULLPTR)
    , application_type(QCoreApplicationPrivate::Tty)
    , in_exec(false)
    , aboutToQuitEmitted(false)
    , threadData_clean(false)

{
    app_compile_version = flags & 0xffffff;
    static const char *const empty = "";
    if (argc == 0 || argv == 0) {
        argc = 0;
        argv = const_cast<char **>(&empty);
    }

    
    origArgc = argc;
    origArgv = new char *[argc];
    std::copy(argv, argv + argc, QT_MAKE_CHECKED_ARRAY_ITERATOR(origArgv, argc));
    

    QCoreApplicationPrivate::is_app_closing = false;

	// oye, in QThreadData::current()->thread, it will set value to theMainThread
    QThread *cur = QThread::currentThread(); // note: this may end up setting theMainThread!
    if (cur != theMainThread)
        qWarning("WARNING: QApplication was not created in the main() thread.");

}

QCoreApplicationPrivate::~QCoreApplicationPrivate()
{
    cleanupThreadData();

    delete [] origArgv;

    QCoreApplicationPrivate::clearApplicationFilePath();
}

#ifndef QT_NO_QOBJECT

void QCoreApplicationPrivate::cleanupThreadData()
{
    if (threadData && !threadData_clean) {
#ifndef QT_NO_THREAD
        void *data = &threadData->tls;
        QThreadStorageData::finish((void **)data);
#endif

        // need to clear the state of the mainData, just in case a new QCoreApplication comes along.
        QMutexLocker locker(&threadData->postEventList.mutex);
        for (int i = 0; i < threadData->postEventList.size(); ++i) {
            const QPostEvent &pe = threadData->postEventList.at(i);
            if (pe.event) {
                --pe.receiver->d_func()->postedEvents;
                pe.event->posted = false;
                delete pe.event;
            }
        }
        threadData->postEventList.clear();
        threadData->postEventList.recursion = 0;
        threadData->quitNow = false;
        threadData_clean = true;
    }
}

void QCoreApplicationPrivate::createEventDispatcher()
{
    Q_Q(QCoreApplication);
	// oye, deleted other platforms like Unix Glib WinRT
	eventDispatcher = new QEventDispatcherWin32(q);

}

void QCoreApplicationPrivate::eventDispatcherReady()
{
}

QBasicAtomicPointer<QThread> QCoreApplicationPrivate::theMainThread = Q_BASIC_ATOMIC_INITIALIZER(0);
QThread *QCoreApplicationPrivate::mainThread()
{
    return theMainThread.load();
}

bool QCoreApplicationPrivate::threadRequiresCoreApplication()
{
    QThreadData *data = QThreadData::current(false);
    if (!data)
        return true;    // default setting
    return data->requiresCoreApplication;
}

void QCoreApplicationPrivate::checkReceiverThread(QObject *receiver)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = receiver->thread();
    Q_ASSERT_X(currentThread == thr || !thr,
               "QCoreApplication::sendEvent",
               QString::asprintf("Cannot send events to objects owned by a different thread. "
                                 "Current thread 0x%p. Receiver '%ls' (of type '%s') was created in thread 0x%p",
                                 currentThread, qUtf16Printable(receiver->objectName()),
                                 receiver->metaObject()->className(), thr)
               .toLocal8Bit().data());
    Q_UNUSED(currentThread);
    Q_UNUSED(thr);
}

#endif // QT_NO_QOBJECT

void QCoreApplicationPrivate::appendApplicationPathToLibraryPaths()
{
	// oye delete
}

QString qAppName()
{
    if (!QCoreApplicationPrivate::checkInstance("qAppName"))
        return QString();
    return QCoreApplication::instance()->d_func()->appName();
}

void QCoreApplicationPrivate::initLocale()
{
    if (qt_locale_initialized)
        return;
    qt_locale_initialized = true;
    setlocale(LC_ALL, "");

}

QCoreApplication::QCoreApplication(QCoreApplicationPrivate &p)
    : QObject(p, 0)
{
    d_func()->q_ptr = this;
    // note: it is the subclasses' job to call
    // QCoreApplicationPrivate::eventDispatcher->startingUp();
}


QCoreApplication::QCoreApplication(int &argc, char **argv , int _internal )
    : QObject(*new QCoreApplicationPrivate(argc, argv, _internal))
{
    d_func()->q_ptr = this;
    d_func()->init();
    QCoreApplicationPrivate::eventDispatcher->startingUp();

}


void QCoreApplicationPrivate::init()
{

    Q_Q(QCoreApplication);

    initLocale();

    Q_ASSERT_X(!QCoreApplication::self, "QCoreApplication", "there should be only one application object");
    QCoreApplication::self = q;

    // Store app name/version (so they're still available after QCoreApplication is destroyed)
    if (!coreappdata()->applicationNameSet)
        coreappdata()->application = appName();

    if (!coreappdata()->applicationVersionSet)
        coreappdata()->applicationVersion = appVersion();

#if QT_CONFIG(library)
    // Reset the lib paths, so that they will be recomputed, taking the availability of argv[0]
    // into account. If necessary, recompute right away and replay the manual changes on top of the
    // new lib paths.
    QStringList *appPaths = coreappdata()->app_libpaths.take();
    QStringList *manualPaths = coreappdata()->manual_libpaths.take();
    if (appPaths) {
        if (manualPaths) {
            // Replay the delta. As paths can only be prepended to the front or removed from
            // anywhere in the list, we can just linearly scan the lists and find the items that
            // have been removed. Once the original list is exhausted we know all the remaining
            // items have been added.
            QStringList newPaths(q->libraryPaths());
            for (int i = manualPaths->length(), j = appPaths->length(); i > 0 || j > 0; qt_noop()) {
                if (--j < 0) {
                    newPaths.prepend((*manualPaths)[--i]);
                } else if (--i < 0) {
                    newPaths.removeAll((*appPaths)[j]);
                } else if ((*manualPaths)[i] != (*appPaths)[j]) {
                    newPaths.removeAll((*appPaths)[j]);
                    ++i; // try again with next item.
                }
            }
            delete manualPaths;
            coreappdata()->manual_libpaths.reset(new QStringList(newPaths));
        }
        delete appPaths;
    }
#endif


    // use the event dispatcher created by the app programmer (if any)
    if (!eventDispatcher)
        eventDispatcher = threadData->eventDispatcher.load();
    // otherwise we create one
    if (!eventDispatcher)
        createEventDispatcher();
    Q_ASSERT(eventDispatcher);

    if (!eventDispatcher->parent()) {
        eventDispatcher->moveToThread(threadData->thread);
        eventDispatcher->setParent(q);
    }

    threadData->eventDispatcher = eventDispatcher;
    eventDispatcherReady();


#ifdef QT_EVAL
    extern void qt_core_eval_init(QCoreApplicationPrivate::Type);
    qt_core_eval_init(application_type);
#endif

    processCommandLineArguments();

    qt_call_pre_routines();
    qt_startup_hook();
#ifndef QT_BOOTSTRAPPED
    if (Q_UNLIKELY(qtHookData[QHooks::Startup]))
        reinterpret_cast<QHooks::StartupCallback>(qtHookData[QHooks::Startup])();
#endif
    is_app_running = true; // No longer starting up.
}


QCoreApplication::~QCoreApplication()
{
    qt_call_post_routines();

    self = 0;

    QCoreApplicationPrivate::is_app_closing = true;
    QCoreApplicationPrivate::is_app_running = false;



    // Synchronize and stop the global thread pool threads.
    QThreadPool *globalThreadPool = 0;
    QT_TRY {
        globalThreadPool = QThreadPool::globalInstance();
    } QT_CATCH (...) {
        // swallow the exception, since destructors shouldn't throw
    }
    if (globalThreadPool)
        globalThreadPool->waitForDone();


    d_func()->threadData->eventDispatcher = 0;
    if (QCoreApplicationPrivate::eventDispatcher)
        QCoreApplicationPrivate::eventDispatcher->closingDown();
    QCoreApplicationPrivate::eventDispatcher = 0;


#if QT_CONFIG(library)
    coreappdata()->app_libpaths.reset();
    coreappdata()->manual_libpaths.reset();
#endif
}


void QCoreApplication::setSetuidAllowed(bool allow)
{
    QCoreApplicationPrivate::setuidAllowed = allow;
}


bool QCoreApplication::isSetuidAllowed()
{
    return QCoreApplicationPrivate::setuidAllowed;
}


void QCoreApplication::setAttribute(Qt::ApplicationAttribute attribute, bool on)
{
    if (on)
        QCoreApplicationPrivate::attribs |= 1 << attribute;
    else
        QCoreApplicationPrivate::attribs &= ~(1 << attribute);
}


bool QCoreApplication::testAttribute(Qt::ApplicationAttribute attribute)
{
    return QCoreApplicationPrivate::testAttribute(attribute);
}


bool QCoreApplication::isQuitLockEnabled()
{
    return quitLockRefEnabled;
}

static bool doNotify(QObject *, QEvent *);

void QCoreApplication::setQuitLockEnabled(bool enabled)
{
    quitLockRefEnabled = enabled;
}


bool QCoreApplication::notifyInternal2(QObject *receiver, QEvent *event)
{
    bool selfRequired = QCoreApplicationPrivate::threadRequiresCoreApplication();
    if (!self && selfRequired)
        return false;

    // Make it possible for Qt Script to hook into events even
    // though QApplication is subclassed...
    bool result = false;
    void *cbdata[] = { receiver, event, &result };
    if (QInternal::activateCallbacks(QInternal::EventNotifyCallback, cbdata)) {
        return result;
    }

    // Qt enforces the rule that events can only be sent to objects in
    // the current thread, so receiver->d_func()->threadData is
    // equivalent to QThreadData::current(), just without the function
    // call overhead.
    QObjectPrivate *d = receiver->d_func();
    QThreadData *threadData = d->threadData;
    QScopedScopeLevelCounter scopeLevelCounter(threadData);
    if (!selfRequired)
        return doNotify(receiver, event);
    return self->notify(receiver, event);
}

bool QCoreApplication::notify(QObject *receiver, QEvent *event) // virtual base
{
    // no events are delivered after ~QCoreApplication() has started
    if (QCoreApplicationPrivate::is_app_closing)
        return true;
    return doNotify(receiver, event);
}

static bool doNotify(QObject *receiver, QEvent *event)
{
    if (receiver == 0) {                        // serious error
        qWarning("QCoreApplication::notify: Unexpected null receiver");
        return true;
    }

    return receiver->isWidgetType() ? false : QCoreApplicationPrivate::notify_helper(receiver, event);
}

bool QCoreApplicationPrivate::sendThroughApplicationEventFilters(QObject *receiver, QEvent *event)
{
    // We can't access the application event filters outside of the main thread (race conditions)
    Q_ASSERT(receiver->d_func()->threadData->thread == mainThread());

    if (extraData) {
        // application event filters are only called for objects in the GUI thread
        for (int i = 0; i < extraData->eventFilters.size(); ++i) {
            QObject *obj = extraData->eventFilters.at(i);
            if (!obj)
                continue;
            if (obj->d_func()->threadData != threadData) {
                qWarning("QCoreApplication: Application event filter cannot be in a different thread.");
                continue;
            }
            if (obj->eventFilter(receiver, event))
                return true;
        }
    }
    return false;
}

bool QCoreApplicationPrivate::sendThroughObjectEventFilters(QObject *receiver, QEvent *event)
{
    if (receiver != QCoreApplication::instance() && receiver->d_func()->extraData) {
        for (int i = 0; i < receiver->d_func()->extraData->eventFilters.size(); ++i) {
            QObject *obj = receiver->d_func()->extraData->eventFilters.at(i);
            if (!obj)
                continue;
            if (obj->d_func()->threadData != receiver->d_func()->threadData) {
                qWarning("QCoreApplication: Object event filter cannot be in a different thread.");
                continue;
            }
            if (obj->eventFilter(receiver, event))
                return true;
        }
    }
    return false;
}


bool QCoreApplicationPrivate::notify_helper(QObject *receiver, QEvent * event)
{
    // send to all application event filters (only does anything in the main thread)
    if (QCoreApplication::self
            && receiver->d_func()->threadData->thread == mainThread()
            && QCoreApplication::self->d_func()->sendThroughApplicationEventFilters(receiver, event))
        return true;
    // send to all receiver event filters
    if (sendThroughObjectEventFilters(receiver, event))
        return true;
    // deliver the event
    return receiver->event(event);
}


bool QCoreApplication::startingUp()
{
    return !QCoreApplicationPrivate::is_app_running;
}

bool QCoreApplication::closingDown()
{
    return QCoreApplicationPrivate::is_app_closing;
}


void QCoreApplication::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    QThreadData *data = QThreadData::current();
    if (!data->hasEventDispatcher())
        return;
    data->eventDispatcher.load()->processEvents(flags);
}


void QCoreApplication::processEvents(QEventLoop::ProcessEventsFlags flags, int maxtime)
{

    QThreadData *data = QThreadData::current();
    if (!data->hasEventDispatcher())
        return;
    QElapsedTimer start;
    start.start();
    while (data->eventDispatcher.load()->processEvents(flags & ~QEventLoop::WaitForMoreEvents)) {
        if (start.elapsed() > maxtime)
            break;
    }
}

// oye:: QEventLoop eventLoop and its exec();
int QCoreApplication::exec()  // static
{
	// oye, checkInstance will make sure the singleton must be instanciated
	// since exec is the static function
	// sanity check
    if (!QCoreApplicationPrivate::checkInstance("exec"))
        return -1;

	// sanity check
    QThreadData *threadData = self->d_func()->threadData;
    if (threadData != QThreadData::current()) {
        qWarning("%s::exec: Must be called from the main thread", self->metaObject()->className());
        return -1;
    }
	// sanith check
    if (!threadData->eventLoops.isEmpty()) {
        qWarning("QCoreApplication::exec: The event loop is already running");
        return -1;
    }

	// algo begins
    threadData->quitNow = false;
    QEventLoop eventLoop;
    self->d_func()->in_exec = true;
    self->d_func()->aboutToQuitEmitted = false;
    int returnCode = eventLoop.exec();
    threadData->quitNow = false;

    if (self)
        self->d_func()->execCleanup();

    return returnCode;
}


// Cleanup after eventLoop is done executing in QCoreApplication::exec().
// This is for use cases in which QCoreApplication is instantiated by a
// library and not by an application executable, for example, Active X
// servers.

void QCoreApplicationPrivate::execCleanup()
{
    threadData->quitNow = false;
    in_exec = false;
    if (!aboutToQuitEmitted)
        emit q_func()->aboutToQuit(QCoreApplication::QPrivateSignal());
    aboutToQuitEmitted = true;
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
}


/*!
  Tells the application to exit with a return code.

    After this function has been called, the application leaves the
    main event loop and returns from the call to exec(). The exec()
    function returns \a returnCode. If the event loop is not running,
    this function does nothing.

  By convention, a \a returnCode of 0 means success, and any non-zero
  value indicates an error.

  Note that unlike the C library function of the same name, this
  function \e does return to the caller -- it is event processing that
  stops.

  \sa quit(), exec()
*/
void QCoreApplication::exit(int returnCode)
{
    if (!self)
        return;
    QThreadData *data = self->d_func()->threadData;
    data->quitNow = true;
    for (int i = 0; i < data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}

// public static
void QCoreApplication::postEvent(QObject *receiver, QEvent *event, int priority)
{
	// oye sanity check
    if (receiver == 0) {
        qWarning("QCoreApplication::postEvent: Unexpected null receiver");
        delete event;
        return;
    }

	// get t_data throught receiver, not current app's thread
	// sicne the receiver can be moved to another thread
    QThreadData * volatile * pdata = &receiver->d_func()->threadData;
    QThreadData *data = *pdata;

	// oye sanity check
    if (!data) {
        // posting during destruction? just delete the event to prevent a leak
        delete event;
        return;
    }


    data->postEventList.mutex.lock();

    // if object has moved to another thread, follow it
    while (data != *pdata) {
        data->postEventList.mutex.unlock();

        data = *pdata;
        if (!data) {
            // posting during destruction? just delete the event to prevent a leak
            delete event;
            return;
        }

        data->postEventList.mutex.lock();
    }

    QMutexUnlocker locker(&data->postEventList.mutex);

    // if this is one of the compressible events, do compression
    if (receiver->d_func()->postedEvents && self && self->compressEvent(event, receiver, &data->postEventList)) {
        return;
    }

    if (event->type() == QEvent::DeferredDelete)
        receiver->d_ptr->deleteLaterCalled = true;

    if (event->type() == QEvent::DeferredDelete && data == QThreadData::current()) {
        // remember the current running eventloop for DeferredDelete
        // events posted in the receiver's thread.

        // Events sent by non-Qt event handlers (such as glib) may not
        // have the scopeLevel set correctly. The scope level makes sure that
        // code like this:
        //     foo->deleteLater();
        //     qApp->processEvents(); // without passing QEvent::DeferredDelete
        // will not cause "foo" to be deleted before returning to the event loop.

        // If the scope level is 0 while loopLevel != 0, we are called from a
        // non-conformant code path, and our best guess is that the scope level
        // should be 1. (Loop level 0 is special: it means that no event loops
        // are running.)
        int loopLevel = data->loopLevel;
        int scopeLevel = data->scopeLevel;
        if (scopeLevel == 0 && loopLevel != 0)
            scopeLevel = 1;
        static_cast<QDeferredDeleteEvent *>(event)->level = loopLevel + scopeLevel;
    }

    // delete the event on exceptions to protect against memory leaks till the event is
    // properly owned in the postEventList
    QScopedPointer<QEvent> eventDeleter(event);
    data->postEventList.addEvent(QPostEvent(receiver, event, priority));
    eventDeleter.take();
    event->posted = true;
    ++receiver->d_func()->postedEvents;
    data->canWait = false;
    locker.unlock();

    QAbstractEventDispatcher* dispatcher = data->eventDispatcher.loadAcquire();
    if (dispatcher)
        dispatcher->wakeUp();
}


bool QCoreApplication::compressEvent(QEvent *event, 
         QObject *receiver, QPostEventList *postedEvents) override
{

    // compress posted timers to this object.
    if (event->type() == QEvent::Timer && receiver->d_func()->postedEvents > 0) 
	{
	
        int timerId = ((QTimerEvent *) event)->timerId();
        for (int i=0; i<postedEvents->size(); ++i) {
            const QPostEvent &e = postedEvents->at(i);
            if (e.receiver == receiver && e.event && e.event->type() == QEvent::Timer
                && ((QTimerEvent *) e.event)->timerId() == timerId) {
                delete event;
                return true;
            }
        }
        return false;
    }


    if (event->type() == QEvent::DeferredDelete) {
        if (receiver->d_ptr->deleteLaterCalled) {
            // there was a previous DeferredDelete event, so we can drop the new one
            delete event;
            return true;
        }
        // deleteLaterCalled is set to true in postedEvents when queueing the very first
        // deferred deletion event.
        return false;
    }

    if (event->type() == QEvent::Quit && receiver->d_func()->postedEvents > 0) {
        for (int i = 0; i < postedEvents->size(); ++i) {
            const QPostEvent &cur = postedEvents->at(i);
            if (cur.receiver != receiver
                    || cur.event == 0
                    || cur.event->type() != event->type())
                continue;
            // found an event for this receiver
            delete event;
            return true;
        }
    }

    return false;
}

void QCoreApplication::sendPostedEvents(QObject *receiver, int event_type)
{
    QThreadData *data = QThreadData::current();
    QCoreApplicationPrivate::sendPostedEvents(receiver, event_type, data);
}

// static
void QCoreApplicationPrivate::sendPostedEvents(QObject *receiver, int event_type,
                                               QThreadData *data)
{
	// oye sanith check
    if (receiver && receiver->d_func()->threadData != data) {
        qWarning("QCoreApplication::sendPostedEvents: Cannot send "
                 "posted events for objects in another thread");
        return;
    }

	
    ++data->postEventList.recursion;

    QMutexLocker locker(&data->postEventList.mutex);

    // by default, we assume that the event dispatcher can go to sleep after
    // processing all events. if any new events are posted while we send
    // events, canWait will be set to false.
    data->canWait = (data->postEventList.size() == 0);

    if (data->postEventList.size() == 0 || (receiver && !receiver->d_func()->postedEvents)) {
        --data->postEventList.recursion;
        return;
    }

    data->canWait = true;

    // okay. here is the tricky loop. be careful about optimizing
    // this, it looks the way it does for good reasons.
    int startOffset = data->postEventList.startOffset;
    int &i = (!event_type && !receiver) ? data->postEventList.startOffset : startOffset;
    data->postEventList.insertionOffset = data->postEventList.size();

    // Exception-safe cleaning up without the need for a try/catch block
    struct CleanUp {
        QObject *receiver;
        int event_type;
        QThreadData *data;
        bool exceptionCaught;

        inline CleanUp(QObject *receiver, int event_type, QThreadData *data) :
            receiver(receiver), event_type(event_type), data(data), exceptionCaught(true)
        {}
        inline ~CleanUp()
        {
            if (exceptionCaught) {
                // since we were interrupted, we need another pass to make sure we clean everything up
                data->canWait = false;
            }

            --data->postEventList.recursion;
            if (!data->postEventList.recursion && !data->canWait && data->hasEventDispatcher())
                data->eventDispatcher.load()->wakeUp();

            // clear the global list, i.e. remove everything that was
            // delivered.
            if (!event_type && !receiver && data->postEventList.startOffset >= 0) {
                const QPostEventList::iterator it = data->postEventList.begin();
                data->postEventList.erase(it, it + data->postEventList.startOffset);
                data->postEventList.insertionOffset -= data->postEventList.startOffset;
                Q_ASSERT(data->postEventList.insertionOffset >= 0);
                data->postEventList.startOffset = 0;
            }
        }
    };
    CleanUp cleanup(receiver, event_type, data);

    while (i < data->postEventList.size()) {
        // avoid live-lock
        if (i >= data->postEventList.insertionOffset)
            break;

        const QPostEvent &pe = data->postEventList.at(i);
        ++i;

        if (!pe.event)
            continue;
        if ((receiver && receiver != pe.receiver) || (event_type && event_type != pe.event->type())) {
            data->canWait = false;
            continue;
        }

        if (pe.event->type() == QEvent::DeferredDelete) {
            // DeferredDelete events are sent either
            // 1) when the event loop that posted the event has returned; or
            // 2) if explicitly requested (with QEvent::DeferredDelete) for
            //    events posted by the current event loop; or
            // 3) if the event was posted before the outermost event loop.

            int eventLevel = static_cast<QDeferredDeleteEvent *>(pe.event)->loopLevel();
            int loopLevel = data->loopLevel + data->scopeLevel;
            const bool allowDeferredDelete =
                (eventLevel > loopLevel
                 || (!eventLevel && loopLevel > 0)
                 || (event_type == QEvent::DeferredDelete
                     && eventLevel == loopLevel));
            if (!allowDeferredDelete) {
                // cannot send deferred delete
                if (!event_type && !receiver) {
                    // we must copy it first; we want to re-post the event
                    // with the event pointer intact, but we can't delay
                    // nulling the event ptr until after re-posting, as
                    // addEvent may invalidate pe.
                    QPostEvent pe_copy = pe;

                    // null out the event so if sendPostedEvents recurses, it
                    // will ignore this one, as it's been re-posted.
                    const_cast<QPostEvent &>(pe).event = 0;

                    // re-post the copied event so it isn't lost
                    data->postEventList.addEvent(pe_copy);
                }
                continue;
            }
        }

        // first, we diddle the event so that we can deliver
        // it, and that no one will try to touch it later.
        pe.event->posted = false;
        QEvent *e = pe.event;
        QObject * r = pe.receiver;

        --r->d_func()->postedEvents;

        // next, update the data structure so that we're ready
        // for the next event.
        const_cast<QPostEvent &>(pe).event = 0;

        struct MutexUnlocker
        {
            QMutexLocker &m;
            MutexUnlocker(QMutexLocker &m) : m(m) { m.unlock(); }
            ~MutexUnlocker() { m.relock(); }
        };
        MutexUnlocker unlocker(locker);

        QScopedPointer<QEvent> event_deleter(e); // will delete the event (with the mutex unlocked)

        // after all that work, it's time to deliver the event.
        QCoreApplication::sendEvent(r, e);

        // careful when adding anything below this point - the
        // sendEvent() call might invalidate any invariants this
        // function depends on.
    }

    cleanup.exceptionCaught = false;
}


void QCoreApplication::removePostedEvents(QObject *receiver, int eventType)
{
    QThreadData *data = receiver ? receiver->d_func()->threadData : QThreadData::current();
    QMutexLocker locker(&data->postEventList.mutex);

    // the QObject destructor calls this function directly.  this can
    // happen while the event loop is in the middle of posting events,
    // and when we get here, we may not have any more posted events
    // for this object.
    if (receiver && !receiver->d_func()->postedEvents)
        return;

    //we will collect all the posted events for the QObject
    //and we'll delete after the mutex was unlocked
    QVarLengthArray<QEvent*> events;
    int n = data->postEventList.size();
    int j = 0;

    for (int i = 0; i < n; ++i) {
        const QPostEvent &pe = data->postEventList.at(i);

        if ((!receiver || pe.receiver == receiver)
            && (pe.event && (eventType == 0 || pe.event->type() == eventType))) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            events.append(pe.event);
            const_cast<QPostEvent &>(pe).event = 0;
        } else if (!data->postEventList.recursion) {
            if (i != j)
                qSwap(data->postEventList[i], data->postEventList[j]);
            ++j;
        }
    }

#ifdef QT_DEBUG
    if (receiver && eventType == 0) {
        Q_ASSERT(!receiver->d_func()->postedEvents);
    }
#endif

    if (!data->postEventList.recursion) {
        // truncate list
        data->postEventList.erase(data->postEventList.begin() + j, data->postEventList.end());
    }

    locker.unlock();
    for (int i = 0; i < events.count(); ++i) {
        delete events[i];
    }
}


void QCoreApplicationPrivate::removePostedEvent(QEvent * event)
{
    if (!event || !event->posted)
        return;

    QThreadData *data = QThreadData::current();

    QMutexLocker locker(&data->postEventList.mutex);

    if (data->postEventList.size() == 0) {
        qDebug("QCoreApplication::removePostedEvent: Internal error: %p %d is posted",
                (void*)event, event->type());
        return;
    }

    for (int i = 0; i < data->postEventList.size(); ++i) {
        const QPostEvent & pe = data->postEventList.at(i);
        if (pe.event == event) {
            qWarning("QCoreApplication::removePostedEvent: Event of type %d deleted while posted to %s %s",
                     event->type(),
                     pe.receiver->metaObject()->className(),
                     pe.receiver->objectName().toLocal8Bit().data());
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
            const_cast<QPostEvent &>(pe).event = 0;
            return;
        }
    }
}

/*!\reimp

*/
bool QCoreApplication::event(QEvent *e)
{
    if (e->type() == QEvent::Quit) {
        quit();
        return true;
    }
    return QObject::event(e);
}

/*! \enum QCoreApplication::Encoding
    \obsolete

    This enum type used to define the 8-bit encoding of character string
    arguments to translate(). This enum is now obsolete and UTF-8 will be
    used in all cases.

    \value UnicodeUTF8   UTF-8.
    \omitvalue Latin1
    \omitvalue DefaultCodec  UTF-8.
    \omitvalue CodecForTr

    \sa QObject::tr(), QString::fromUtf8()
*/

void QCoreApplicationPrivate::ref()
{
    quitLockRef.ref();
}

void QCoreApplicationPrivate::deref()
{
    if (!quitLockRef.deref())
        maybeQuit();
}

void QCoreApplicationPrivate::maybeQuit()
{
    if (quitLockRef.load() == 0 && in_exec && quitLockRefEnabled && shouldQuit())
        QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::Quit));
}

/*!
    Tells the application to exit with return code 0 (success).
    Equivalent to calling QCoreApplication::exit(0).

    It's common to connect the QGuiApplication::lastWindowClosed() signal
    to quit(), and you also often connect e.g. QAbstractButton::clicked() or
    signals in QAction, QMenu, or QMenuBar to it.

    Example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 1

    \sa exit(), aboutToQuit(), QGuiApplication::lastWindowClosed()
*/

void QCoreApplication::quit()
{
    exit(0);
}

/*!
  \fn void QCoreApplication::aboutToQuit()

  This signal is emitted when the application is about to quit the
  main event loop, e.g. when the event loop level drops to zero.
  This may happen either after a call to quit() from inside the
  application or when the user shuts down the entire desktop session.

  The signal is particularly useful if your application has to do some
  last-second cleanup. Note that no user interaction is possible in
  this state.

  \sa quit()
*/



#ifndef QT_NO_TRANSLATION
/*!
    Adds the translation file \a translationFile to the list of
    translation files to be used for translations.

    Multiple translation files can be installed. Translations are
    searched for in the reverse order in which they were installed,
    so the most recently installed translation file is searched first
    and the first translation file installed is searched last.
    The search stops as soon as a translation containing a matching
    string is found.

    Installing or removing a QTranslator, or changing an installed QTranslator
    generates a \l{QEvent::LanguageChange}{LanguageChange} event for the
    QCoreApplication instance. A QApplication instance will propagate the event
    to all toplevel widgets, where a reimplementation of changeEvent can
    re-translate the user interface by passing user-visible strings via the
    tr() function to the respective property setters. User-interface classes
    generated by Qt Designer provide a \c retranslateUi() function that can be
    called.

    The function returns \c true on success and false on failure.

    \sa removeTranslator(), translate(), QTranslator::load(), {Dynamic Translation}
*/

bool QCoreApplication::installTranslator(QTranslator *translationFile)
{
    if (!translationFile)
        return false;

    if (!QCoreApplicationPrivate::checkInstance("installTranslator"))
        return false;
    QCoreApplicationPrivate *d = self->d_func();
    d->translators.prepend(translationFile);

#ifndef QT_NO_TRANSLATION_BUILDER
    if (translationFile->isEmpty())
        return false;
#endif

#ifndef QT_NO_QOBJECT
    QEvent ev(QEvent::LanguageChange);
    QCoreApplication::sendEvent(self, &ev);
#endif

    return true;
}

/*!
    Removes the translation file \a translationFile from the list of
    translation files used by this application. (It does not delete the
    translation file from the file system.)

    The function returns \c true on success and false on failure.

    \sa installTranslator(), translate(), QObject::tr()
*/

bool QCoreApplication::removeTranslator(QTranslator *translationFile)
{
    if (!translationFile)
        return false;
    if (!QCoreApplicationPrivate::checkInstance("removeTranslator"))
        return false;
    QCoreApplicationPrivate *d = self->d_func();
    if (d->translators.removeAll(translationFile)) {
#ifndef QT_NO_QOBJECT
        if (!self->closingDown()) {
            QEvent ev(QEvent::LanguageChange);
            QCoreApplication::sendEvent(self, &ev);
        }
#endif
        return true;
    }
    return false;
}

static void replacePercentN(QString *result, int n)
{
    if (n >= 0) {
        int percentPos = 0;
        int len = 0;
        while ((percentPos = result->indexOf(QLatin1Char('%'), percentPos + len)) != -1) {
            len = 1;
            QString fmt;
            if (result->at(percentPos + len) == QLatin1Char('L')) {
                ++len;
                fmt = QLatin1String("%L1");
            } else {
                fmt = QLatin1String("%1");
            }
            if (result->at(percentPos + len) == QLatin1Char('n')) {
                fmt = fmt.arg(n);
                ++len;
                result->replace(percentPos, len, fmt);
                len = fmt.length();
            }
        }
    }
}

/*!
    \reentrant

    Returns the translation text for \a sourceText, by querying the
    installed translation files. The translation files are searched
    from the most recently installed file back to the first
    installed file.

    QObject::tr() provides this functionality more conveniently.

    \a context is typically a class name (e.g., "MyDialog") and \a
    sourceText is either English text or a short identifying text.

    \a disambiguation is an identifying string, for when the same \a
    sourceText is used in different roles within the same context. By
    default, it is null.

    See the \l QTranslator and \l QObject::tr() documentation for
    more information about contexts, disambiguations and comments.

    \a n is used in conjunction with \c %n to support plural forms.
    See QObject::tr() for details.

    If none of the translation files contain a translation for \a
    sourceText in \a context, this function returns a QString
    equivalent of \a sourceText.

    This function is not virtual. You can use alternative translation
    techniques by subclassing \l QTranslator.

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will most likely result in crashes or other undesirable
    behavior.

    \sa QObject::tr(), installTranslator()
*/
QString QCoreApplication::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n)
{
    QString result;

    if (!sourceText)
        return result;

    if (self && !self->d_func()->translators.isEmpty()) {
        QList<QTranslator*>::ConstIterator it;
        QTranslator *translationFile;
        for (it = self->d_func()->translators.constBegin(); it != self->d_func()->translators.constEnd(); ++it) {
            translationFile = *it;
            result = translationFile->translate(context, sourceText, disambiguation, n);
            if (!result.isNull())
                break;
        }
    }

    if (result.isNull())
        result = QString::fromUtf8(sourceText);

    replacePercentN(&result, n);
    return result;
}

/*! \fn static QString QCoreApplication::translate(const char *context, const char *key, const char *disambiguation, Encoding encoding, int n = -1)

  \obsolete
*/

// Declared in qglobal.h
QString qtTrId(const char *id, int n)
{
    return QCoreApplication::translate(0, id, 0, n);
}

bool QCoreApplicationPrivate::isTranslatorInstalled(QTranslator *translator)
{
    return QCoreApplication::self
           && QCoreApplication::self->d_func()->translators.contains(translator);
}

#else

QString QCoreApplication::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n)
{
    Q_UNUSED(context)
    Q_UNUSED(disambiguation)
    QString ret = QString::fromUtf8(sourceText);
    if (n >= 0)
        ret.replace(QLatin1String("%n"), QString::number(n));
    return ret;
}

#endif //QT_NO_TRANSLATION

// Makes it possible to point QCoreApplication to a custom location to ensure
// the directory is added to the patch, and qt.conf and deployed plugins are
// found from there. This is for use cases in which QGuiApplication is
// instantiated by a library and not by an application executable, for example,
// Active X servers.

void QCoreApplicationPrivate::setApplicationFilePath(const QString &path)
{
    if (QCoreApplicationPrivate::cachedApplicationFilePath)
        *QCoreApplicationPrivate::cachedApplicationFilePath = path;
    else
        QCoreApplicationPrivate::cachedApplicationFilePath = new QString(path);
}


QString QCoreApplication::applicationDirPath()
{
    if (!self) {
        qWarning("QCoreApplication::applicationDirPath: Please instantiate the QApplication object first");
        return QString();
    }

    QCoreApplicationPrivate *d = self->d_func();
    if (d->cachedApplicationDirPath.isNull())
        d->cachedApplicationDirPath = QFileInfo(applicationFilePath()).path();
    return d->cachedApplicationDirPath;
}


QString QCoreApplication::applicationFilePath()
{
    if (!self) {
        qWarning("QCoreApplication::applicationFilePath: Please instantiate the QApplication object first");
        return QString();
    }

    QCoreApplicationPrivate *d = self->d_func();

    if (d->argc) {
        static QByteArray procName = QByteArray(d->argv[0]);
        if (procName != d->argv[0]) {
            // clear the cache if the procname changes, so we reprocess it.
            QCoreApplicationPrivate::clearApplicationFilePath();
            procName = QByteArray(d->argv[0]);
        }
    }

    if (QCoreApplicationPrivate::cachedApplicationFilePath)
        return *QCoreApplicationPrivate::cachedApplicationFilePath;

#if defined(Q_OS_WIN)
    QCoreApplicationPrivate::setApplicationFilePath(QFileInfo(qAppFileName()).filePath());
    return *QCoreApplicationPrivate::cachedApplicationFilePath;
#elif defined(Q_OS_MAC)
    QString qAppFileName_str = qAppFileName();
    if(!qAppFileName_str.isEmpty()) {
        QFileInfo fi(qAppFileName_str);
        if (fi.exists()) {
            QCoreApplicationPrivate::setApplicationFilePath(fi.canonicalFilePath());
            return *QCoreApplicationPrivate::cachedApplicationFilePath;
        }
    }
#endif
#if defined( Q_OS_UNIX )
#  if defined(Q_OS_LINUX) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_EMBEDDED))
    // Try looking for a /proc/<pid>/exe symlink first which points to
    // the absolute path of the executable
    QFileInfo pfi(QString::fromLatin1("/proc/%1/exe").arg(getpid()));
    if (pfi.exists() && pfi.isSymLink()) {
        QCoreApplicationPrivate::setApplicationFilePath(pfi.canonicalFilePath());
        return *QCoreApplicationPrivate::cachedApplicationFilePath;
    }
#  endif
    if (!arguments().isEmpty()) {
        QString argv0 = QFile::decodeName(arguments().at(0).toLocal8Bit());
        QString absPath;

        if (!argv0.isEmpty() && argv0.at(0) == QLatin1Char('/')) {
            /*
              If argv0 starts with a slash, it is already an absolute
              file path.
            */
            absPath = argv0;
        } else if (argv0.contains(QLatin1Char('/'))) {
            /*
              If argv0 contains one or more slashes, it is a file path
              relative to the current directory.
            */
            absPath = QDir::current().absoluteFilePath(argv0);
        } else {
            /*
              Otherwise, the file path has to be determined using the
              PATH environment variable.
            */
            absPath = QStandardPaths::findExecutable(argv0);
        }

        absPath = QDir::cleanPath(absPath);

        QFileInfo fi(absPath);
        if (fi.exists()) {
            QCoreApplicationPrivate::setApplicationFilePath(fi.canonicalFilePath());
            return *QCoreApplicationPrivate::cachedApplicationFilePath;
        }
    }

#endif
    return QString();
}

/*!
    \since 4.4

    Returns the current process ID for the application.
*/
qint64 QCoreApplication::applicationPid()
{
#if defined(Q_OS_WIN)
    return GetCurrentProcessId();
#elif defined(Q_OS_VXWORKS)
    return (pid_t) taskIdCurrent;
#else
    return getpid();
#endif
}

/*!
    \since 4.1

    Returns the list of command-line arguments.

    Usually arguments().at(0) is the program name, arguments().at(1)
    is the first argument, and arguments().last() is the last
    argument. See the note below about Windows.

    Calling this function is slow - you should store the result in a variable
    when parsing the command line.

    \warning On Unix, this list is built from the argc and argv parameters passed
    to the constructor in the main() function. The string-data in argv is
    interpreted using QString::fromLocal8Bit(); hence it is not possible to
    pass, for example, Japanese command line arguments on a system that runs in a
    Latin1 locale. Most modern Unix systems do not have this limitation, as they are
    Unicode-based.

    On Windows, the list is built from the argc and argv parameters only if
    modified argv/argc parameters are passed to the constructor. In that case,
    encoding problems might occur.

    Otherwise, the arguments() are constructed from the return value of
    \l{http://msdn2.microsoft.com/en-us/library/ms683156(VS.85).aspx}{GetCommandLine()}.
    As a result of this, the string given by arguments().at(0) might not be
    the program name on Windows, depending on how the application was started.

    \sa applicationFilePath(), QCommandLineParser
*/

QStringList QCoreApplication::arguments()
{
    QStringList list;

    if (!self) {
        qWarning("QCoreApplication::arguments: Please instantiate the QApplication object first");
        return list;
    }
    const int ac = self->d_func()->argc;
    char ** const av = self->d_func()->argv;
    list.reserve(ac);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    // On Windows, it is possible to pass Unicode arguments on
    // the command line. To restore those, we split the command line
    // and filter out arguments that were deleted by derived application
    // classes by index.
    QString cmdline = QString::fromWCharArray(GetCommandLine());

    const QCoreApplicationPrivate *d = self->d_func();
    if (d->origArgv) {
        const QStringList allArguments = qWinCmdArgs(cmdline);
        Q_ASSERT(allArguments.size() == d->origArgc);
        for (int i = 0; i < d->origArgc; ++i) {
            if (contains(ac, av, d->origArgv[i]))
                list.append(allArguments.at(i));
        }
        return list;
    } // Fall back to rebuilding from argv/argc when a modified argv was passed.
#endif // defined(Q_OS_WIN) && !defined(Q_OS_WINRT)

    for (int a = 0; a < ac; ++a) {
        list << QString::fromLocal8Bit(av[a]);
    }

    return list;
}

/*!
    \property QCoreApplication::organizationName
    \brief the name of the organization that wrote this application

    The value is used by the QSettings class when it is constructed
    using the empty constructor. This saves having to repeat this
    information each time a QSettings object is created.

    On Mac, QSettings uses \l {QCoreApplication::}{organizationDomain()} as the organization
    if it's not an empty string; otherwise it uses
    organizationName(). On all other platforms, QSettings uses
    organizationName() as the organization.

    \sa organizationDomain, applicationName
*/

/*!
  \fn void QCoreApplication::organizationNameChanged()
  \internal

  While not useful from C++ due to how organizationName is normally set once on
  startup, this is still needed for QML so that bindings are reevaluated after
  that initial change.
*/
void QCoreApplication::setOrganizationName(const QString &orgName)
{
    if (coreappdata()->orgName == orgName)
        return;
    coreappdata()->orgName = orgName;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->organizationNameChanged();
#endif
}

QString QCoreApplication::organizationName()
{
    return coreappdata()->orgName;
}

/*!
    \property QCoreApplication::organizationDomain
    \brief the Internet domain of the organization that wrote this application

    The value is used by the QSettings class when it is constructed
    using the empty constructor. This saves having to repeat this
    information each time a QSettings object is created.

    On Mac, QSettings uses organizationDomain() as the organization
    if it's not an empty string; otherwise it uses organizationName().
    On all other platforms, QSettings uses organizationName() as the
    organization.

    \sa organizationName, applicationName, applicationVersion
*/
/*!
  \fn void QCoreApplication::organizationDomainChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setOrganizationDomain(const QString &orgDomain)
{
    if (coreappdata()->orgDomain == orgDomain)
        return;
    coreappdata()->orgDomain = orgDomain;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->organizationDomainChanged();
#endif
}

QString QCoreApplication::organizationDomain()
{
    return coreappdata()->orgDomain;
}

/*!
    \property QCoreApplication::applicationName
    \brief the name of this application

    The value is used by the QSettings class when it is constructed
    using the empty constructor. This saves having to repeat this
    information each time a QSettings object is created.

    If not set, the application name defaults to the executable name (since 5.0).

    \sa organizationName, organizationDomain, applicationVersion, applicationFilePath()
*/
/*!
  \fn void QCoreApplication::applicationNameChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setApplicationName(const QString &application)
{
    coreappdata()->applicationNameSet = !application.isEmpty();
    QString newAppName = application;
    if (newAppName.isEmpty() && QCoreApplication::self)
        newAppName = QCoreApplication::self->d_func()->appName();
    if (coreappdata()->application == newAppName)
        return;
    coreappdata()->application = newAppName;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->applicationNameChanged();
#endif
}

QString QCoreApplication::applicationName()
{
    return coreappdata() ? coreappdata()->application : QString();
}

// Exported for QDesktopServices (Qt4 behavior compatibility)
Q_CORE_EXPORT QString qt_applicationName_noFallback()
{
    return coreappdata()->applicationNameSet ? coreappdata()->application : QString();
}

/*!
    \property QCoreApplication::applicationVersion
    \since 4.4
    \brief the version of this application

    If not set, the application version defaults to a platform-specific value
    determined from the main application executable or package (since Qt 5.9):

    \table
    \header
        \li Platform
        \li Source
    \row
        \li Windows (classic desktop)
        \li PRODUCTVERSION parameter of the VERSIONINFO resource
    \row
        \li Universal Windows Platform
        \li version attribute of the application package manifest
    \row
        \li macOS, iOS, tvOS, watchOS
        \li CFBundleVersion property of the information property list
    \row
        \li Android
        \li android:versionName property of the AndroidManifest.xml manifest element
    \endtable

    On other platforms, the default is the empty string.

    \sa applicationName, organizationName, organizationDomain
*/
/*!
  \fn void QCoreApplication::applicationVersionChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setApplicationVersion(const QString &version)
{
    coreappdata()->applicationVersionSet = !version.isEmpty();
    QString newVersion = version;
    if (newVersion.isEmpty() && QCoreApplication::self)
        newVersion = QCoreApplication::self->d_func()->appVersion();
    if (coreappdata()->applicationVersion == newVersion)
        return;
    coreappdata()->applicationVersion = newVersion;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->applicationVersionChanged();
#endif
}

QString QCoreApplication::applicationVersion()
{
    return coreappdata() ? coreappdata()->applicationVersion : QString();
}

#if QT_CONFIG(library)

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, libraryPathMutex, (QMutex::Recursive))

/*!
    Returns a list of paths that the application will search when
    dynamically loading libraries.

    The return value of this function may change when a QCoreApplication
    is created. It is not recommended to call it before creating a
    QCoreApplication. The directory of the application executable (\b not
    the working directory) is part of the list if it is known. In order
    to make it known a QCoreApplication has to be constructed as it will
    use \c {argv[0]} to find it.

    Qt provides default library paths, but they can also be set using
    a \l{Using qt.conf}{qt.conf} file. Paths specified in this file
    will override default values. Note that if the qt.conf file is in
    the directory of the application executable, it may not be found
    until a QCoreApplication is created. If it is not found when calling
    this function, the default library paths will be used.

    The list will include the installation directory for plugins if
    it exists (the default installation directory for plugins is \c
    INSTALL/plugins, where \c INSTALL is the directory where Qt was
    installed). The colon separated entries of the \c QT_PLUGIN_PATH
    environment variable are always added. The plugin installation
    directory (and its existence) may change when the directory of
    the application executable becomes known.

    If you want to iterate over the list, you can use the \l foreach
    pseudo-keyword:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 2

    \sa setLibraryPaths(), addLibraryPath(), removeLibraryPath(), QLibrary,
        {How to Create Qt Plugins}
*/
QStringList QCoreApplication::libraryPaths()
{
    QMutexLocker locker(libraryPathMutex());

    if (coreappdata()->manual_libpaths)
        return *(coreappdata()->manual_libpaths);

    if (!coreappdata()->app_libpaths) {
        QStringList *app_libpaths = new QStringList;
        coreappdata()->app_libpaths.reset(app_libpaths);

        const QByteArray libPathEnv = qgetenv("QT_PLUGIN_PATH");
        if (!libPathEnv.isEmpty()) {
            QStringList paths = QFile::decodeName(libPathEnv).split(QDir::listSeparator(), QString::SkipEmptyParts);
            for (QStringList::const_iterator it = paths.constBegin(); it != paths.constEnd(); ++it) {
                QString canonicalPath = QDir(*it).canonicalPath();
                if (!canonicalPath.isEmpty()
                    && !app_libpaths->contains(canonicalPath)) {
                    app_libpaths->append(canonicalPath);
                }
            }
        }

        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
        if (QFile::exists(installPathPlugins)) {
            // Make sure we convert from backslashes to slashes.
            installPathPlugins = QDir(installPathPlugins).canonicalPath();
            if (!app_libpaths->contains(installPathPlugins))
                app_libpaths->append(installPathPlugins);
        }

        // If QCoreApplication is not yet instantiated,
        // make sure we add the application path when we construct the QCoreApplication
        if (self) self->d_func()->appendApplicationPathToLibraryPaths();
    }
    return *(coreappdata()->app_libpaths);
}



/*!

    Sets the list of directories to search when loading libraries to
    \a paths. All existing paths will be deleted and the path list
    will consist of the paths given in \a paths.

    The library paths are reset to the default when an instance of
    QCoreApplication is destructed.

    \sa libraryPaths(), addLibraryPath(), removeLibraryPath(), QLibrary
 */
void QCoreApplication::setLibraryPaths(const QStringList &paths)
{
    QMutexLocker locker(libraryPathMutex());

    // setLibraryPaths() is considered a "remove everything and then add some new ones" operation.
    // When the application is constructed it should still amend the paths. So we keep the originals
    // around, and even create them if they don't exist, yet.
    if (!coreappdata()->app_libpaths)
        libraryPaths();

    if (coreappdata()->manual_libpaths)
        *(coreappdata()->manual_libpaths) = paths;
    else
        coreappdata()->manual_libpaths.reset(new QStringList(paths));

    locker.unlock();
    QFactoryLoader::refreshAll();
}

/*!
  Prepends \a path to the beginning of the library path list, ensuring that
  it is searched for libraries first. If \a path is empty or already in the
  path list, the path list is not changed.

  The default path list consists of a single entry, the installation
  directory for plugins.  The default installation directory for plugins
  is \c INSTALL/plugins, where \c INSTALL is the directory where Qt was
  installed.

  The library paths are reset to the default when an instance of
  QCoreApplication is destructed.

  \sa removeLibraryPath(), libraryPaths(), setLibraryPaths()
 */
void QCoreApplication::addLibraryPath(const QString &path)
{
    if (path.isEmpty())
        return;

    QString canonicalPath = QDir(path).canonicalPath();
    if (canonicalPath.isEmpty())
        return;

    QMutexLocker locker(libraryPathMutex());

    QStringList *libpaths = coreappdata()->manual_libpaths.data();
    if (libpaths) {
        if (libpaths->contains(canonicalPath))
            return;
    } else {
        // make sure that library paths are initialized
        libraryPaths();
        QStringList *app_libpaths = coreappdata()->app_libpaths.data();
        if (app_libpaths->contains(canonicalPath))
            return;

        coreappdata()->manual_libpaths.reset(libpaths = new QStringList(*app_libpaths));
    }

    libpaths->prepend(canonicalPath);
    locker.unlock();
    QFactoryLoader::refreshAll();
}

/*!
    Removes \a path from the library path list. If \a path is empty or not
    in the path list, the list is not changed.

    The library paths are reset to the default when an instance of
    QCoreApplication is destructed.

    \sa addLibraryPath(), libraryPaths(), setLibraryPaths()
*/
void QCoreApplication::removeLibraryPath(const QString &path)
{
    if (path.isEmpty())
        return;

    QString canonicalPath = QDir(path).canonicalPath();
    if (canonicalPath.isEmpty())
        return;

    QMutexLocker locker(libraryPathMutex());

    QStringList *libpaths = coreappdata()->manual_libpaths.data();
    if (libpaths) {
        if (libpaths->removeAll(canonicalPath) == 0)
            return;
    } else {
        // make sure that library paths is initialized
        libraryPaths();
        QStringList *app_libpaths = coreappdata()->app_libpaths.data();
        if (!app_libpaths->contains(canonicalPath))
            return;

        coreappdata()->manual_libpaths.reset(libpaths = new QStringList(*app_libpaths));
        libpaths->removeAll(canonicalPath);
    }

    locker.unlock();
    QFactoryLoader::refreshAll();
}

#endif // QT_CONFIG(library)


/*!
    Installs an event filter \a filterObj for all native events
    received by the application in the main thread.

    The event filter \a filterObj receives events via its \l {QAbstractNativeEventFilter::}{nativeEventFilter()}
    function, which is called for all native events received in the main thread.

    The QAbstractNativeEventFilter::nativeEventFilter() function should
    return true if the event should be filtered, i.e. stopped. It should
    return false to allow normal Qt processing to continue: the native
    event can then be translated into a QEvent and handled by the standard
    Qt \l{QEvent} {event} filtering, e.g. QObject::installEventFilter().

    If multiple event filters are installed, the filter that was
    installed last is activated first.

    \note The filter function set here receives native messages,
    i.e. MSG or XCB event structs.

    \note Native event filters will be disabled in the application when the
    Qt::AA_PluginApplication attribute is set.

    For maximum portability, you should always try to use QEvent
    and QObject::installEventFilter() whenever possible.

    \sa QObject::installEventFilter()

    \since 5.0
*/
void QCoreApplication::installNativeEventFilter(QAbstractNativeEventFilter *filterObj)
{
    if (QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {
        qWarning("Native event filters are not applied when the Qt::AA_PluginApplication attribute is set");
        return;
    }

    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance(QCoreApplicationPrivate::theMainThread);
    if (!filterObj || !eventDispatcher)
        return;
    eventDispatcher->installNativeEventFilter(filterObj);
}

/*!
    Removes an event \a filterObject from this object. The
    request is ignored if such an event filter has not been installed.

    All event filters for this object are automatically removed when
    this object is destroyed.

    It is always safe to remove an event filter, even during event
    filter activation (i.e. from the nativeEventFilter() function).

    \sa installNativeEventFilter()
    \since 5.0
*/
void QCoreApplication::removeNativeEventFilter(QAbstractNativeEventFilter *filterObject)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (!filterObject || !eventDispatcher)
        return;
    eventDispatcher->removeNativeEventFilter(filterObject);
}

/*!
    \deprecated

    This function returns \c true if there are pending events; otherwise
    returns \c false. Pending events can be either from the window
    system or posted events using postEvent().

    \note this function is not thread-safe. It may only be called in the main
    thread and only if there are no other threads running in the application
    (including threads Qt starts for its own purposes).

    \sa QAbstractEventDispatcher::hasPendingEvents()
*/
#if QT_DEPRECATED_SINCE(5, 3)
bool QCoreApplication::hasPendingEvents()
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (eventDispatcher)
        return eventDispatcher->hasPendingEvents();
    return false;
}
#endif

/*!
    Returns a pointer to the event dispatcher object for the main thread. If no
    event dispatcher exists for the thread, this function returns 0.
*/
QAbstractEventDispatcher *QCoreApplication::eventDispatcher()
{
    if (QCoreApplicationPrivate::theMainThread)
        return QCoreApplicationPrivate::theMainThread.load()->eventDispatcher();
    return 0;
}

/*!
    Sets the event dispatcher for the main thread to \a eventDispatcher. This
    is only possible as long as there is no event dispatcher installed yet. That
    is, before QCoreApplication has been instantiated. This method takes
    ownership of the object.
*/
void QCoreApplication::setEventDispatcher(QAbstractEventDispatcher *eventDispatcher)
{
    QThread *mainThread = QCoreApplicationPrivate::theMainThread;
    if (!mainThread)
        mainThread = QThread::currentThread(); // will also setup theMainThread
    mainThread->setEventDispatcher(eventDispatcher);
}



#include "qthread.h"
#include "qthread_p.h"
#include "qthreadstorage.h"
#include "qmutex.h"

#include <qcoreapplication.h>
#include <qpointer.h>

#include <private/qcoreapplication_p.h>
#include <private/qeventdispatcher_win_p.h>


#include <qt_windows.h>

#include <process.h>


QT_BEGIN_NAMESPACE


void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread);
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID);

static DWORD qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
void qt_create_tls()
{
	// requires a slot-id for all threads, 
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES)
        return;
    static QBasicMutex mutex;
    QMutexLocker locker(&mutex);
    qt_current_thread_data_tls_index = TlsAlloc();
}

static void qt_free_tls()
{
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES) {
        TlsFree(qt_current_thread_data_tls_index);
        qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
    }
}


void QThreadData::clearCurrentThreadData()
{
    TlsSetValue(qt_current_thread_data_tls_index, 0);
}

QThreadData *QThreadData::current(bool createIfNecessary)
{
    qt_create_tls();
    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
	// create a new one
    if (!threadData && createIfNecessary) {
        threadData = new QThreadData;
        // This needs to be called prior to new AdoptedThread() to
        // avoid recursion.
        TlsSetValue(qt_current_thread_data_tls_index, threadData);
        QT_TRY {
            threadData->thread = new QAdoptedThread(threadData);
        } QT_CATCH(...) {
            TlsSetValue(qt_current_thread_data_tls_index, 0);
            threadData->deref();
            threadData = 0;
            QT_RETHROW;
        }
        threadData->deref();
        threadData->isAdopted = true;
        threadData->threadId.store(reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId())));

        if (!QCoreApplicationPrivate::theMainThread) {
            QCoreApplicationPrivate::theMainThread = threadData->thread.load();
            // TODO: is there a way to reflect the branch's behavior using
            // WinRT API?
        } else {
            HANDLE realHandle = INVALID_HANDLE_VALUE;
            DuplicateHandle(GetCurrentProcess(),
                    GetCurrentThread(),
                    GetCurrentProcess(),
                    &realHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);
            qt_watch_adopted_thread(realHandle, threadData->thread);
        }
    }
    return threadData;
}


static QVector<HANDLE> qt_adopted_thread_handles;
static QVector<QThread *> qt_adopted_qthreads;
static QBasicMutex qt_adopted_thread_watcher_mutex;
static DWORD qt_adopted_thread_watcher_id = 0;
static HANDLE qt_adopted_thread_wakeup = 0;

/*!
    \internal
    Adds an adopted thread to the list of threads that Qt watches to make sure
    the thread data is properly cleaned up. This function starts the watcher
    thread if necessary.
*/
void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread)
{
    QMutexLocker lock(&qt_adopted_thread_watcher_mutex);

    if (GetCurrentThreadId() == qt_adopted_thread_watcher_id) {
        CloseHandle(adoptedThreadHandle);
        return;
    }

    qt_adopted_thread_handles.append(adoptedThreadHandle);
    qt_adopted_qthreads.append(qthread);

    // Start watcher thread if it is not already running.
    if (qt_adopted_thread_watcher_id == 0) {
        if (qt_adopted_thread_wakeup == 0) {
#ifndef Q_OS_WINRT
            qt_adopted_thread_wakeup = CreateEvent(0, false, false, 0);
#else
            qt_adopted_thread_wakeup = CreateEventEx(0, NULL, 0, EVENT_ALL_ACCESS);
#endif
            qt_adopted_thread_handles.prepend(qt_adopted_thread_wakeup);
        }

        CloseHandle(CreateThread(0, 0, qt_adopted_thread_watcher_function, 0, 0, &qt_adopted_thread_watcher_id));
    } else {
        SetEvent(qt_adopted_thread_wakeup);
    }
}

/*
    This function loops and waits for native adopted threads to finish.
    When this happens it derefs the QThreadData for the adopted thread
    to make sure it gets cleaned up properly.
*/
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID)
{
    forever {
        qt_adopted_thread_watcher_mutex.lock();

        if (qt_adopted_thread_handles.count() == 1) {
            qt_adopted_thread_watcher_id = 0;
            qt_adopted_thread_watcher_mutex.unlock();
            break;
        }

        QVector<HANDLE> handlesCopy = qt_adopted_thread_handles;
        qt_adopted_thread_watcher_mutex.unlock();

        DWORD ret = WAIT_TIMEOUT;
        int count;
        int offset;
        int loops = handlesCopy.size() / MAXIMUM_WAIT_OBJECTS;
        if (handlesCopy.size() % MAXIMUM_WAIT_OBJECTS)
            ++loops;
        if (loops == 1) {
            // no need to loop, no timeout
            offset = 0;
            count = handlesCopy.count();
#ifndef Q_OS_WINRT
            ret = WaitForMultipleObjects(handlesCopy.count(), handlesCopy.constData(), false, INFINITE);
#else
            ret = WaitForMultipleObjectsEx(handlesCopy.count(), handlesCopy.constData(), false, INFINITE, false);
#endif
        } else {
            int loop = 0;
            do {
                offset = loop * MAXIMUM_WAIT_OBJECTS;
                count = qMin(handlesCopy.count() - offset, MAXIMUM_WAIT_OBJECTS);
#ifndef Q_OS_WINRT
                ret = WaitForMultipleObjects(count, handlesCopy.constData() + offset, false, 100);
#else
                ret = WaitForMultipleObjectsEx(count, handlesCopy.constData() + offset, false, 100, false);
#endif
                loop = (loop + 1) % loops;
            } while (ret == WAIT_TIMEOUT);
        }

        if (ret == WAIT_FAILED || ret >= WAIT_OBJECT_0 + uint(count)) {
            qWarning("QThread internal error while waiting for adopted threads: %d", int(GetLastError()));
            continue;
        }

        const int handleIndex = offset + ret - WAIT_OBJECT_0;
        if (handleIndex == 0){
            // New handle to watch was added.
            continue;
        } else {
//             printf("(qt) - qt_adopted_thread_watcher_function... called\n");
            const int qthreadIndex = handleIndex - 1;

            qt_adopted_thread_watcher_mutex.lock();
            QThreadData *data = QThreadData::get2(qt_adopted_qthreads.at(qthreadIndex));
            qt_adopted_thread_watcher_mutex.unlock();
            if (data->isAdopted) {
                QThread *thread = data->thread;
                Q_ASSERT(thread);
                QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
                Q_UNUSED(thread_p)
                Q_ASSERT(!thread_p->finished);
                thread_p->finish(thread);
            }
            data->deref();

            QMutexLocker lock(&qt_adopted_thread_watcher_mutex);
            CloseHandle(qt_adopted_thread_handles.at(handleIndex));
            qt_adopted_thread_handles.remove(handleIndex);
            qt_adopted_qthreads.remove(qthreadIndex);
        }
    }

    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
    if (threadData)
        threadData->deref();

    return 0;
}

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC) && !defined(Q_OS_WINRT)


#define ULONG_PTR DWORD


typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;      // must be 0x1000
    LPCSTR szName;     // pointer to name (in user addr space)
    HANDLE dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags;     // reserved for future use, must be zero
} THREADNAME_INFO;

void qt_set_thread_name(HANDLE threadId, LPCSTR threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = threadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}
#endif // !QT_NO_DEBUG && Q_CC_MSVC && !Q_OS_WINRT





void QThreadPrivate::createEventDispatcher(QThreadData *data)
{
#ifndef Q_OS_WINRT
    QEventDispatcherWin32 *theEventDispatcher = new QEventDispatcherWin32;
#else
    QEventDispatcherWinRT *theEventDispatcher = new QEventDispatcherWinRT;
#endif
    data->eventDispatcher.storeRelease(theEventDispatcher);
    theEventDispatcher->startingUp();
}



unsigned int __stdcall  QThreadPrivate::start(void *arg)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadData *data = QThreadData::get2(thr);

    qt_create_tls();
    TlsSetValue(qt_current_thread_data_tls_index, data);
    data->threadId.store(reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId())));

    QThread::setTerminationEnabled(false);

    {
        QMutexLocker locker(&thr->d_func()->mutex);
        data->quitNow = thr->d_func()->exited;
    }

    if (data->eventDispatcher.load()) // custom event dispatcher set?
        data->eventDispatcher.load()->startingUp();
    else
        createEventDispatcher(data);

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC) && !defined(Q_OS_WINRT)
    // sets the name of the current thread.
    QByteArray objectName = thr->objectName().toLocal8Bit();
    qt_set_thread_name((HANDLE)-1,
                       objectName.isEmpty() ?
                       thr->metaObject()->className() : objectName.constData());
#endif

    emit thr->started(QThread::QPrivateSignal());
    QThread::setTerminationEnabled(true);
    thr->run();

    finish(arg);
    return 0;
}

void QThreadPrivate::finish(void *arg, bool lockAnyway)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadPrivate *d = thr->d_func();

    QMutexLocker locker(lockAnyway ? &d->mutex : 0);
    d->isInFinish = true;
    d->priority = QThread::InheritPriority;
    void **tls_data = reinterpret_cast<void **>(&d->data->tls);
    locker.unlock();
    emit thr->finished(QThread::QPrivateSignal());
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QThreadStorageData::finish(tls_data);
    locker.relock();

    QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.load();
    if (eventDispatcher) {
        d->data->eventDispatcher = 0;
        locker.unlock();
        eventDispatcher->closingDown();
        delete eventDispatcher;
        locker.relock();
    }

    d->running = false;
    d->finished = true;
    d->isInFinish = false;
    d->interruptionRequested = false;

    if (!d->waiters) {
        CloseHandle(d->handle);
        d->handle = 0;
    }

    d->id = 0;
}


Qt::HANDLE QThread::currentThreadId() 
{
    return reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId()));
}

int QThread::idealThreadCount() 
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

void QThread::yieldCurrentThread()
{
    SwitchToThread();
}

void QThread::sleep(unsigned long secs)
{
    ::Sleep(secs * 1000);
}

void QThread::msleep(unsigned long msecs)
{
    ::Sleep(msecs);
}

void QThread::usleep(unsigned long usecs)
{
    ::Sleep((usecs / 1000) + 1);
}

void QThread::start(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->isInFinish) {
        locker.unlock();
        wait();
        locker.relock();
    }

    if (d->running)
        return;

    d->running = true;
    d->finished = false;
    d->exited = false;
    d->returnCode = 0;
    d->interruptionRequested = false;

    /*
      NOTE: we create the thread in the suspended state, set the
      priority and then resume the thread.

      since threads are created with normal priority by default, we
      could get into a case where a thread (with priority less than
      NormalPriority) tries to create a new thread (also with priority
      less than NormalPriority), but the newly created thread preempts
      its 'parent' and runs at normal priority.
    */
#if defined(Q_CC_MSVC) && !defined(_DLL) // && !defined(Q_OS_WINRT)
#  ifdef Q_OS_WINRT
    // If you wish to accept the memory leaks, uncomment the part above.
    // See:
    //  https://support.microsoft.com/en-us/kb/104641
    //  https://msdn.microsoft.com/en-us/library/kdzttdcb.aspx
#    error "Microsoft documentation says this combination leaks memory every time a thread is started. " \
    "Please change your build back to -MD/-MDd or, if you understand this issue and want to continue, " \
    "edit this source file."
#  endif
    // MSVC -MT or -MTd build
    d->handle = (Qt::HANDLE) _beginthreadex(NULL, d->stackSize, QThreadPrivate::start,
                                            this, CREATE_SUSPENDED, &(d->id));
#else
    // MSVC -MD or -MDd or MinGW build
    d->handle = (Qt::HANDLE) CreateThread(NULL, d->stackSize, (LPTHREAD_START_ROUTINE)QThreadPrivate::start,
                                            this, CREATE_SUSPENDED, reinterpret_cast<LPDWORD>(&d->id));
#endif // Q_OS_WINRT

    if (!d->handle) {
        qErrnoWarning(errno, "QThread::start: Failed to create thread");
        d->running = false;
        d->finished = true;
        return;
    }

    int prio;
    d->priority = priority;
    switch (d->priority) {
    case IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    case InheritPriority:
    default:
        prio = GetThreadPriority(GetCurrentThread());
        break;
    }

    if (!SetThreadPriority(d->handle, prio)) {
        qErrnoWarning("QThread::start: Failed to set thread priority");
    }

    if (ResumeThread(d->handle) == (DWORD) -1) {
        qErrnoWarning("QThread::start: Failed to resume new thread");
    }
}

void QThread::terminate()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    if (!d->running)
        return;
    if (!d->terminationEnabled) {
        d->terminatePending = true;
        return;
    }


    TerminateThread(d->handle, 0);

    QThreadPrivate::finish(this, false);
}

bool QThread::wait(unsigned long time)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->id == GetCurrentThreadId()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }
    if (d->finished || !d->running)
        return true;

    ++d->waiters;
    locker.mutex()->unlock();

    bool ret = false;

    switch (WaitForSingleObjectEx(d->handle, time, false)) {
    case WAIT_OBJECT_0:
        ret = true;
        break;
    case WAIT_FAILED:
        qErrnoWarning("QThread::wait: Thread wait failure");
        break;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    default:
        break;
    }

    locker.mutex()->lock();
    --d->waiters;

    if (ret && !d->finished) {
        // thread was terminated by someone else

        QThreadPrivate::finish(this, false);
    }

    if (d->finished && !d->waiters) {
        CloseHandle(d->handle);
        d->handle = 0;
    }

    return ret;
}

void QThread::setTerminationEnabled(bool enabled)
{
    QThread *thr = currentThread();
    Q_ASSERT_X(thr != 0, "QThread::setTerminationEnabled()",
               "Current thread was not started with QThread.");
    QThreadPrivate *d = thr->d_func();
    QMutexLocker locker(&d->mutex);
    d->terminationEnabled = enabled;
    if (enabled && d->terminatePending) {
        QThreadPrivate::finish(thr, false);
        locker.unlock(); // don't leave the mutex locked!
#ifndef Q_OS_WINRT
        _endthreadex(0);
#else
        ExitThread(0);
#endif
    }
}

// Caller must hold the mutex
void QThreadPrivate::setPriority(QThread::Priority threadPriority)
{
    // copied from start() with a few modifications:

    int prio;
    priority = threadPriority;
    switch (priority) {
    case QThread::IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case QThread::LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case QThread::LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case QThread::NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case QThread::HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case QThread::HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case QThread::TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    case QThread::InheritPriority:
    default:
        qWarning("QThread::setPriority: Argument cannot be InheritPriority");
        return;
    }

    if (!SetThreadPriority(handle, prio)) {
        qErrnoWarning("QThread::setPriority: Failed to set thread priority");
    }
}

QT_END_NAMESPACE

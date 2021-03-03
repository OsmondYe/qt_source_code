//
//  oye :: Remove Socket related code and fucntions
//
#include "qeventdispatcher_win_p.h"

#include "qcoreapplication.h"
#include <private/qsystemlibrary_p.h>
#include "qoperatingsystemversion.h"
#include "qpair.h"
#include "qset.h"
#include "qsocketnotifier.h"
#include "qvarlengtharray.h"
#include "qwineventnotifier.h"

#include "qelapsedtimer.h"
#include "qcoreapplication_p.h"
#include <private/qthread_p.h>
#include <private/qmutexpool_p.h>

QT_BEGIN_NAMESPACE

extern uint qGlobalPostedEventsCount();

#ifndef TIME_KILL_SYNCHRONOUS
#  define TIME_KILL_SYNCHRONOUS 0x0100
#endif

#ifndef QS_RAWINPUT
#  define QS_RAWINPUT 0x0400
#endif

#ifndef WM_TOUCH
#  define WM_TOUCH 0x0240
#endif

#ifndef QT_NO_GESTURES
#ifndef WM_GESTURE
#  define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
#  define WM_GESTURENOTIFY 0x011A
#endif
#endif // QT_NO_GESTURES

enum {
    WM_QT_SOCKETNOTIFIER = WM_USER,
		
	// send / posted events,  dispatcher->sendpostedEvent()
    WM_QT_SENDPOSTEDEVENTS = WM_USER + 1,
    WM_QT_ACTIVATENOTIFIERS = WM_USER + 2,
    // timer
    SendPostedEventsWindowsTimerId = ~1u
};

class QEventDispatcherWin32Private;

#define DWORD_PTR DWORD

// win32 WaitForXXX returns
void QEventDispatcherWin32Private::activateEventNotifier(QWinEventNotifier * wen)
{
    QEvent event(QEvent::WinEventAct);
    QCoreApplication::sendEvent(wen, &event);
}

// This function is called by a workerthread
void WINAPI QT_WIN_CALLBACK qt_fast_timer_proc(uint timerId, uint /*reserved*/, DWORD_PTR user, DWORD_PTR /*reserved*/, DWORD_PTR /*reserved*/)
{
    if (!timerId) // sanity check
        return;
    WinTimerInfo *t = (WinTimerInfo*)user;
    Q_ASSERT(t);
    QCoreApplication::postEvent(t->dispatcher, new QTimerEvent(t->timerId));
}
// oye WM_QT_SENDPOSTEDEVENTS -> q->sendPostedEvents();  //QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);
LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    MSG msg;
    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wp;
    msg.lParam = lp;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    long result;
 
    if (dispatcher->filterNativeEvent(QByteArrayLiteral("windows_dispatcher_MSG"), &msg, &result)) {
        return result;
    }

    QEventDispatcherWin32 *q = (QEventDispatcherWin32 *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    QEventDispatcherWin32Private *d = q->d_func();   

    if (message == WM_QT_SOCKETNOTIFIER) {
        // socket notifier message
        int type = -1;
        switch (WSAGETSELECTEVENT(lp)) {
        case FD_READ:
        case FD_ACCEPT:
            type = 0;
            break;
        case FD_WRITE:
        case FD_CONNECT:
            type = 1;
            break;
        case FD_OOB:
            type = 2;
            break;
        case FD_CLOSE:
            type = 3;
            break;
        }
        if (type >= 0) {
            Q_ASSERT(d != 0);
            QSNDict *sn_vec[4] = { &d->sn_read, &d->sn_write, &d->sn_except, &d->sn_read };
            QSNDict *dict = sn_vec[type];

            QSockNot *sn = dict ? dict->value(wp) : 0;
            if (sn == nullptr) {
                d->postActivateSocketNotifiers();
            } else {
                Q_ASSERT(d->active_fd.contains(sn->fd));
                QSockFd &sd = d->active_fd[sn->fd];
                if (sd.selected) {
                    Q_ASSERT(sd.mask == 0);
                    d->doWsaAsyncSelect(sn->fd, 0);
                    sd.selected = false;
                }
                d->postActivateSocketNotifiers();

                // Ignore the message if a notification with the same type was
                // received previously. Suppressed message is definitely spurious.
                const long eventCode = WSAGETSELECTEVENT(lp);
                if ((sd.mask & eventCode) != eventCode) {
                    sd.mask |= eventCode;
                    QEvent event(type < 3 ? QEvent::SockAct : QEvent::SockClose);
                    QCoreApplication::sendEvent(sn->obj, &event);
                }
            }
        }
        return 0;
    } 
	else if (message == WM_QT_ACTIVATENOTIFIERS) {
        // Postpone activation if we have unhandled socket notifier messages
        // in the queue. WM_QT_ACTIVATENOTIFIERS will be posted again as a result of
        // event processing.
        MSG msg;
        if (!PeekMessage(&msg, d->internalHwnd,WM_QT_SOCKETNOTIFIER, WM_QT_SOCKETNOTIFIER, PM_NOREMOVE)
            && d->queuedSocketEvents.isEmpty()) {
            // register all socket notifiers
            for (QSFDict::iterator it = d->active_fd.begin(), end = d->active_fd.end();
                 it != end; ++it) {
                QSockFd &sd = it.value();
                if (!sd.selected) {
                    d->doWsaAsyncSelect(it.key(), sd.event);
                    // allow any event to be accepted
                    sd.mask = 0;
                    sd.selected = true;
                }
            }
        }
        d->activateNotifiersPosted = false;
        return 0;
    } 
	else if (message == WM_QT_SENDPOSTEDEVENTS || 
    // we also use a Windows timer to send posted events when the message queue is full
    (message == WM_TIMER && d->sendPostedEventsWindowsTimerId != 0&& wp == (uint)d->sendPostedEventsWindowsTimerId)) 
   {
        const int localSerialNumber = d->serialNumber.load();
		
        if (localSerialNumber != d->lastSerialNumber) {
            d->lastSerialNumber = localSerialNumber;
            q->sendPostedEvents();  //QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);
        }
        return 0;
    } else if (message == WM_TIMER) {
        d->sendTimerEvent(wp);  // wp: timerID
        return 0;
    }

    return DefWindowProc(hwnd, message, wp, lp);
}

static inline UINT inputTimerMask()
{
	// oye : QS_INPUT = {(QS_MOUSE | QS_KEY | QS_RAWINPUT)}
    UINT result = QS_TIMER | QS_INPUT | QS_RAWINPUT;
    // QTBUG 28513, QTBUG-29097, QTBUG-29435: QS_TOUCH, QS_POINTER became part of
    // QS_INPUT in Windows Kit 8. They should not be used when running on pre-Windows 8.
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows8)
        result &= ~(QS_TOUCH | QS_POINTER);
    return result;
}

// will be called just before Get/PeekMsg() is going to return to user
LRESULT QT_WIN_CALLBACK qt_GetMessageHook(int code, WPARAM wp, LPARAM lp)
{
    QEventDispatcherWin32 *q = qobject_cast<QEventDispatcherWin32 *>(QAbstractEventDispatcher::instance()); 

	// oye:  The message has been removed from the queue
    if (wp == PM_REMOVE) {        
        MSG *msg = (MSG *) lp;			
        QEventDispatcherWin32Private *d = q->d_func();
	
        const int localSerialNumber = d->serialNumber.load();
        static const UINT mask = inputTimerMask();
		// is not 
        if (HIWORD(GetQueueStatus(mask)) == 0) {
            // no more input or timer events in the message queue, we can allow posted events to be sent normally now
            if (d->sendPostedEventsWindowsTimerId != 0) {
                // stop the timer to send posted events, since we now allow the WM_QT_SENDPOSTEDEVENTS message
                KillTimer(d->internalHwnd, d->sendPostedEventsWindowsTimerId);
                d->sendPostedEventsWindowsTimerId = 0;
            }
			
            (void) d->wakeUps.fetchAndStoreRelease(0); // oye set as not wakeup
            // oye I dont known serialNumber's meaning
            if (localSerialNumber != d->lastSerialNumber  &&
                // if this message IS the one that triggers sendPostedEvents(), no need to post it again
                 (msg->hwnd != d->internalHwnd || msg->message != WM_QT_SENDPOSTEDEVENTS)) {
                PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);
            }
        } else if (d->sendPostedEventsWindowsTimerId == 0
                   && localSerialNumber != d->lastSerialNumber) {
            // start a special timer to continue delivering posted events while
            // there are still input and timer messages in the message queue
            d->sendPostedEventsWindowsTimerId = 
            	SetTimer(d->internalHwnd,SendPostedEventsWindowsTimerId,
                         0, // we specify zero, but Windows uses USER_TIMER_MINIMUM
                         NULL);
        }
        
    }
    return q->d_func()->getMessageHook ? CallNextHookEx(0, code, wp, lp) : 0;
}

// Provide class name and atom for the message window used by
// QEventDispatcherWin32Private via Q_GLOBAL_STATIC shared between threads.
struct QWindowsMessageWindowClassContext
{
    QWindowsMessageWindowClassContext();
    ~QWindowsMessageWindowClassContext();

    ATOM atom;
    wchar_t *className;
};

QWindowsMessageWindowClassContext::QWindowsMessageWindowClassContext()
    : atom(0), className(0)
{
    // make sure that multiple Qt's can coexist in the same process
    const QString qClassName = QStringLiteral("QEventDispatcherWin32_Internal_Widget")
        + QString::number(quintptr(qt_internal_proc));
    className = new wchar_t[qClassName.size() + 1];
    qClassName.toWCharArray(className);
    className[qClassName.size()] = 0;

    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = qt_internal_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(0);
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    atom = RegisterClass(&wc);
    if (!atom) {
        qErrnoWarning("%s RegisterClass() failed", qPrintable(qClassName));
        delete [] className;
        className = 0;
    }
}

QWindowsMessageWindowClassContext::~QWindowsMessageWindowClassContext()
{
    if (className) {
        UnregisterClass(className, GetModuleHandle(0));
        delete [] className;
    }
}

// what's this mean
//Q_GLOBAL_STATIC(QWindowsMessageWindowClassContext, qWindowsMessageWindowClassContext)

QWindowsMessageWindowClassContext* qWindowsMessageWindowClassContext(){
	static QWindowsMessageWindowClassContext rt;
	return &rt;
}


static HWND qt_create_internal_window(const QEventDispatcherWin32 *eventDispatcher)
{
    QWindowsMessageWindowClassContext *ctx = qWindowsMessageWindowClassContext();
    if (!ctx->atom)
        return 0;
	// oye this is the message only window
	// the only purpose of it is to dispatch message
    HWND wnd = CreateWindow(ctx->className,    // classname
                            ctx->className,    // window name
                            0,                 // style
                            0, 0, 0, 0,        // geometry
                            HWND_MESSAGE,            // parent
                            0,                 // menu handle
                            GetModuleHandle(0),     // application
                            0);                // windows creation data.

    if (!wnd) {
        qErrnoWarning("CreateWindow() for QEventDispatcherWin32 internal window failed");
        return 0;
    }

	// oye put this class eventDispatcher in _USERDATA
    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)eventDispatcher);


    return wnd;
}

static void calculateNextTimeout(WinTimerInfo *t, quint64 currentTime)
{
    uint interval = t->interval;
    if ((interval >= 20000u && t->timerType != Qt::PreciseTimer) || t->timerType == Qt::VeryCoarseTimer) {
        // round the interval, VeryCoarseTimers only have full second accuracy
        interval = ((interval + 500)) / 1000 * 1000;
    }
    t->interval = interval;
    t->timeout = currentTime + interval;
}

void QEventDispatcherWin32Private::registerTimer(WinTimerInfo *t)
{
    Q_Q(QEventDispatcherWin32);

    bool ok = false;
    calculateNextTimeout(t, qt_msectime());
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->timerId));
        ok = true;
    } else if (interval < 20u || t->timerType == Qt::PreciseTimer) {
        // 3/2016: Although MSDN states timeSetEvent() is deprecated, the function
        // is still deemed to be the most reliable precision timer.
        t->fastTimerId = timeSetEvent(interval, 1, qt_fast_timer_proc, DWORD_PTR(t),
                                      TIME_CALLBACK_FUNCTION | TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
        ok = t->fastTimerId;
    }

    if (!ok) {
        // user normal timers for (Very)CoarseTimers, or if no more multimedia timers available
        ok = SetTimer(internalHwnd, t->timerId, interval, 0);
    }

    if (!ok)
        qErrnoWarning("QEventDispatcherWin32::registerTimer: Failed to create a timer");
}

void QEventDispatcherWin32Private::unregisterTimer(WinTimerInfo *t)
{
    if (t->interval == 0) {
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else if (t->fastTimerId != 0) {
        timeKillEvent(t->fastTimerId);
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else if (internalHwnd) {
        KillTimer(internalHwnd, t->timerId);
    }
    t->timerId = -1;
    if (!t->inTimerEvent)
        delete t;
}

void QEventDispatcherWin32Private::sendTimerEvent(int timerId)
{
    WinTimerInfo *t = timerDict.value(timerId);
    if (t && !t->inTimerEvent) {
        // send event, but don't allow it to recurse
        t->inTimerEvent = true;

        // recalculate next emission
        calculateNextTimeout(t, qt_msectime());

        QTimerEvent e(t->timerId);
        QCoreApplication::sendEvent(t->obj, &e);

        // timer could have been removed
        if (t->timerId == -1) {
            delete t;
        } else {
            t->inTimerEvent = false;
        }
    }
}



void QEventDispatcherWin32::createInternalHwnd()
{
    QEventDispatcherWin32Private * const d = d_func();

	// oye:  sanity check
    if (d->internalHwnd)    return;

	// oye: give it a message only window
    d->internalHwnd = qt_create_internal_window(this);

	// 
    installMessageHook();

    // start all normal timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->registerTimer(d->timerVec.at(i));
}

void QEventDispatcherWin32::installMessageHook()
{
    QEventDispatcherWin32Private * const d = d_func();

    if (d->getMessageHook) return;

    // setup GetMessage hook needed to drive our posted events
    d->getMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC) qt_GetMessageHook, NULL, GetCurrentThreadId());
}

void QEventDispatcherWin32::uninstallMessageHook()
{
    QEventDispatcherWin32Private * const d = d_func();

    if (d->getMessageHook)
        UnhookWindowsHookEx(d->getMessageHook);
    d->getMessageHook = 0;
}

//oye::  Modified, remove socket related
bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    QEventDispatcherWin32Private * const d = d_func();

    if (!d->internalHwnd) {
        createInternalHwnd();
        wakeUp(); // trigger a call to sendPostedEvents()
    }

    d->interrupt = false;
    emit awake();

    bool canWait;
    bool retVal = false;
    bool seenWM_QT_SENDPOSTEDEVENTS = false;
    bool needWM_QT_SENDPOSTEDEVENTS = false;
    do {
        DWORD waitRet = 0;
        HANDLE pHandles[MAXIMUM_WAIT_OBJECTS - 1];
        QVarLengthArray<MSG> processedTimers;
        while (!d->interrupt) {
            DWORD nCount = d->winEventNotifierList.count();
            MSG msg;
            bool haveMessage;
            if (!(flags & QEventLoop::ExcludeUserInputEvents) && !d->queuedUserInputEvents.isEmpty()) {
                // process queued user input events
                haveMessage = true;
                msg = d->queuedUserInputEvents.takeFirst();
            }  
			else 
			{
				//OYE,  Fetch Msg From Win32 Here
				//      both Peek windows and thread msg, and no-filter, peek all msg
				//      dequeue the msg from queue ,also trigger WindowsHook to be called-back
                haveMessage = PeekMessage(&msg, 0, 0, 0, PM_REMOVE); 
                if (haveMessage) {
                    if ((flags & QEventLoop::ExcludeUserInputEvents) &&
						// Input and touch msg
                       ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) ||(msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)                            
                            || msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL|| msg.message == WM_TOUCH  || msg.message == WM_GESTURE  || msg.message == WM_GESTURENOTIFY
                            || msg.message == WM_CLOSE)) {
                        // queue user input events for later processing
                        d->queuedUserInputEvents.append(msg);
                        continue;  // **** check the else's if above the else
                    }                    
                }
            }
			
            if (!haveMessage) {
                // no message - check for signalled objects
                for (int i=0; i<(int)nCount; i++)
                    pHandles[i] = d->winEventNotifierList.at(i)->handle();
				// oye, check here, if it returns a normal Msg
                waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, 0, QS_ALLINPUT, MWMO_ALERTABLE);
				// WAIT_OBJECT_0 + nCount , -> it is message arrived
                if ((haveMessage = (waitRet == WAIT_OBJECT_0 + nCount))) {
                    // a new message has arrived, process it
                    continue;
                }
            }
            if (haveMessage) {                

                if (d->internalHwnd == msg.hwnd && msg.message == WM_QT_SENDPOSTEDEVENTS) {
                    if (seenWM_QT_SENDPOSTEDEVENTS) {
                        needWM_QT_SENDPOSTEDEVENTS = true;
                        continue;
                    }
                    seenWM_QT_SENDPOSTEDEVENTS = true;
                } else if (msg.message == WM_TIMER) {
                    // avoid live-lock by keeping track of the timers we've already sent
                    bool found = false;
                    for (int i = 0; !found && i < processedTimers.count(); ++i) {
                        const MSG processed = processedTimers.constData()[i];
                        found = (processed.wParam == msg.wParam && processed.hwnd == msg.hwnd && processed.lParam == msg.lParam);
                    }
                    if (found)
                        continue;
                    processedTimers.append(msg);
                } else if (msg.message == WM_QUIT) {
                    if (QCoreApplication::instance())
                        QCoreApplication::instance()->quit();
                    return false;
                }

                if (!filterNativeEvent(QByteArrayLiteral("windows_generic_MSG"), &msg, 0)) {
					// may be to other window, but the QT frame window
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } 
			else if (waitRet - WAIT_OBJECT_0 < nCount) {
                d->activateEventNotifier(d->winEventNotifierList.at(waitRet - WAIT_OBJECT_0));
            } 
			// oye:: no message and no event, break
			else {
                // nothing todo so break
                break;
            }
            retVal = true;
        }

        // still nothing - wait for message or signalled objects
        canWait = (!retVal && !d->interrupt && (flags & QEventLoop::WaitForMoreEvents));
        if (canWait) {
            DWORD nCount = d->winEventNotifierList.count();
            for (int i=0; i<(int)nCount; i++)
                pHandles[i] = d->winEventNotifierList.at(i)->handle();

            emit aboutToBlock();
            waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
            emit awake();
            if (waitRet - WAIT_OBJECT_0 < nCount) {
                d->activateEventNotifier(d->winEventNotifierList.at(waitRet - WAIT_OBJECT_0));
                retVal = true;
            }
        }
    } while (canWait);

    if (!seenWM_QT_SENDPOSTEDEVENTS && (flags & QEventLoop::EventLoopExec) == 0) {
        // when called "manually", always send posted events
        sendPostedEvents();
    }

    if (needWM_QT_SENDPOSTEDEVENTS)
        PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);

    return retVal;
}



bool QEventDispatcherWin32::hasPendingEvents()
{
    MSG msg;
    return qGlobalPostedEventsCount() || PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

void QEventDispatcherWin32::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWin32::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    QEventDispatcherWin32Private * const d = d_func();

    // exiting ... do not register new timers
    // (QCoreApplication::closingDown() is set too late to be used here)
    if (d->closingDown)
        return;

    WinTimerInfo *t = new WinTimerInfo;
    t->dispatcher = this;
    t->timerId  = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj  = object;
    t->inTimerEvent = false;
    t->fastTimerId = 0;

    if (d->internalHwnd)
        d->registerTimer(t);

    d->timerVec.append(t);                      // store in timer vector
    d->timerDict.insert(t->timerId, t);          // store timers in dict
}

bool QEventDispatcherWin32::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::unregisterTimer: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (thread() != currentThread) {
        qWarning("QEventDispatcherWin32::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    QEventDispatcherWin32Private * const d = d_func();
    if (d->timerVec.isEmpty() || timerId <= 0)
        return false;

    WinTimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return false;

    d->timerDict.remove(t->timerId);
    d->timerVec.removeAll(t);
    d->unregisterTimer(t);
    return true;
}

bool QEventDispatcherWin32::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QEventDispatcherWin32::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    QEventDispatcherWin32Private * const d = d_func();
    if (d->timerVec.isEmpty())
        return false;
    WinTimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->obj == object) {                // object found
            d->timerDict.remove(t->timerId);
            d->timerVec.removeAt(i);
            d->unregisterTimer(t);
            --i;
        }
    }
    return true;
}

QList<QEventDispatcherWin32::TimerInfo>
QEventDispatcherWin32::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherWin32:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QEventDispatcherWin32);
    QList<TimerInfo> list;
    for (int i = 0; i < d->timerVec.size(); ++i) {
        const WinTimerInfo *t = d->timerVec.at(i);
        if (t && t->obj == object)
            list << TimerInfo(t->timerId, t->interval, t->timerType);
    }
    return list;
}

bool QEventDispatcherWin32::registerEventNotifier(QWinEventNotifier *notifier)
{
    if (!notifier) {
        qWarning("QWinEventNotifier: Internal error");
        return false;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QWinEventNotifier: event notifiers cannot be enabled from another thread");
        return false;
    }

    QEventDispatcherWin32Private * const d = d_func();

    if (d->winEventNotifierList.contains(notifier))
        return true;

    if (d->winEventNotifierList.count() >= MAXIMUM_WAIT_OBJECTS - 2) {
        qWarning("QWinEventNotifier: Cannot have more than %d enabled at one time", MAXIMUM_WAIT_OBJECTS - 2);
        return false;
    }
    d->winEventNotifierList.append(notifier);
    return true;
}

void QEventDispatcherWin32::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    if (!notifier) {
        qWarning("QWinEventNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QWinEventNotifier: event notifiers cannot be disabled from another thread");
        return;
    }

    QEventDispatcherWin32Private * const d = d_func();

    int i = d->winEventNotifierList.indexOf(notifier);
    if (i != -1)
        d->winEventNotifierList.takeAt(i);
}

void QEventDispatcherWin32::activateEventNotifiers()
{
    QEventDispatcherWin32Private * const d = d_func();
    //### this could break if events are removed/added in the activation
    for (int i=0; i<d->winEventNotifierList.count(); i++) {
        if (WaitForSingleObjectEx(d->winEventNotifierList.at(i)->handle(), 0, TRUE) == WAIT_OBJECT_0)
            d->activateEventNotifier(d->winEventNotifierList.at(i));
    }
}

int QEventDispatcherWin32::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::remainingTime: invalid argument");
        return -1;
    }
#endif

    QEventDispatcherWin32Private * const d = d_func();

    if (d->timerVec.isEmpty())
        return -1;

    quint64 currentTime = qt_msectime();

    WinTimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->timerId == timerId) {                // timer found
            if (currentTime < t->timeout) {
                // time to wait
                return t->timeout - currentTime;
            } else {
                return 0;
            }
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWin32::remainingTime: timer id %d not found", timerId);
#endif

    return -1;
}

void QEventDispatcherWin32::wakeUp()
{
    QEventDispatcherWin32Private * const d = d_func();
    d->serialNumber.ref();
    if (d->internalHwnd && d->wakeUps.testAndSetAcquire(0, 1)) {
        // post a WM_QT_SENDPOSTEDEVENTS to this thread if there isn't one already pending
        PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);
    }
}

void QEventDispatcherWin32::interrupt()
{
    QEventDispatcherWin32Private * const d = d_func();
    d->interrupt = true;
    wakeUp();
}

void QEventDispatcherWin32::closingDown()
{
    QEventDispatcherWin32Private * const d = d_func();

    // clean up any socketnotifiers
    while (!d->sn_read.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_read.begin()))->obj);
    while (!d->sn_write.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_write.begin()))->obj);
    while (!d->sn_except.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_except.begin()))->obj);
    Q_ASSERT(d->active_fd.isEmpty());

    // clean up any timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->unregisterTimer(d->timerVec.at(i));
    d->timerVec.clear();
    d->timerDict.clear();

    d->closingDown = true;

    uninstallMessageHook();
}

bool QEventDispatcherWin32::event(QEvent *e)
{
    QEventDispatcherWin32Private * const d = d_func();
    if (e->type() == QEvent::ZeroTimerEvent) {
        QZeroTimerEvent *zte = static_cast<QZeroTimerEvent*>(e);
        WinTimerInfo *t = d->timerDict.value(zte->timerId());
        if (t) {
            t->inTimerEvent = true;

            QTimerEvent te(zte->timerId());
            QCoreApplication::sendEvent(t->obj, &te);

            // timer could have been removed
            if (t->timerId == -1) {
                delete t;
            } else {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(zte->timerId()));
                }

                t->inTimerEvent = false;
            }
        }
        return true;
    } else if (e->type() == QEvent::Timer) {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        d->sendTimerEvent(te->timerId());
    }
    return QAbstractEventDispatcher::event(e);
}

void QEventDispatcherWin32::sendPostedEvents()
{
    QEventDispatcherWin32Private * const d = d_func();
    QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);
}

HWND QEventDispatcherWin32::internalHwnd()
{
    QEventDispatcherWin32Private * const d = d_func();
    createInternalHwnd();
    return d->internalHwnd;
}

QT_END_NAMESPACE

#ifndef QEVENTDISPATCHER_WIN_P_H
#define QEVENTDISPATCHER_WIN_P_H


#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qt_windows.h"
#include "QtCore/qhash.h"

#include "qabstracteventdispatcher_p.h"


typedef HWND void*;


QT_BEGIN_NAMESPACE

class QWinEventNotifier;
class QEventDispatcherWin32Private;

// forward declaration
LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
quint64 qt_msectime();

class  QEventDispatcherWin32 : public QAbstractEventDispatcher
{
    //Q_OBJECT
    //Q_DECLARE_PRIVATE(QEventDispatcherWin32)

protected:
    void createInternalHwnd();
    void installMessageHook();
    void uninstallMessageHook();

public:
    explicit QEventDispatcherWin32(QObject *parent = 0)
	 : QAbstractEventDispatcher(*new QEventDispatcherWin32Private, parent)
	{
	}
    ~QEventDispatcherWin32(){}
	
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
    bool hasPendingEvents();
    void interrupt();
	

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    bool registerEventNotifier(QWinEventNotifier *notifier);
    void unregisterEventNotifier(QWinEventNotifier *notifier);
    void activateEventNotifiers();

    int remainingTime(int timerId);

    void wakeUp();

    void flush(){}

    void startingUp(){ }
    void closingDown();

    bool event(QEvent *e);

    HWND internalHwnd();

protected:
    QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent = 0): QAbstractEventDispatcher(dd, parent){ }
    virtual void sendPostedEvents();
    void doUnregisterSocketNotifier(QSocketNotifier *notifier);

private:
    friend LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
    friend LRESULT QT_WIN_CALLBACK qt_GetMessageHook(int, WPARAM, LPARAM);
};

struct QSockNot {
    QSocketNotifier *obj;
    int fd;
};
typedef QHash<int, QSockNot *> QSNDict;

struct QSockFd {
    long event;
    long mask;
    bool selected;

    explicit inline QSockFd(long ev = 0, long ma = 0) : event(ev), mask(ma), selected(false) { }
};
typedef QHash<int, QSockFd> QSFDict;

struct WinTimerInfo {                           // internal timer info
    QObject *dispatcher;
    int timerId;
    int interval;
    Qt::TimerType timerType;
    quint64 timeout;                            // - when to actually fire
    QObject *obj;                               // - object to receive events
    bool inTimerEvent;
    UINT fastTimerId;
};

class QZeroTimerEvent : public QTimerEvent
{
public:
    explicit inline QZeroTimerEvent(int timerId)
        : QTimerEvent(timerId)
    { t = QEvent::ZeroTimerEvent; }
};

typedef QList<WinTimerInfo*>  WinTimerVec;      // vector of TimerInfo structs
typedef QHash<int, WinTimerInfo*> WinTimerDict; // fast dict of timers

class  QEventDispatcherWin32Private : public QAbstractEventDispatcherPrivate
{
   // Q_DECLARE_PUBLIC(QEventDispatcherWin32)
public:
    QEventDispatcherWin32Private(): 
	  threadId(GetCurrentThreadId()), interrupt(false), closingDown(false), internalHwnd(0),
      getMessageHook(0), serialNumber(0), lastSerialNumber(0), sendPostedEventsWindowsTimerId(0),
      wakeUps(0)
    , activateNotifiersPosted(false)
	{
	}
    ~QEventDispatcherWin32Private(){
    	if (internalHwnd)
       	 	DestroyWindow(internalHwnd);
	}

    DWORD threadId;

    bool interrupt;
    bool closingDown;

    // internal window handle used for socketnotifiers/timers/etc
    HWND internalHwnd;
    HHOOK getMessageHook;

    // for controlling when to send posted events
    QAtomicInt serialNumber;  //oye every wakeUp it will ++;
    int lastSerialNumber;
	int sendPostedEventsWindowsTimerId;
	
    QAtomicInt wakeUps; // whether has wakeup [0,1]

    // timers
    WinTimerVec timerVec;
    WinTimerDict timerDict;
    void registerTimer(WinTimerInfo *t);
    void unregisterTimer(WinTimerInfo *t);
    void sendTimerEvent(int timerId);

    // socket notifiers
    QSNDict sn_read;
    QSNDict sn_write;
    QSNDict sn_except;
    QSFDict active_fd;
    bool activateNotifiersPosted;
    void postActivateSocketNotifiers();
    void doWsaAsyncSelect(int socket, long event);

    QList<QWinEventNotifier *> winEventNotifierList;			// waitForXXX happens
    void activateEventNotifier(QWinEventNotifier * wen);

	// oye only occure in MSG_Process pump processEvents();
	// store WM_Keyboard, WM_Mouse, WM_Touch WM_Gesture
	// pure win32, ֻ���ݴ���һ��, ���ջ��ǻ�����win32��·ȥ�ɷ�
    QList<MSG> queuedUserInputEvents; 
    QList<MSG> queuedSocketEvents;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_WIN_P_H

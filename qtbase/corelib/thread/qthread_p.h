#ifndef QTHREAD_P_H
#define QTHREAD_P_H


#include "qplatformdefs.h"
#include "QtCore/qthread.h"
#include "QtCore/qmutex.h"
#include "QtCore/qstack.h"
#include "QtCore/qwaitcondition.h"
#include "QtCore/qmap.h"
#include "QtCore/qcoreapplication.h"
#include "private/qobject_p.h"

#include <algorithm>


QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QEventLoop;

class QPostEvent
{
public:
    QObject *receiver;
    QEvent *event;
    int priority;
    inline QPostEvent()
        : receiver(0), event(0), priority(0)
    { }
    inline QPostEvent(QObject *r, QEvent *e, int p)
        : receiver(r), event(e), priority(p)
    { }
};
//Q_DECLARE_TYPEINFO(QPostEvent, Q_MOVABLE_TYPE);

inline bool operator<(const QPostEvent &first, const QPostEvent &second)
{
    return first.priority > second.priority;
}


class QPostEventList : public QVector<QPostEvent>
{
public:
    // recursion == recursion count for sendPostedEvents()
    int recursion;

    // sendOffset == the current event to start sending
    int startOffset;
    // insertionOffset == set by sendPostedEvents to tell postEvent() where to start insertions
    int insertionOffset;

    QMutex mutex;  // oye this list will be protected by mutex,

    inline QPostEventList()
        : QVector<QPostEvent>(), recursion(0), startOffset(0), insertionOffset(0)
    { }

    void addEvent(const QPostEvent &ev) {
        int priority = ev.priority;
        if (isEmpty() ||
            constLast().priority >= priority ||
            insertionOffset >= size()) {
            // optimization: we can simply append if the last event in
            // the queue has higher or equal priority
            append(ev);
        } else {
            // insert event in descending priority order, using upper
            // bound for a given priority (to ensure proper ordering
            // of events with the same priority)
            QPostEventList::iterator at = std::upper_bound(begin() + insertionOffset, end(), ev);
            insert(at, ev);
        }
    }
private:
    //hides because they do not keep that list sorted. addEvent must be used
    using QVector<QPostEvent>::append;
    using QVector<QPostEvent>::insert;
};



class  QDaemonThread : public QThread
{
public:
    QDaemonThread(QObject *parent = 0);
    ~QDaemonThread(){}
};

class QThreadPrivate : public QObjectPrivate
{
    //Q_DECLARE_PUBLIC(QThread)

public:
    mutable QMutex mutex;
    QAtomicInt quitLockRef;

    bool running;
    bool finished;
    bool isInFinish; //when in QThreadPrivate::finish
    bool interruptionRequested;

    bool exited;
    int returnCode;

    uint stackSize;
    QThread::Priority priority;

    Qt::HANDLE handle;
    unsigned int id;
    int waiters;
    bool terminationEnabled;
	bool terminatePending;
    QThreadData *data;
	
public:
	
    QThreadPrivate(QThreadData *d = 0);
    ~QThreadPrivate();

    void setPriority(QThread::Priority prio);

	
    static QThread *threadForId(int id);

	// oye: will be attached by _beginthreadex and send param as this
    static unsigned int  start(void *);
    static void finish(void *, bool lockAnyway=true);

    static void createEventDispatcher(QThreadData *data);

    void ref()    {        quitLockRef.ref();    }

    void deref()
    {
        if (!quitLockRef.deref() && running) {
            QCoreApplication::instance()->postEvent(q_ptr, new QEvent(QEvent::Quit));
        }
    }
};


//oye: each thread has this class with different class object by thread local data
class QThreadData
{
private:
    QAtomicInt _ref;

public:
    int loopLevel;
    int scopeLevel; // using by QScopedScopeLevelCounter

	// MSG 3tuple[ eventLoops, postEvetnList, eventDispather ] 
    QStack<QEventLoop *> eventLoops;
    QPostEventList postEventList;
	// created by QThreadPrivate::createEventDispatcher()
    QAtomicPointer<QAbstractEventDispatcher> eventDispatcher;    //using QEventDispatcherWin32 

	// thread specifics
    QAtomicPointer<QThread> thread;			// in QThread::QThread   d->data->thread = this;
    QAtomicPointer<void> threadId;   							// GetCurrentThreadId
    QVector<void *> tls;
	
    FlaggedDebugSignatures flaggedSignatures;

    bool quitNow;
    bool canWait;
    bool isAdopted;  // true-> if called new QAdoptedThread(threadData);
    bool requiresCoreApplication;
	

public:
    QThreadData(int initialRefCount = 1): _ref(initialRefCount), loopLevel(0), scopeLevel(0),
      eventDispatcher(0),
      quitNow(false), canWait(true), isAdopted(false), requiresCoreApplication(true)
	{
	    
	}
    ~QThreadData();

    static  QThreadData *current(bool createIfNecessary = true);
    static void clearCurrentThreadData();
    static QThreadData *get2(QThread *thread)  {  return thread->d_func()->data; }


    void ref(){(void) _ref.ref();} // _ref+1;
	
    void deref(){if (!_ref.deref()) delete this;}
    inline bool hasEventDispatcher() const    { return eventDispatcher.load() != 0; }

    bool canWaitLocked()
    {
        QMutexLocker locker(&postEventList.mutex);
        return canWait;
    }

    // This class provides per-thread (by way of being a QThreadData
    // member) storage for qFlagLocation()
    class FlaggedDebugSignatures
    {
        static const uint Count = 2;

        uint idx;
        const char* locations[Count];

    public:
        FlaggedDebugSignatures() : idx(0)
        { std::fill_n(locations, Count, static_cast<char*>(0)); }

        void store(const char* method)
        { locations[idx++ % Count] = method; }

        bool contains(const char *method) const
        { return std::find(locations, locations + Count, method) != locations + Count; }
    };


};

class QScopedScopeLevelCounter
{
    QThreadData *threadData;
public:
    inline QScopedScopeLevelCounter(QThreadData *threadData)
        : threadData(threadData)
    { ++threadData->scopeLevel; }
    inline ~QScopedScopeLevelCounter()
    { --threadData->scopeLevel; }
};

// thread wrapper for the main() thread
class QAdoptedThread : public QThread
{
    //Q_DECLARE_PRIVATE(QThread)
    inline QThreadPrivate* d_func() { return reinterpret_cast<QThreadPrivate *>(qGetPtrHelper(d_ptr)); } 

public:
    QAdoptedThread(QThreadData *data = 0): QThread(*new QThreadPrivate(data))	{
		// thread should be running and not finished for the lifetime
		// of the application (even if QCoreApplication goes away)
		d_func()->running = true;
		d_func()->finished = false;
		init();    
	}
	
    ~QAdoptedThread(){}
    
	
	void init()	{
	    d_func()->handle = GetCurrentThread();
	    d_func()->id = GetCurrentThreadId();
	}

private:
	
	void run()	{
		// this function should never be called
		qFatal("QAdoptedThread::run(): Internal error, this implementation should never be called.");
	}
};

QT_END_NAMESPACE

#endif // QTHREAD_P_H

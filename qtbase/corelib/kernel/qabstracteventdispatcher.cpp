#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"
#include "qabstractnativeeventfilter.h"

#include "qthread.h"
#include <private/qthread_p.h>
#include <private/qcoreapplication_p.h>
#include <private/qfreelist_p.h>

QT_BEGIN_NAMESPACE

// we allow for 2^24 = 8^8 = 16777216 simultaneously running timers
struct QtTimerIdFreeListConstants : public QFreeListDefaultConstants
{
    enum
    {
        InitialNextValue = 1,
        BlockCount = 6
    };

    static const int Sizes[BlockCount];
};

enum {
    Offset0 = 0x00000000,
    Offset1 = 0x00000040,
    Offset2 = 0x00000100,
    Offset3 = 0x00001000,
    Offset4 = 0x00010000,
    Offset5 = 0x00100000,

    Size0 = Offset1  - Offset0,
    Size1 = Offset2  - Offset1,
    Size2 = Offset3  - Offset2,
    Size3 = Offset4  - Offset3,
    Size4 = Offset5  - Offset4,
    Size5 = QtTimerIdFreeListConstants::MaxIndex - Offset5
};

const int QtTimerIdFreeListConstants::Sizes[QtTimerIdFreeListConstants::BlockCount] = {
    Size0,
    Size1,
    Size2,
    Size3,
    Size4,
    Size5
};

typedef QFreeList<void, QtTimerIdFreeListConstants> QtTimerIdFreeList;
Q_GLOBAL_STATIC(QtTimerIdFreeList, timerIdFreeList)

int QAbstractEventDispatcherPrivate::allocateTimerId()
{
    return timerIdFreeList()->next();
}

void QAbstractEventDispatcherPrivate::releaseTimerId(int timerId)
{
    // this function may be called by a global destructor after
    // timerIdFreeList() has been destructed
    if (QtTimerIdFreeList *fl = timerIdFreeList())
        fl->release(timerId);
}

QAbstractEventDispatcher::QAbstractEventDispatcher(QObject *parent)
    : QObject(*new QAbstractEventDispatcherPrivate, parent) {}

/*!
    \internal
*/
QAbstractEventDispatcher::QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &dd,
                                                   QObject *parent)
    : QObject(dd, parent) {}

/*!
    Destroys the event dispatcher.
*/
QAbstractEventDispatcher::~QAbstractEventDispatcher()
{ }


QAbstractEventDispatcher *QAbstractEventDispatcher::instance(QThread *thread)
{
    QThreadData *data = thread ? QThreadData::get2(thread) : QThreadData::current();
    return data->eventDispatcher.load();
}

int QAbstractEventDispatcher::registerTimer(int interval, Qt::TimerType timerType, QObject *object)
{
    int id = QAbstractEventDispatcherPrivate::allocateTimerId();
    registerTimer(id, interval, timerType, object);
    return id;
}

void QAbstractEventDispatcher::startingUp()
{ }

/*!
    \internal
*/
void QAbstractEventDispatcher::closingDown()
{ }

void QAbstractEventDispatcher::installNativeEventFilter(QAbstractNativeEventFilter *filterObj)
{
    QAbstractEventDispatcherPrivate * const d = d_func();

    // clean up unused items in the list
    d->eventFilters.removeAll(0);
    d->eventFilters.removeAll(filterObj);
    d->eventFilters.prepend(filterObj);
}


void QAbstractEventDispatcher::removeNativeEventFilter(QAbstractNativeEventFilter *filter)
{
    QAbstractEventDispatcherPrivate * const d = d_func();
    for (int i = 0; i < d->eventFilters.count(); ++i) {
        if (d->eventFilters.at(i) == filter) {
            d->eventFilters[i] = 0;
            break;
        }
    }
}

bool QAbstractEventDispatcher::filterNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    QAbstractEventDispatcherPrivate * const d = d_func();
    if (!d->eventFilters.isEmpty()) {
        // Raise the loopLevel so that deleteLater() calls in or triggered
        // by event_filter() will be processed from the main event loop.
        QScopedScopeLevelCounter scopeLevelCounter(d->threadData);
        for (int i = 0; i < d->eventFilters.size(); ++i) {
            QAbstractNativeEventFilter *filter = d->eventFilters.at(i);
            if (!filter)
                continue;
            if (filter->nativeEventFilter(eventType, message, result))
                return true;
        }
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qabstracteventdispatcher.cpp"

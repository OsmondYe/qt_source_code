#include "qwineventnotifier.h"
#include "qeventdispatcher_win_p.h"
#include "qcoreapplication.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

class QWinEventNotifierPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWinEventNotifier)
public:
    QWinEventNotifierPrivate()
    : handleToEvent(0), enabled(false) {}
    QWinEventNotifierPrivate(HANDLE h, bool e)
    : handleToEvent(h), enabled(e) {}

    HANDLE handleToEvent;
    bool enabled;
};


QWinEventNotifier::QWinEventNotifier(QObject *parent)
  : QObject(*new QWinEventNotifierPrivate, parent)
{}



QWinEventNotifier::QWinEventNotifier(HANDLE hEvent, QObject *parent)
 : QObject(*new QWinEventNotifierPrivate(hEvent, false), parent)
{
    Q_D(QWinEventNotifier);
    QAbstractEventDispatcher *eventDispatcher = d->threadData->eventDispatcher.load();
    if (Q_UNLIKELY(!eventDispatcher)) {
        qWarning("QWinEventNotifier: Can only be used with threads started with QThread");
        return;
    }
    eventDispatcher->registerEventNotifier(this);
    d->enabled = true;
}



QWinEventNotifier::~QWinEventNotifier()
{
    setEnabled(false);
}

void QWinEventNotifier::setHandle(HANDLE hEvent)
{
    Q_D(QWinEventNotifier);
    setEnabled(false);
    d->handleToEvent = hEvent;
}


HANDLE  QWinEventNotifier::handle() const
{
    Q_D(const QWinEventNotifier);
    return d->handleToEvent;
}


bool QWinEventNotifier::isEnabled() const
{
    Q_D(const QWinEventNotifier);
    return d->enabled;
}


void QWinEventNotifier::setEnabled(bool enable)
{
    Q_D(QWinEventNotifier);
    if (d->enabled == enable)                        // no change
        return;
    d->enabled = enable;

    QAbstractEventDispatcher *eventDispatcher = d->threadData->eventDispatcher.load();
    if (!eventDispatcher) // perhaps application is shutting down
        return;
    if (Q_UNLIKELY(thread() != QThread::currentThread())) {
        qWarning("QWinEventNotifier: Event notifiers cannot be enabled or disabled from another thread");
        return;
    }

    if (enable)
        eventDispatcher->registerEventNotifier(this);
    else
        eventDispatcher->unregisterEventNotifier(this);
}


bool QWinEventNotifier::event(QEvent * e)
{
    Q_D(QWinEventNotifier);
    if (e->type() == QEvent::ThreadChange) {
        if (d->enabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, true));
            setEnabled(false);
        }
    }
    QObject::event(e);                        // will activate filters
    if (e->type() == QEvent::WinEventAct) {
        emit activated(d->handleToEvent, QPrivateSignal());
        return true;
    }
    return false;
}

QT_END_NAMESPACE

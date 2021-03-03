#ifndef QABSTRACTEVENTDISPATCHER_P_H
#define QABSTRACTEVENTDISPATCHER_P_H


#include "QtCore/qabstracteventdispatcher.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

uint qGlobalPostedEventsCount();

class  QAbstractEventDispatcherPrivate : public QObjectPrivate
{
    //Q_DECLARE_PUBLIC(QAbstractEventDispatcher)
public:
    inline QAbstractEventDispatcherPrivate()
    { }

    QList<QAbstractNativeEventFilter *> eventFilters;

    static int allocateTimerId();
    static void releaseTimerId(int id);
};

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_P_H

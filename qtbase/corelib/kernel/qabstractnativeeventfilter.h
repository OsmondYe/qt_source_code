#ifndef QABSTRACTNATIVEEVENTFILTER_H
#define QABSTRACTNATIVEEVENTFILTER_H

#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

class QAbstractNativeEventFilterPrivate;

class  QAbstractNativeEventFilter
{
public:
    QAbstractNativeEventFilter(){}
    virtual ~QAbstractNativeEventFilter(){
    {
	    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
	    if (eventDispatcher)
	        eventDispatcher->removeNativeEventFilter(this);
	}

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) = 0;

private:
    //Q_DISABLE_COPY(QAbstractNativeEventFilter)
    QAbstractNativeEventFilterPrivate *d;
};

QT_END_NAMESPACE

#endif /* QABSTRACTNATIVEEVENTFILTER_H */

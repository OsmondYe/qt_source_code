#ifndef QEVENTLOOP_P_H
#define QEVENTLOOP_P_H

#include "qcoreapplication.h"
#include "qobject_p.h"

QT_BEGIN_NAMESPACE

class QEventLoopPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QEventLoop)
public:
    inline QEventLoopPrivate() : inExec(false)
    {
        returnCode.store(-1);
        exit.store(true);
    }

    QAtomicInt quitLockRef;
    QBasicAtomicInt exit; // bool
    QBasicAtomicInt returnCode;
    bool inExec;

    void ref()    {        quitLockRef.ref();    }

    void deref()
    {
        if (!quitLockRef.deref() && inExec) {
            QCoreApplication::instance()->postEvent(q_ptr, new QEvent(QEvent::Quit));
        }
    }
};

QT_END_NAMESPACE

#endif // QEVENTLOOP_P_H

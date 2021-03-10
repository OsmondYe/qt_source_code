#ifndef QBASICTIMER_H
#define QBASICTIMER_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

class  QBasicTimer
{
    int id;
public:
    inline QBasicTimer() : id(0) {}
    inline ~QBasicTimer() { if (id) stop(); }

    inline bool isActive() const { return id != 0; }
    inline int timerId() const { return id; }

    void start(int msec, QObject *obj);
    void start(int msec, Qt::TimerType timerType, QObject *obj);
    void stop();
};
//Q_DECLARE_TYPEINFO(QBasicTimer, Q_MOVABLE_TYPE);



#endif // QBASICTIMER_H

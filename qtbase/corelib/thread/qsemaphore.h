#ifndef QSEMAPHORE_H
#define QSEMAPHORE_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QSemaphorePrivate;

class QSemaphore
{
private:
    //Q_DISABLE_COPY(QSemaphore)

    QSemaphorePrivate *d;

public:
    explicit QSemaphore(int n = 0);
    ~QSemaphore();

    void acquire(int n = 1);
    bool tryAcquire(int n = 1);
    bool tryAcquire(int n, int timeout);

    void release(int n = 1);

    int available() const;


};


QT_END_NAMESPACE

#endif // QSEMAPHORE_H

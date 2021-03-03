#ifndef QMUTEX_H
#define QMUTEX_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <new>

#if QT_HAS_INCLUDE(<chrono>)
#  include <chrono>
#  include <limits>
#endif

class tst_QMutex;

QT_BEGIN_NAMESPACE

class QMutexData;

// oye basic: [Lock TLock, Recur]
class  QBasicMutex
{
public:
    // BasicLockable concept
    inline void lock()  {
        if (!fastTryLock())
            lockInternal();
    }

    inline void unlock()  {
        Q_ASSERT(d_ptr.load()); //mutex must be locked
        if (!fastTryUnlock())
            unlockInternal();
    }

    bool tryLock()  {
        return fastTryLock();
    }

    // Lockable concept
    bool try_lock()  { return tryLock(); }

    bool isRecursive() const ;

private:
    inline bool fastTryLock()  {
        return d_ptr.testAndSetAcquire(Q_NULLPTR, dummyLocked());
    }
    inline bool fastTryUnlock()  {
        return d_ptr.testAndSetRelease(dummyLocked(), Q_NULLPTR);
    }
    inline bool fastTryLock(QMutexData *&current)  {
        return d_ptr.testAndSetAcquire(Q_NULLPTR, dummyLocked(), current);
    }
    inline bool fastTryUnlock(QMutexData *&current)  {
        return d_ptr.testAndSetRelease(dummyLocked(), Q_NULLPTR, current);
    }

    void lockInternal() ;
    bool lockInternal(int timeout) ;
    void unlockInternal() ;

    QBasicAtomicPointer<QMutexData> d_ptr;
    static inline QMutexData *dummyLocked() {
        return reinterpret_cast<QMutexData *>(quintptr(1));
    }

    friend class QMutex;
    friend class QMutexData;
};

class QMutex : public QBasicMutex
{
public:
    enum RecursionMode { NonRecursive, Recursive };
    explicit QMutex(RecursionMode mode = NonRecursive);
    ~QMutex();

    // BasicLockable concept
    void lock() ;
    bool tryLock(int timeout = 0) ;
    // BasicLockable concept
    void unlock() ;

    // Lockable concept
    bool try_lock()  { return tryLock(); }

#if QT_HAS_INCLUDE(<chrono>)
    // TimedLockable concept
    template <class Rep, class Period>
    bool try_lock_for(std::chrono::duration<Rep, Period> duration)
    {
        return tryLock(convertToMilliseconds(duration));
    }

    // TimedLockable concept
    template<class Clock, class Duration>
    bool try_lock_until(std::chrono::time_point<Clock, Duration> timePoint)
    {
        // Implemented in terms of try_lock_for to honor the similar
        // requirement in N4606 § 30.4.1.3 [thread.timedmutex.requirements]/12.

        return try_lock_for(timePoint - Clock::now());
    }
#endif

    bool isRecursive() const 
    { return QBasicMutex::isRecursive(); }

private:
    Q_DISABLE_COPY(QMutex)
    friend class QMutexLocker;
    friend class ::tst_QMutex;

#if QT_HAS_INCLUDE(<chrono>)
    template<class Rep, class Period>
    static int convertToMilliseconds(std::chrono::duration<Rep, Period> duration)
    {
        // N4606 § 30.4.1.3.5 [thread.timedmutex.requirements] specifies that a
        // duration less than or equal to duration.zero() shall result in a
        // try_lock, unlike QMutex's tryLock with a negative duration which
        // results in a lock.

        if (duration <= duration.zero())
            return 0;

        // when converting from 'duration' to milliseconds, make sure that
        // the result is not shorter than 'duration':
        std::chrono::milliseconds wait = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        if (wait < duration)
            wait += std::chrono::milliseconds(1);
        Q_ASSERT(wait >= duration);
        const auto ms = wait.count();
        const auto maxInt = (std::numeric_limits<int>::max)();

        return ms < maxInt ? int(ms) : maxInt;
    }
#endif
};

class  QMutexLocker
{
public:
    inline explicit QMutexLocker(QBasicMutex *m) 
    {
        Q_ASSERT_X((reinterpret_cast<quintptr>(m) & quintptr(1u)) == quintptr(0),
                   "QMutexLocker", "QMutex pointer is misaligned");
        val = quintptr(m);
        if (Q_LIKELY(m)) {
            // call QMutex::lock() instead of QBasicMutex::lock()
            static_cast<QMutex *>(m)->lock();
            val |= 1;
        }
    }

    inline ~QMutexLocker() { unlock(); }

    inline void unlock() 
    {
        if ((val & quintptr(1u)) == quintptr(1u)) {
            val &= ~quintptr(1u);
            mutex()->unlock();
        }
    }

    inline void relock() 
    {
        if (val) {
            if ((val & quintptr(1u)) == quintptr(0u)) {
                mutex()->lock();
                val |= quintptr(1u);
            }
        }
    }

    inline QMutex *mutex() const
    {
        return reinterpret_cast<QMutex *>(val & ~quintptr(1u));
    }


private:
    Q_DISABLE_COPY(QMutexLocker)

    quintptr val;
};


QT_END_NAMESPACE

#endif // QMUTEX_H

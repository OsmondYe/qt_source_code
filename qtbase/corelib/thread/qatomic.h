#include <QtCore/qglobal.h>

#ifndef QATOMIC_H
#define QATOMIC_H

#include <QtCore/qbasicatomic.h>

QT_BEGIN_NAMESPACE


// High-level atomic integer operations
template <typename T>
class QAtomicInteger : public QBasicAtomicInteger<T>
{
public:
    // Non-atomic API
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr QAtomicInteger(T value = 0)  : QBasicAtomicInteger<T>(value) {}
#else
    inline QAtomicInteger(T value = 0) 
    {
        this->_q_value = value;
    }
#endif

    inline QAtomicInteger(const QAtomicInteger &other) 
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
        : QBasicAtomicInteger<T>()
#endif
    {
        this->storeRelease(other.loadAcquire());
    }

    inline QAtomicInteger &operator=(const QAtomicInteger &other) 
    {
        this->storeRelease(other.loadAcquire());
        return *this;
    }

#ifdef Q_QDOC
    T load() const;
    T loadAcquire() const;
    void store(T newValue);
    void storeRelease(T newValue);

    operator T() const;
    QAtomicInteger &operator=(T);

    static  bool isReferenceCountingNative();
    static  bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static  bool isTestAndSetNative();
    static  bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T expectedValue, T newValue);
    bool testAndSetAcquire(T expectedValue, T newValue);
    bool testAndSetRelease(T expectedValue, T newValue);
    bool testAndSetOrdered(T expectedValue, T newValue);

    static  bool isFetchAndStoreNative();
    static  bool isFetchAndStoreWaitFree();

    T fetchAndStoreRelaxed(T newValue);
    T fetchAndStoreAcquire(T newValue);
    T fetchAndStoreRelease(T newValue);
    T fetchAndStoreOrdered(T newValue);

    static  bool isFetchAndAddNative();
    static  bool isFetchAndAddWaitFree();

    T fetchAndAddRelaxed(T valueToAdd);
    T fetchAndAddAcquire(T valueToAdd);
    T fetchAndAddRelease(T valueToAdd);
    T fetchAndAddOrdered(T valueToAdd);

    T fetchAndSubRelaxed(T valueToSub);
    T fetchAndSubAcquire(T valueToSub);
    T fetchAndSubRelease(T valueToSub);
    T fetchAndSubOrdered(T valueToSub);

    T fetchAndOrRelaxed(T valueToOr);
    T fetchAndOrAcquire(T valueToOr);
    T fetchAndOrRelease(T valueToOr);
    T fetchAndOrOrdered(T valueToOr);

    T fetchAndAndRelaxed(T valueToAnd);
    T fetchAndAndAcquire(T valueToAnd);
    T fetchAndAndRelease(T valueToAnd);
    T fetchAndAndOrdered(T valueToAnd);

    T fetchAndXorRelaxed(T valueToXor);
    T fetchAndXorAcquire(T valueToXor);
    T fetchAndXorRelease(T valueToXor);
    T fetchAndXorOrdered(T valueToXor);

    T operator++();
    T operator++(int);
    T operator--();
    T operator--(int);
    T operator+=(T value);
    T operator-=(T value);
    T operator|=(T value);
    T operator&=(T value);
    T operator^=(T value);
#endif
};


class QAtomicInt : public QAtomicInteger<int>
{
public:

    QAtomicInt(int value = 0)  : QAtomicInteger<int>(value) {}
};

// High-level atomic pointer operations
template <typename T>
class QAtomicPointer : public QBasicAtomicPointer<T>
{
public:
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr QAtomicPointer(T *value = 0)  : QBasicAtomicPointer<T>(value) {}
#else
    inline QAtomicPointer(T *value = 0) 
    {
        this->store(value);
    }
#endif
    inline QAtomicPointer(const QAtomicPointer<T> &other) 
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
        : QBasicAtomicPointer<T>()
#endif
    {
        this->storeRelease(other.loadAcquire());
    }

    inline QAtomicPointer<T> &operator=(const QAtomicPointer<T> &other) 
    {
        this->storeRelease(other.loadAcquire());
        return *this;
    }

#ifdef Q_QDOC
    T *load() const;
    T *loadAcquire() const;
    void store(T *newValue);
    void storeRelease(T *newValue);

    static  bool isTestAndSetNative();
    static  bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static  bool isFetchAndStoreNative();
    static  bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static  bool isFetchAndAddNative();
    static  bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
#endif
};



template <typename T>
inline void qAtomicAssign(T *&d, T *x)
{
    if (d == x)
        return;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}


template <typename T>
inline void qAtomicDetach(T *&d)
{
    if (d->ref.load() == 1)
        return;
    T *x = d;
    d = new T(*d);
    if (!x->ref.deref())
        delete x;
}

QT_END_NAMESPACE
#endif // QATOMIC_H

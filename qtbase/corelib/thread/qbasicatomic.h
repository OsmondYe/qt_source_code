#include <QtCore/qatomic.h>

#ifndef QBASICATOMIC_H
#define QBASICATOMIC_H


#include <QtCore/qatomic_bootstrap.h>


#include <QtCore/qatomic_cxx11.h>


#include <QtCore/qatomic_msvc.h>



QT_BEGIN_NAMESPACE

// New atomics

template <typename T>
class QBasicAtomicInteger
{
public:
    typedef QAtomicOps<T> Ops;
    // static check that this is a valid integer
    Q_STATIC_ASSERT_X(QTypeInfo<T>::isIntegral, "template parameter is not an integral type");
    Q_STATIC_ASSERT_X(QAtomicOpsSupport<sizeof(T)>::IsSupported, "template parameter is an integral of a size not supported on this platform");

    typename Ops::Type _q_value;

    // Everything below is either implemented in ../arch/qatomic_XXX.h or (as fallback) in qgenericatomic.h

    T load() const  { return Ops::load(_q_value); }
    void store(T newValue)  { Ops::store(_q_value, newValue); }

    T loadAcquire() const  { return Ops::loadAcquire(_q_value); }
    void storeRelease(T newValue)  { Ops::storeRelease(_q_value, newValue); }
    operator T() const  { return loadAcquire(); }
    T operator=(T newValue)  { storeRelease(newValue); return newValue; }

    static Q_DECL_CONSTEXPR bool isReferenceCountingNative()  { return Ops::isReferenceCountingNative(); }
    static Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree()  { return Ops::isReferenceCountingWaitFree(); }

    bool ref()  { return Ops::ref(_q_value); }
    bool deref()  { return Ops::deref(_q_value); }

    static Q_DECL_CONSTEXPR bool isTestAndSetNative()  { return Ops::isTestAndSetNative(); }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree()  { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(T expectedValue, T newValue) 
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(T expectedValue, T newValue) 
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(T expectedValue, T newValue) 
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(T expectedValue, T newValue) 
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    bool testAndSetRelaxed(T expectedValue, T newValue, T &currentValue) 
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(T expectedValue, T newValue, T &currentValue) 
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(T expectedValue, T newValue, T &currentValue) 
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(T expectedValue, T newValue, T &currentValue) 
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue, &currentValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative()  { return Ops::isFetchAndStoreNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree()  { return Ops::isFetchAndStoreWaitFree(); }

    T fetchAndStoreRelaxed(T newValue) 
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    T fetchAndStoreAcquire(T newValue) 
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    T fetchAndStoreRelease(T newValue) 
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    T fetchAndStoreOrdered(T newValue) 
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative()  { return Ops::isFetchAndAddNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree()  { return Ops::isFetchAndAddWaitFree(); }

    T fetchAndAddRelaxed(T valueToAdd) 
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    T fetchAndAddAcquire(T valueToAdd) 
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    T fetchAndAddRelease(T valueToAdd) 
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    T fetchAndAddOrdered(T valueToAdd) 
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

    T fetchAndSubRelaxed(T valueToAdd) 
    { return Ops::fetchAndSubRelaxed(_q_value, valueToAdd); }
    T fetchAndSubAcquire(T valueToAdd) 
    { return Ops::fetchAndSubAcquire(_q_value, valueToAdd); }
    T fetchAndSubRelease(T valueToAdd) 
    { return Ops::fetchAndSubRelease(_q_value, valueToAdd); }
    T fetchAndSubOrdered(T valueToAdd) 
    { return Ops::fetchAndSubOrdered(_q_value, valueToAdd); }

    T fetchAndAndRelaxed(T valueToAdd) 
    { return Ops::fetchAndAndRelaxed(_q_value, valueToAdd); }
    T fetchAndAndAcquire(T valueToAdd) 
    { return Ops::fetchAndAndAcquire(_q_value, valueToAdd); }
    T fetchAndAndRelease(T valueToAdd) 
    { return Ops::fetchAndAndRelease(_q_value, valueToAdd); }
    T fetchAndAndOrdered(T valueToAdd) 
    { return Ops::fetchAndAndOrdered(_q_value, valueToAdd); }

    T fetchAndOrRelaxed(T valueToAdd) 
    { return Ops::fetchAndOrRelaxed(_q_value, valueToAdd); }
    T fetchAndOrAcquire(T valueToAdd) 
    { return Ops::fetchAndOrAcquire(_q_value, valueToAdd); }
    T fetchAndOrRelease(T valueToAdd) 
    { return Ops::fetchAndOrRelease(_q_value, valueToAdd); }
    T fetchAndOrOrdered(T valueToAdd) 
    { return Ops::fetchAndOrOrdered(_q_value, valueToAdd); }

    T fetchAndXorRelaxed(T valueToAdd) 
    { return Ops::fetchAndXorRelaxed(_q_value, valueToAdd); }
    T fetchAndXorAcquire(T valueToAdd) 
    { return Ops::fetchAndXorAcquire(_q_value, valueToAdd); }
    T fetchAndXorRelease(T valueToAdd) 
    { return Ops::fetchAndXorRelease(_q_value, valueToAdd); }
    T fetchAndXorOrdered(T valueToAdd) 
    { return Ops::fetchAndXorOrdered(_q_value, valueToAdd); }

    T operator++() 
    { return fetchAndAddOrdered(1) + 1; }
    T operator++(int) 
    { return fetchAndAddOrdered(1); }
    T operator--() 
    { return fetchAndSubOrdered(1) - 1; }
    T operator--(int) 
    { return fetchAndSubOrdered(1); }

    T operator+=(T v) 
    { return fetchAndAddOrdered(v) + v; }
    T operator-=(T v) 
    { return fetchAndSubOrdered(v) - v; }
    T operator&=(T v) 
    { return fetchAndAndOrdered(v) & v; }
    T operator|=(T v) 
    { return fetchAndOrOrdered(v) | v; }
    T operator^=(T v) 
    { return fetchAndXorOrdered(v) ^ v; }


#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    QBasicAtomicInteger() = default;
    constexpr QBasicAtomicInteger(T value)  : _q_value(value) {}
    QBasicAtomicInteger(const QBasicAtomicInteger &) = delete;
    QBasicAtomicInteger &operator=(const QBasicAtomicInteger &) = delete;
    QBasicAtomicInteger &operator=(const QBasicAtomicInteger &) volatile = delete;
#endif
};
typedef QBasicAtomicInteger<int> QBasicAtomicInt;

template <typename X>
class QBasicAtomicPointer
{
public:
    typedef X *Type;
    typedef QAtomicOps<Type> Ops;
    typedef typename Ops::Type AtomicType;

    AtomicType _q_value;

    Type load() const  { return Ops::load(_q_value); }
    void store(Type newValue)  { Ops::store(_q_value, newValue); }
    operator Type() const  { return loadAcquire(); }
    Type operator=(Type newValue)  { storeRelease(newValue); return newValue; }

    // Atomic API, implemented in qatomic_XXX.h
    Type loadAcquire() const  { return Ops::loadAcquire(_q_value); }
    void storeRelease(Type newValue)  { Ops::storeRelease(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isTestAndSetNative()  { return Ops::isTestAndSetNative(); }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree()  { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(Type expectedValue, Type newValue) 
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(Type expectedValue, Type newValue) 
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(Type expectedValue, Type newValue) 
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(Type expectedValue, Type newValue) 
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    bool testAndSetRelaxed(Type expectedValue, Type newValue, Type &currentValue) 
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetAcquire(Type expectedValue, Type newValue, Type &currentValue) 
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetRelease(Type expectedValue, Type newValue, Type &currentValue) 
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue, &currentValue); }
    bool testAndSetOrdered(Type expectedValue, Type newValue, Type &currentValue) 
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue, &currentValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative()  { return Ops::isFetchAndStoreNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree()  { return Ops::isFetchAndStoreWaitFree(); }

    Type fetchAndStoreRelaxed(Type newValue) 
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    Type fetchAndStoreAcquire(Type newValue) 
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    Type fetchAndStoreRelease(Type newValue) 
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    Type fetchAndStoreOrdered(Type newValue) 
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative()  { return Ops::isFetchAndAddNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree()  { return Ops::isFetchAndAddWaitFree(); }

    Type fetchAndAddRelaxed(qptrdiff valueToAdd) 
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    Type fetchAndAddAcquire(qptrdiff valueToAdd) 
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    Type fetchAndAddRelease(qptrdiff valueToAdd) 
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    Type fetchAndAddOrdered(qptrdiff valueToAdd) 
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

    Type fetchAndSubRelaxed(qptrdiff valueToAdd) 
    { return Ops::fetchAndSubRelaxed(_q_value, valueToAdd); }
    Type fetchAndSubAcquire(qptrdiff valueToAdd) 
    { return Ops::fetchAndSubAcquire(_q_value, valueToAdd); }
    Type fetchAndSubRelease(qptrdiff valueToAdd) 
    { return Ops::fetchAndSubRelease(_q_value, valueToAdd); }
    Type fetchAndSubOrdered(qptrdiff valueToAdd) 
    { return Ops::fetchAndSubOrdered(_q_value, valueToAdd); }

    Type operator++() 
    { return fetchAndAddOrdered(1) + 1; }
    Type operator++(int) 
    { return fetchAndAddOrdered(1); }
    Type operator--() 
    { return fetchAndSubOrdered(1) - 1; }
    Type operator--(int) 
    { return fetchAndSubOrdered(1); }
    Type operator+=(qptrdiff valueToAdd) 
    { return fetchAndAddOrdered(valueToAdd) + valueToAdd; }
    Type operator-=(qptrdiff valueToSub) 
    { return fetchAndSubOrdered(valueToSub) - valueToSub; }

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    QBasicAtomicPointer() = default;
    constexpr QBasicAtomicPointer(Type value)  : _q_value(value) {}
    QBasicAtomicPointer(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) volatile = delete;
#endif
};

#ifndef Q_BASIC_ATOMIC_INITIALIZER
#  define Q_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

QT_END_NAMESPACE



#endif // QBASICATOMIC_H

#ifndef QGENERICATOMIC_H
#define QGENERICATOMIC_H

#include <QtCore/qglobal.h>
#include <QtCore/qtypeinfo.h>

QT_BEGIN_NAMESPACE

template<int> struct QAtomicOpsSupport { enum { IsSupported = 0 }; };
template<> struct QAtomicOpsSupport<4> { enum { IsSupported = 1 }; };

template <typename T> struct QAtomicAdditiveType
{
    typedef T AdditiveT;
    static const int AddScale = 1;
};
template <typename T> struct QAtomicAdditiveType<T *>
{
    typedef qptrdiff AdditiveT;
    static const int AddScale = sizeof(T);
};

// not really atomic...
template <typename BaseClass> struct QGenericAtomicOps
{
    template <typename T> struct AtomicType { typedef T Type; typedef T *PointerType; };

    template <typename T> static void acquireMemoryFence(const T &_q_value) 
    {
        BaseClass::orderedMemoryFence(_q_value);
    }
    template <typename T> static void releaseMemoryFence(const T &_q_value) 
    {
        BaseClass::orderedMemoryFence(_q_value);
    }
    template <typename T> static void orderedMemoryFence(const T &) 
    {
    }

    template <typename T> static inline
    T load(const T &_q_value) 
    {
        return _q_value;
    }

    template <typename T, typename X> static inline
    void store(T &_q_value, X newValue) 
    {
        _q_value = newValue;
    }

    template <typename T> static inline
    T loadAcquire(const T &_q_value) 
    {
        T tmp = *static_cast<const volatile T *>(&_q_value);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T, typename X> static inline
    void storeRelease(T &_q_value, X newValue) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        *static_cast<volatile T *>(&_q_value) = newValue;
    }

    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() 
    { return BaseClass::isFetchAndAddNative(); }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() 
    { return BaseClass::isFetchAndAddWaitFree(); }
    template <typename T> static inline
    bool ref(T &_q_value) 
    {
        return BaseClass::fetchAndAddRelaxed(_q_value, 1) != T(-1);
    }

    template <typename T> static inline
    bool deref(T &_q_value) 
    {
         return BaseClass::fetchAndAddRelaxed(_q_value, -1) != 1;
    }


    template <typename T, typename X> static inline
    bool testAndSetAcquire(T &_q_value, X expectedValue, X newValue) 
    {
        bool tmp = BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T, typename X> static inline
    bool testAndSetRelease(T &_q_value, X expectedValue, X newValue) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue);
    }

    template <typename T, typename X> static inline
    bool testAndSetOrdered(T &_q_value, X expectedValue, X newValue) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue);
    }

    template <typename T, typename X> static inline
    bool testAndSetAcquire(T &_q_value, X expectedValue, X newValue, X *currentValue) 
    {
        bool tmp = BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue, currentValue);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T, typename X> static inline
    bool testAndSetRelease(T &_q_value, X expectedValue, X newValue, X *currentValue) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue, currentValue);
    }

    template <typename T, typename X> static inline
    bool testAndSetOrdered(T &_q_value, X expectedValue, X newValue, X *currentValue) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::testAndSetRelaxed(_q_value, expectedValue, newValue, currentValue);
    }

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative()  { return false; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree()  { return false; }

    template <typename T, typename X> static inline
    T fetchAndStoreRelaxed(T &_q_value, X newValue) 
    {
        // implement fetchAndStore on top of testAndSet
        Q_FOREVER {
            T tmp = load(_q_value);
            if (BaseClass::testAndSetRelaxed(_q_value, tmp, newValue))
                return tmp;
        }
    }

    template <typename T, typename X> static inline
    T fetchAndStoreAcquire(T &_q_value, X newValue) 
    {
        T tmp = BaseClass::fetchAndStoreRelaxed(_q_value, newValue);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T, typename X> static inline
    T fetchAndStoreRelease(T &_q_value, X newValue) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndStoreRelaxed(_q_value, newValue);
    }

    template <typename T, typename X> static inline
    T fetchAndStoreOrdered(T &_q_value, X newValue) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndStoreRelaxed(_q_value, newValue);
    }

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative()  { return false; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree()  { return false; }
    template <typename T> static inline
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) 
    {
        // implement fetchAndAdd on top of testAndSet
        Q_FOREVER {
            T tmp = BaseClass::load(_q_value);
            if (BaseClass::testAndSetRelaxed(_q_value, tmp, T(tmp + valueToAdd)))
                return tmp;
        }
    }

    template <typename T> static inline
    T fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) 
    {
        T tmp = BaseClass::fetchAndAddRelaxed(_q_value, valueToAdd);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T> static inline
    T fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndAddRelaxed(_q_value, valueToAdd);
    }

    template <typename T> static inline
    T fetchAndAddOrdered(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndAddRelaxed(_q_value, valueToAdd);
    }

    template <typename T> static inline
    T fetchAndSubRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT operand) 
    {
        // implement fetchAndSub on top of fetchAndAdd
        return fetchAndAddRelaxed(_q_value, -operand);
    }

    template <typename T> static inline
    T fetchAndSubAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT operand) 
    {
        T tmp = BaseClass::fetchAndSubRelaxed(_q_value, operand);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T> static inline
    T fetchAndSubRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT operand) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndSubRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndSubOrdered(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT operand) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndSubRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndAndRelaxed(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        // implement fetchAndAnd on top of testAndSet
        T tmp = BaseClass::load(_q_value);
        Q_FOREVER {
            if (BaseClass::testAndSetRelaxed(_q_value, tmp, T(tmp & operand), &tmp))
                return tmp;
        }
    }

    template <typename T> static inline
    T fetchAndAndAcquire(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        T tmp = BaseClass::fetchAndAndRelaxed(_q_value, operand);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T> static inline
    T fetchAndAndRelease(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndAndRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndAndOrdered(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndAndRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndOrRelaxed(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        // implement fetchAndOr on top of testAndSet
        T tmp = BaseClass::load(_q_value);
        Q_FOREVER {
            if (BaseClass::testAndSetRelaxed(_q_value, tmp, T(tmp | operand), &tmp))
                return tmp;
        }
    }

    template <typename T> static inline
    T fetchAndOrAcquire(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        T tmp = BaseClass::fetchAndOrRelaxed(_q_value, operand);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T> static inline
    T fetchAndOrRelease(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndOrRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndOrOrdered(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndOrRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndXorRelaxed(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        // implement fetchAndXor on top of testAndSet
        T tmp = BaseClass::load(_q_value);
        Q_FOREVER {
            if (BaseClass::testAndSetRelaxed(_q_value, tmp, T(tmp ^ operand), &tmp))
                return tmp;
        }
    }

    template <typename T> static inline
    T fetchAndXorAcquire(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        T tmp = BaseClass::fetchAndXorRelaxed(_q_value, operand);
        BaseClass::acquireMemoryFence(_q_value);
        return tmp;
    }

    template <typename T> static inline
    T fetchAndXorRelease(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::releaseMemoryFence(_q_value);
        return BaseClass::fetchAndXorRelaxed(_q_value, operand);
    }

    template <typename T> static inline
    T fetchAndXorOrdered(T &_q_value, typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type operand) 
    {
        BaseClass::orderedMemoryFence(_q_value);
        return BaseClass::fetchAndXorRelaxed(_q_value, operand);
    }
};

QT_END_NAMESPACE
#endif // QGENERICATOMIC_H

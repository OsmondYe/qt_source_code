#ifndef QATOMIC_BOOTSTRAP_H
#define QATOMIC_BOOTSTRAP_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE


template <typename T> struct QAtomicOps: QGenericAtomicOps<QAtomicOps<T> >
{
    typedef T Type;

    static bool ref(T &_q_value) 
    {
        return ++_q_value != 0;
    }
    static bool deref(T &_q_value) 
    {
        return --_q_value != 0;
    }

    static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) 
    {
        if (currentValue)
            *currentValue = _q_value;
        if (_q_value == expectedValue) {
            _q_value = newValue;
            return true;
        }
        return false;
    }

    static T fetchAndStoreRelaxed(T &_q_value, T newValue) 
    {
        T tmp = _q_value;
        _q_value = newValue;
        return tmp;
    }

    template <typename AdditiveType> static
    T fetchAndAddRelaxed(T &_q_value, AdditiveType valueToAdd) 
    {
        T returnValue = _q_value;
        _q_value += valueToAdd;
        return returnValue;
    }
};

QT_END_NAMESPACE

#endif // QATOMIC_BOOTSTRAP_H

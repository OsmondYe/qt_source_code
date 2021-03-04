#include <QtCore/qglobal.h>

#ifndef QGLOBALSTATIC_H
#define QGLOBALSTATIC_H

#include <QtCore/qatomic.h>



namespace QtGlobalStatic {

	enum GuardValues {
	    Destroyed = -2,
	    Initialized = -1,
	    Uninitialized = 0,
	    Initializing = 1
	};
}


#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

#define Q_GLOBAL_STATIC_INTERNAL(ARGS)                                  \
    Q_DECL_HIDDEN inline Type *innerFunction()                          \
    {                                                                   \
        static Type *d;                                                 \
        static QBasicMutex mutex;                                       \
        int x = guard.loadAcquire();                                    \
        if (Q_UNLIKELY(x >= QtGlobalStatic::Uninitialized)) {           \
            QMutexLocker locker(&mutex);                                \
            if (guard.load() == QtGlobalStatic::Uninitialized) {        \
                d = new Type ARGS;                                      \
                static struct Cleanup {                                 \
                    ~Cleanup() {                                        \
                        delete d;                                       \
                        guard.store(QtGlobalStatic::Destroyed);         \
                    }                                                   \
                } cleanup;                                              \
                guard.storeRelease(QtGlobalStatic::Initialized);        \
            }                                                           \
        }                                                               \
        return d;                                                       \
    }


// this class must be POD, unless the compiler supports thread-safe statics
template <typename T, T *(&innerFunction)(), QBasicAtomicInt &guard>
struct QGlobalStatic
{
    typedef T Type;

    bool isDestroyed() const { return guard.load() <= QtGlobalStatic::Destroyed; }
    bool exists() const { return guard.load() == QtGlobalStatic::Initialized; }
    operator Type *() { if (isDestroyed()) return 0; return innerFunction(); }
    Type *operator()() { if (isDestroyed()) return 0; return innerFunction(); }
    Type *operator->()
    {
      Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC", "The global static was used after being destroyed");
      return innerFunction();
    }
    Type &operator*()
    {
      Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC", "The global static was used after being destroyed");
      return *innerFunction();
    }
};

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                         \
    namespace { namespace Q_QGS_ ## NAME {                                  \
        typedef TYPE Type;                                                  \
        QBasicAtomicInt guard = Q_BASIC_ATOMIC_INITIALIZER(QtGlobalStatic::Uninitialized); \
        Q_GLOBAL_STATIC_INTERNAL(ARGS)                                      \
    } }                                                                     \
    static QGlobalStatic<TYPE,                                              \
                         Q_QGS_ ## NAME::innerFunction,                     \
                         Q_QGS_ ## NAME::guard> NAME;

#define Q_GLOBAL_STATIC(TYPE, NAME)                                         \
    Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ())

QT_END_NAMESPACE
#endif // QGLOBALSTATIC_H

#ifndef QSCOPEDPOINTER_H
#define QSCOPEDPOINTER_H

#include <QtCore/qglobal.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

template <typename T>
struct QScopedPointerDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete pointer;
    }
};

template <typename T>
struct QScopedPointerArrayDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete [] pointer;
    }
};

struct QScopedPointerPodDeleter
{
    static inline void cleanup(void *pointer) { if (pointer) free(pointer); }
};

#ifndef QT_NO_QOBJECT
template <typename T>
struct QScopedPointerObjectDeleteLater
{
    static inline void cleanup(T *pointer) { if (pointer) pointer->deleteLater(); }
};

class QObject;
typedef QScopedPointerObjectDeleteLater<QObject> QScopedPointerDeleteLater;
#endif

template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QScopedPointer
{
    typedef T *QScopedPointer:: *RestrictedBool;
public:
    explicit QScopedPointer(T *p = Q_NULLPTR) Q_DECL_NOTHROW : d(p)
    {
    }

    inline ~QScopedPointer()
    {
        T *oldD = this->d;
        Cleanup::cleanup(oldD);
    }

    inline T &operator*() const
    {
        return *d;
    }

    T *operator->() const Q_DECL_NOTHROW
    {
        return d;
    }

    bool operator!() const Q_DECL_NOTHROW
    {
        return !d;
    }

    inline operator bool() const
    {
        return isNull() ? Q_NULLPTR : &QScopedPointer::d;
    }


    T *data() const Q_DECL_NOTHROW
    {
        return d;
    }

    bool isNull() const Q_DECL_NOTHROW
    {
        return !d;
    }

    void reset(T *other = Q_NULLPTR) 
    {
        if (d == other)
            return;
        T *oldD = d;
        d = other;
        Cleanup::cleanup(oldD);
    }

    T *take() Q_DECL_NOTHROW
    {
        T *oldD = d;
        d = Q_NULLPTR;
        return oldD;
    }

    void swap(QScopedPointer<T, Cleanup> &other) Q_DECL_NOTHROW
    {
        qSwap(d, other.d);
    }

    typedef T *pointer;

protected:
    T *d;

private:
    Q_DISABLE_COPY(QScopedPointer)
};

template <class T, class Cleanup>
inline bool operator==(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs) Q_DECL_NOTHROW
{
    return lhs.data() == rhs.data();
}

template <class T, class Cleanup>
inline bool operator!=(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs) Q_DECL_NOTHROW
{
    return lhs.data() != rhs.data();
}

template <class T, class Cleanup>
inline bool operator==(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t) Q_DECL_NOTHROW
{
    return lhs.isNull();
}

template <class T, class Cleanup>
inline bool operator==(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs) Q_DECL_NOTHROW
{
    return rhs.isNull();
}

template <class T, class Cleanup>
inline bool operator!=(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t) Q_DECL_NOTHROW
{
    return !lhs.isNull();
}

template <class T, class Cleanup>
inline bool operator!=(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs) Q_DECL_NOTHROW
{
    return !rhs.isNull();
}

template <class T, class Cleanup>
inline void swap(QScopedPointer<T, Cleanup> &p1, QScopedPointer<T, Cleanup> &p2) Q_DECL_NOTHROW
{ p1.swap(p2); }


namespace QtPrivate {
    template <typename X, typename Y> struct QScopedArrayEnsureSameType;
    template <typename X> struct QScopedArrayEnsureSameType<X,X> { typedef X* Type; };
    template <typename X> struct QScopedArrayEnsureSameType<const X, X> { typedef X* Type; };
}

template <typename T, typename Cleanup = QScopedPointerArrayDeleter<T> >
class QScopedArrayPointer : public QScopedPointer<T, Cleanup>
{
public:
    inline QScopedArrayPointer() : QScopedPointer<T, Cleanup>(Q_NULLPTR) {}

    template <typename D>
    explicit inline QScopedArrayPointer(D *p, typename QtPrivate::QScopedArrayEnsureSameType<T,D>::Type = Q_NULLPTR)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T &operator[](int i)
    {
        return this->d[i];
    }

    inline const T &operator[](int i) const
    {
        return this->d[i];
    }

    void swap(QScopedArrayPointer &other) Q_DECL_NOTHROW // prevent QScopedPointer <->QScopedArrayPointer swaps
    { QScopedPointer<T, Cleanup>::swap(other); }

private:
    explicit inline QScopedArrayPointer(void *) {
        // Enforce the same type.

        // If you get a compile error here, make sure you declare
        // QScopedArrayPointer with the same template type as you pass to the
        // constructor. See also the QScopedPointer documentation.

        // Storing a scalar array as a pointer to a different type is not
        // allowed and results in undefined behavior.
    }

    Q_DISABLE_COPY(QScopedArrayPointer)
};

template <typename T, typename Cleanup>
inline void swap(QScopedArrayPointer<T, Cleanup> &lhs, QScopedArrayPointer<T, Cleanup> &rhs) Q_DECL_NOTHROW
{ lhs.swap(rhs); }

QT_END_NAMESPACE

#endif // QSCOPEDPOINTER_H

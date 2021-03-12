#ifndef QSHAREDDATA_H
#define QSHAREDDATA_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qhashfunctions.h>


template <class T> class QSharedDataPointer;

class  QSharedData
{
public:
    mutable QAtomicInt ref;

    inline QSharedData() : ref(0) { }
    inline QSharedData(const QSharedData &) : ref(0) { }

private:
    // using the assignment operator would lead to corruption in the ref-counting
    QSharedData &operator=(const QSharedData &);
};

template <class T> class QSharedDataPointer
{
	T *d;
public:
    typedef T Type;
    typedef T *pointer;
public:
    inline void detach() { if (d && d->ref.load() != 1) detach_helper(); }
    inline T &operator*() { detach(); return *d; }
    inline const T &operator*() const { return *d; }
    inline T *operator->() { detach(); return d; }
    inline const T *operator->() const { return d; }
    inline operator T *() { detach(); return d; }
    inline operator const T *() const { return d; }
    inline T *data() { detach(); return d; }
    inline const T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline bool operator==(const QSharedDataPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QSharedDataPointer<T> &other) const { return d != other.d; }
public:
    inline QSharedDataPointer() { d = Q_NULLPTR; }	
	explicit QSharedDataPointer(T *adata)  : d(adata){ if (d) d->ref.ref(); }
    inline QSharedDataPointer(const QSharedDataPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }
    inline ~QSharedDataPointer() { if (d && !d->ref.deref()) delete d; }
	
    inline QSharedDataPointer<T> & operator=(const QSharedDataPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QSharedDataPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline bool operator!() const { return !d; }

    inline void swap(QSharedDataPointer &other) 
    { qSwap(d, other.d); }

protected:
    T *clone(){    return new T(*d);}

private:
    void detach_helper(){
	    T *x = clone();
	    x->ref.ref();
	    if (!d->ref.deref())
	        delete d;
	    d = x;
	}

 
};

template <class T> class QExplicitlySharedDataPointer
{
	T *d;
public:
    typedef T Type;
    typedef T *pointer;

    inline T &operator*() const { return *d; }
    inline T *operator->() { return d; }
    inline T *operator->() const { return d; }
    inline T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline void detach() { if (d && d->ref.load() != 1) detach_helper(); }

    inline void reset()
    {
        if(d && !d->ref.deref())
            delete d;

        d = Q_NULLPTR;
    }

    inline operator bool () const { return d != Q_NULLPTR; }

    inline bool operator==(const QExplicitlySharedDataPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QExplicitlySharedDataPointer<T> &other) const { return d != other.d; }
    inline bool operator==(const T *ptr) const { return d == ptr; }
    inline bool operator!=(const T *ptr) const { return d != ptr; }

    inline QExplicitlySharedDataPointer() { d = Q_NULLPTR; }
    inline ~QExplicitlySharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit QExplicitlySharedDataPointer(T *data) Q_DECL_NOTHROW;
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }

    template<class X>
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X> &o)
        : d(static_cast<T *>(o.data()))
    {
        if(d)
            d->ref.ref();
    }

    inline QExplicitlySharedDataPointer<T> & operator=(const QExplicitlySharedDataPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QExplicitlySharedDataPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }

    inline bool operator!() const { return !d; }

    inline void swap(QExplicitlySharedDataPointer &other) 
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();


};



template <class T>
 T *QExplicitlySharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <class T>
 void QExplicitlySharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

template <class T>
 QExplicitlySharedDataPointer<T>::QExplicitlySharedDataPointer(T *adata) Q_DECL_NOTHROW
    : d(adata)
{ if (d) d->ref.ref(); }

template <class T>
 void qSwap(QSharedDataPointer<T> &p1, QSharedDataPointer<T> &p2)
{ p1.swap(p2); }

template <class T>
 void qSwap(QExplicitlySharedDataPointer<T> &p1, QExplicitlySharedDataPointer<T> &p2)
{ p1.swap(p2); }


namespace std {
    template <class T>
     void swap(QT_PREPEND_NAMESPACE(QSharedDataPointer)<T> &p1, QT_PREPEND_NAMESPACE(QSharedDataPointer)<T> &p2)
    { p1.swap(p2); }

    template <class T>
     void swap(QT_PREPEND_NAMESPACE(QExplicitlySharedDataPointer)<T> &p1, QT_PREPEND_NAMESPACE(QExplicitlySharedDataPointer)<T> &p2)
    { p1.swap(p2); }
}


template <class T>
 uint qHash(const QSharedDataPointer<T> &ptr, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(ptr.data(), seed);
}
template <class T>
 uint qHash(const QExplicitlySharedDataPointer<T> &ptr, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(ptr.data(), seed);
}

template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedDataPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QExplicitlySharedDataPointer<T>, Q_MOVABLE_TYPE);

#endif // QSHAREDDATA_H

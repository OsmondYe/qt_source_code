#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qtypeinfo.h>


class QVariant;

//oye  要求T是QObject一族的,  内包QWeakPointer,  不管引用计数, 不负责生命周期
template <class T>
class QPointer
{
    Q_STATIC_ASSERT_X(!std::is_pointer<T>::value, "QPointer's template type must not be a pointer type");
private:
    QWeakPointer<QObjectType> wp;  

    template<typename U>
    struct TypeSelector
    {
        typedef QObject Type;
    };
    template<typename U>
    struct TypeSelector<const U>	  // 再给一个const版本
    {
        typedef const QObject Type;
    };
    typedef typename TypeSelector<T>::Type QObjectType;

public:
    inline QPointer() { }
    inline QPointer(T *p) : wp(p, true) { }
    inline void swap(QPointer &other) { wp.swap(other.wp); }

    inline QPointer<T> &operator=(T* p)   { wp.assign(static_cast<QObjectType*>(p)); return *this; }

    inline T* data() const    { return static_cast<T*>( wp.data()); }
    inline T* operator->() const    { return data(); }
    inline T& operator*() const    { return *data(); }
    inline operator T*() const    { return data(); }

    inline bool isNull() const    { return wp.isNull(); }

    inline void clear()    { wp.clear(); }
};
//template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_MOVABLE_TYPE);

template <class T>
inline bool operator==(const T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const T *o)
{ return p.operator->() == o; }

template <class T>
inline bool operator==(T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, T *o)
{ return p.operator->() == o; }

template<class T>
inline bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() == p2.operator->(); }

template <class T>
inline bool operator!=(const T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const T *o)
{ return p.operator->() != o; }

template <class T>
inline bool operator!=(T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, T *o)
{ return p.operator->() != o; }

template<class T>
inline bool operator!= (const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() != p2.operator->() ; }

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    return QPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}

#endif // QPOINTER_H

QT_BEGIN_NAMESPACE


namespace QtPrivate {


template <typename ArgList> struct TypesAreDeclaredMetaType { enum { Value = false }; };

template <> struct TypesAreDeclaredMetaType<List<>> { enum { Value = true }; };

template <typename Arg, typename... Tail> struct TypesAreDeclaredMetaType<List<Arg, Tail...> >
{ enum { Value = QMetaTypeId2<Arg>::Defined && TypesAreDeclaredMetaType<List<Tail...>>::Value }; };


template <typename ArgList, bool Declared = TypesAreDeclaredMetaType<ArgList>::Value > struct ConnectionTypes
{ static const int *types() { return Q_NULLPTR; } };

template <> struct ConnectionTypes<List<>, true>
{ static const int *types() { return Q_NULLPTR; } };

template <typename... Args> struct ConnectionTypes<List<Args...>, true>
{ static const int *types() { static const int t[sizeof...(Args) + 1] = { (QtPrivate::QMetaTypeIdHelper<Args>::qt_metatype_id())..., 0 }; return t; } };


// internal base class (interface) containing functions required to call a slot managed by a pointer to function.
class QSlotObjectBase {
    QAtomicInt m_ref;
    // don't use virtual functions here; we don't want the
    // compiler to create tons of per-polymorphic-class stuff that
    // we'll never need. We just use one function pointer.
    typedef void (*ImplFn)(int which, QSlotObjectBase* this_, QObject *receiver, void **args, bool *ret);
    const ImplFn m_impl;
protected:
    enum Operation {
        Destroy,
        Call,
        Compare,

        NumOperations
    };
public:
    explicit QSlotObjectBase(ImplFn fn) : m_ref(1), m_impl(fn) {}

    inline int ref()  { return m_ref.ref(); }
    inline void destroyIfLastRef() Q_DECL_NOTHROW
    { if (!m_ref.deref()) m_impl(Destroy, this, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR); }

    inline bool compare(void **a) { bool ret = false; m_impl(Compare, this, Q_NULLPTR, a, &ret); return ret; }
    inline void call(QObject *r, void **a)  { m_impl(Call,    this, r, a, Q_NULLPTR); }
protected:
    ~QSlotObjectBase() {}
private:
    //Q_DISABLE_COPY(QSlotObjectBase)
};
	
// implementation of QSlotObjectBase for which the slot is a pointer to member function of a QObject
// Args and R are the List of arguments and the returntype of the signal to which the slot is connected.
template<typename Func, typename Args, typename R> class QSlotObject : public QSlotObjectBase
{
    typedef QtPrivate::FunctionPointer<Func> FuncType;
    Func function;
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
    {
        switch (which) {
        case Destroy:
            delete static_cast<QSlotObject*>(this_);
            break;
        case Call:
            FuncType::template call<Args, R>(static_cast<QSlotObject*>(this_)->function, static_cast<typename FuncType::Object *>(r), a);
            break;
        case Compare:
            *ret = *reinterpret_cast<Func *>(a) == static_cast<QSlotObject*>(this_)->function;
            break;
        case NumOperations: ;
        }
    }
public:
    explicit QSlotObject(Func f) : QSlotObjectBase(&impl), function(f) {}
};
// implementation of QSlotObjectBase for which the slot is a static function
// Args and R are the List of arguments and the returntype of the signal to which the slot is connected.
template<typename Func, typename Args, typename R> class QStaticSlotObject : public QSlotObjectBase
{
    typedef QtPrivate::FunctionPointer<Func> FuncType;
    Func function;
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
    {
        switch (which) {
        case Destroy:
            delete static_cast<QStaticSlotObject*>(this_);
            break;
        case Call:
            FuncType::template call<Args, R>(static_cast<QStaticSlotObject*>(this_)->function, r, a);
            break;
        case Compare:   // not implemented
        case NumOperations:
            Q_UNUSED(ret);
        }
    }
public:
    explicit QStaticSlotObject(Func f) : QSlotObjectBase(&impl), function(f) {}
};
	
// implementation of QSlotObjectBase for which the slot is a functor (or lambda)
// N is the number of arguments
// Args and R are the List of arguments and the returntype of the signal to which the slot is connected.
template<typename Func, int N, typename Args, typename R> 
class QFunctorSlotObject : public QSlotObjectBase
{
    typedef QtPrivate::Functor<Func, N> FuncType;
    Func function;
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
    {
        switch (which) {
        case Destroy:
            delete static_cast<QFunctorSlotObject*>(this_);
            break;
        case Call:
            FuncType::template call<Args, R>(static_cast<QFunctorSlotObject*>(this_)->function, r, a);
            break;
        case Compare: // not implemented
        case NumOperations:
            Q_UNUSED(ret);
        }
    }
public:
    explicit QFunctorSlotObject(const Func &f) : QSlotObjectBase(&impl), function(f) {}
};
	
} // end QtPrivate


QT_END_NAMESPACE


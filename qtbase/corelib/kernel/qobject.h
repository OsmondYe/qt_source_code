#ifndef QOBJECT_H
#define QOBJECT_H
#include <QtCore/qobjectdefs.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qobject_impl.h>
#include <chrono>


class QEvent;
class QTimerEvent;
class QChildEvent;
struct QMetaObject;
class QVariant;
class QObjectPrivate;
class QObject;
class QThread;
class QWidget;
class QRegExp;
class QRegularExpression;
class QObjectUserData;
struct QDynamicMetaObjectData;

typedef QList<QObject*> QObjectList;

 void qt_qFindChildren_helper(const QObject *parent, const QString &name,
                                           const QMetaObject &mo, QList<void *> *list, Qt::FindChildOptions options);
 void qt_qFindChildren_helper(const QObject *parent, const QRegExp &re,
                                           const QMetaObject &mo, QList<void *> *list, Qt::FindChildOptions options);
 void qt_qFindChildren_helper(const QObject *parent, const QRegularExpression &re,
                                           const QMetaObject &mo, QList<void *> *list, Qt::FindChildOptions options);
 QObject *qt_qFindChild_helper(const QObject *parent, const QString &name, const QMetaObject &mo, Qt::FindChildOptions options);

class  QObjectData   // Private Class的顶级数据类, QObjectPrivate也继承自此
{
public:
    virtual ~QObjectData(){}
	QMetaObject *dynamicMetaObject() const{return metaObject->toDynamicMetaObject(q_ptr);}
public:
    QObject *q_ptr;						// oye 所有Q_DECLARE_PUBLIC(QLayout)宏都会用到,指向外部实际Qtclass的指针
    // 存父和一级子节点
    QObject *parent;					// oye widget之类的树形布局中
    QObjectList children;  				// oye 用在widget上绝了, 控件内涵子控件

    uint isWidget : 1;					// Widget's PrivateClass will set it to 1
    uint blockSig : 1;					// 是否暂时阻止此Obj发送signal
    
    uint wasDeleted : 1;				// 智能指针节会用到,其内构造引用时,要求wasDeleted是false, ~QObject中设为true
    uint isDeletingChildren : 1;
    uint sendChildEvents : 1;			// oye, 关注下 SendEvent
    uint receiveChildEvents : 1;
    uint isWindow : 1; 					//for QWindow
    uint deleteLaterCalled : 1;
    uint unused : 24;
	
	//	++ in QCoreApplication::postEvent
	//  -- in QCoreApplicationPrivate::sendPostedEvents
    int postedEvents;                     	// oye  how many Events has been posted
    
    QDynamicMetaObjectData *metaObject;		// 静变动怎么实现?
};


/*
	oye
	- 先天的对象树思想, 需要有 parent() 方法  parent + children
	- event handle + 虚函数 子类自动分配机制
	- event filter, install, remove
	- signal slot
	- dynamic property
	- user defined data
	
*/
class  QObject      //QObjectPrivate
{
protected:
    QScopedPointer<QObjectData> d_ptr;   // 唯一的数据成员

	// Q_OBJECT
	
// OYE moc 将会处理Q_OBJECT 宏,生成 相对应的moc_QObject.cpp代码
// Q_OBJECT will be expanded as 
//----------------------------------------------------------
public:     
	//0ye : 经过moc后,这些信息会在另一个 moc_XXX.cpp文件中被写入
	//  QObject 其本身应该具有的在Meta方面的特征
	// [className, SuperMetaObject,User_extra, MetaCall[????? 重点关注]]
    static const QMetaObject staticMetaObject; 
    virtual const QMetaObject *metaObject() const; 	
	// 参考实现  return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
	   
    virtual void *qt_metacast(const char *); 	
    virtual int qt_metacall(QMetaObject::Call, int, void **); 
	
    static inline QString tr(const char *s, const char *c = Q_NULLPTR, int n = -1) 
        { return staticMetaObject.tr(s, c, n); } 
    static inline QString trUtf8(const char *s, const char *c = Q_NULLPTR, int n = -1) 
        { return staticMetaObject.tr(s, c, n); } 
private:      
    static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);      
    struct QPrivateSignal {}; 
    //QT_ANNOTATE_CLASS(qt_qobject, "")
//----------------------------------------------------------
// end enpandsion of the macro O_OBJECT
	friend inline const QMetaObject *qt_getQtMetaObject() { return &QObject::staticQtMetaObject; }	
    //Q_PROPERTY(QString objectName READ objectName WRITE setObjectName NOTIFY objectNameChanged)


public:
    explicit QObject(QObject *parent=Q_NULLPTR);
    virtual ~QObject();
	    //Q_DECLARE_PRIVATE(QObject)    
	inline QObjectPrivate* d_func() { return reinterpret_cast<QObjectPrivate *>(qGetPtrHelper(d_ptr)); } 	
Q_SIGNALS:
    void destroyed(QObject * = Q_NULLPTR);
    void objectNameChanged(const QString &objectName, QPrivateSignal);  //emit in call setObjectName
public Q_SLOTS:
    void deleteLater();		
public: // oye, widget overrides it to expand ui event
	//虚函数导致了,叶子类是最先被call到, 基类的event是最后,来做默认响应
    virtual bool event(QEvent *event);
    virtual bool eventFilter(QObject *watched, QEvent *event);
	// 给当前的Obj安装过滤,  比如 给PushButton安装Filter,Qt对象有qt功能的都必须继承自QObject
    void installEventFilter(QObject *filterObj);
    void removeEventFilter(QObject *obj);
protected:
	// 让派生类来保护继承, 扩展自己的feature,
	virtual void timerEvent(QTimerEvent *event){}
	virtual void childEvent(QChildEvent *event){}
	virtual void customEvent(QEvent *event){}

	virtual void connectNotify(const QMetaMethod &signal){	 Q_UNUSED(signal);}
	virtual void disconnectNotify(const QMetaMethod &signal){	Q_UNUSED(signal);}

public: 
    QString objectName() const;
    void setObjectName(const QString &name);

    inline bool isWidgetType() const { return d_ptr->isWidget; }
    inline bool isWindowType() const { return d_ptr->isWindow; }
public: 
    inline bool signalsBlocked() const  { return d_ptr->blockSig; }
    bool blockSignals(bool b) ;
	public: 
public:
    QThread *thread() const;
    void moveToThread(QThread *thread);
public: 
    int startTimer(int interval, Qt::TimerType timerType = Qt::CoarseTimer);    
    int startTimer(std::chrono::milliseconds time, Qt::TimerType timerType = Qt::CoarseTimer)    {
        return startTimer(int(time.count()), timerType);
    }
    void killTimer(int id);
public:
	bool setProperty(const char *name, const QVariant &value);
	QVariant property(const char *name) const;
	QList<QByteArray> dynamicPropertyNames() const;
public:
	static uint registerUserData();
	void setUserData(uint id, QObjectUserData* data);
	QObjectUserData* userData(uint id) const;
public:	
	void setParent(QObject *parent);
	inline QObject *parent() const { return d_ptr->parent; }
	inline const QObjectList &children() const { return d_ptr->children; }
	inline bool inherits(const char *classname) const
		{ return const_cast<QObject *>(this)->qt_metacast(classname) != Q_NULLPTR; }

public:
    template<typename T>
    inline T findChild(const QString &aName = QString(), Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        return static_cast<T>(qt_qFindChild_helper(this, aName, ObjType::staticMetaObject, options));
    }

    template<typename T>
    inline QList<T> findChildren(const QString &aName = QString(), Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        QList<T> list;
        qt_qFindChildren_helper(this, aName, ObjType::staticMetaObject,
                                reinterpret_cast<QList<void *> *>(&list), options);
        return list;
    }

    template<typename T>
    inline QList<T> findChildren(const QRegExp &re, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        QList<T> list;
        qt_qFindChildren_helper(this, re, ObjType::staticMetaObject,
                                reinterpret_cast<QList<void *> *>(&list), options);
        return list;
    }

    template<typename T>
    inline QList<T> findChildren(const QRegularExpression &re, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        QList<T> list;
        qt_qFindChildren_helper(this, re, ObjType::staticMetaObject,
                                reinterpret_cast<QList<void *> *>(&list), options);
        return list;
    }

    //Connect a signal to a pointer to qobject member function
    template <typename Func1, typename Func2>
    static inline 
    QMetaObject::Connection connect(
    		const typename QtPrivate::FunctionPointer<Func1>::Object *sender, 	Func1 signal,
            const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot,
            Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;

		// oye sender 必须是Q_OBJECT兼容的类类型
        Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

		// oye sender receiver 函数参数计数不对, 栈崩溃
        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                          "The slot requires more arguments than the signal provides.");

		// oye 参数不匹配				  
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");
		// 返回值类型不匹配
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");

        const int *types = Q_NULLPTR;
        if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
            types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

        return connectImpl(
        		sender, reinterpret_cast<void **>(&signal),
                receiver, reinterpret_cast<void **>(&slot),
               new QtPrivate::QSlotObject<
               				Func2, 
               				typename QtPrivate::List_Left<typename SignalType::Arguments, SlotType::ArgumentCount>::Value,
                            typename SignalType::ReturnType
                            >(slot),
                type, types, 
                // 直接拿sender所对应的Object中的staticMetaObject
                &SignalType::Object::staticMetaObject);
    }


	//oye  标配Lambda,或者其它函数指针,因为函数指针相对了Process的Module而言, 他装在后就是固定的
    //connect to a function pointer  (not a member)
    //
    template <typename Func1, typename Func2>
    static inline 
    typename 
    // Oye Func2函数具有的参数有意义, 强制传入的一定是一个函数指针
    // 这这样的情况下 此模板函数connect的返回值为 QMetaObject::Connection
    std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0, QMetaObject::Connection>::type
    
    connect(
    	// 从这里看 Func1必然是一个类的成员函数, 并且这里要拿到这个类的class
    	const typename QtPrivate::FunctionPointer<Func1>::Object *sender, 
    	// 从第一个参数的要求来看, 这里就必须写成类成员函数指针的形式
    	Func1 signal, 
    	Func2 slot)
    {
        return connect(sender, signal, sender, slot, Qt::DirectConnection);
    }


    //connect to a function pointer  (not a member)
    template <typename Func1, typename Func2>
    static inline typename 

	//oye 确保Func2有效, 同时是一般函数指针
    std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0 
    					&& !QtPrivate::FunctionPointer<Func2>::IsPointerToMemberFunction, 
                   QMetaObject::Connection
                   >::type
                   
    connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, 
    				Func1 signal, 
    				const QObject *context, Func2 slot,
                    Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;

        Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                          "The slot requires more arguments than the signal provides.");
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");

        const int *types = Q_NULLPTR;
        if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
            types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

        return connectImpl(sender, reinterpret_cast<void **>(&signal), context, Q_NULLPTR,
                           new QtPrivate::QStaticSlotObject<Func2,
                                                 typename QtPrivate::List_Left<typename SignalType::Arguments, SlotType::ArgumentCount>::Value,
                                                 typename SignalType::ReturnType>(slot),
                           type, types, &SignalType::Object::staticMetaObject);
    }

    //connect to a functor
    template <typename Func1, typename Func2>
    static inline typename 
    std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, 
    QMetaObject::Connection>::type

	connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
    {
        return connect(sender, signal, sender, slot, Qt::DirectConnection);
    }

    //connect to a functor, with a "context" object defining in which event loop is going to be executed
    template <typename Func1, typename Func2>
    static inline typename std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::type
            connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, const QObject *context, Func2 slot,
                    Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        const int FunctorArgumentCount = QtPrivate::ComputeFunctorArgumentCount<Func2 , typename SignalType::Arguments>::Value;

        Q_STATIC_ASSERT_X((FunctorArgumentCount >= 0),
                          "Signal and slot arguments are not compatible.");
        const int SlotArgumentCount = (FunctorArgumentCount >= 0) ? FunctorArgumentCount : 0;
        typedef typename QtPrivate::FunctorReturnType<Func2, typename QtPrivate::List_Left<typename SignalType::Arguments, SlotArgumentCount>::Value>::Value SlotReturnType;

        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<SlotReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");

        Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

        const int *types = Q_NULLPTR;
        if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
            types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

        return connectImpl(sender, reinterpret_cast<void **>(&signal), context, Q_NULLPTR,
                           new QtPrivate::QFunctorSlotObject<Func2, SlotArgumentCount,
                                typename QtPrivate::List_Left<typename SignalType::Arguments, SlotArgumentCount>::Value,
                                typename SignalType::ReturnType>(slot),
                           type, types, &SignalType::Object::staticMetaObject);
    }


    static bool disconnect(const QObject *sender, const char *signal,
                           const QObject *receiver, const char *member);
    static bool disconnect(const QObject *sender, const QMetaMethod &signal,
                           const QObject *receiver, const QMetaMethod &member);
    inline bool disconnect(const char *signal = Q_NULLPTR,
                           const QObject *receiver = Q_NULLPTR, const char *member = Q_NULLPTR) const
        { return disconnect(this, signal, receiver, member); }
    inline bool disconnect(const QObject *receiver, const char *member = Q_NULLPTR) const
        { return disconnect(this, Q_NULLPTR, receiver, member); }
    static bool disconnect(const QMetaObject::Connection &);

    template <typename Func1, typename Func2>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;

        Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");

        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, reinterpret_cast<void **>(&slot),
                              &SignalType::Object::staticMetaObject);
    }
    template <typename Func1>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const QObject *receiver, void **zero)
    {
        // This is the overload for when one wish to disconnect a signal from any slot. (slot=Q_NULLPTR)
        // Since the function template parameter cannot be deduced from '0', we use a
        // dummy void ** parameter that must be equal to 0
        Q_ASSERT(!zero);
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, zero,
                              &SignalType::Object::staticMetaObject);
    }
protected:
    QObject *sender() const;
    int senderSignalIndex() const;
    int receivers(const char* signal) const;
    bool isSignalConnected(const QMetaMethod &signal) const;



protected:
    QObject(QObjectPrivate &dd, QObject *parent = Q_NULLPTR);
private:
    // Q_DISABLE_COPY(QObject)
    // Q_PRIVATE_SLOT(d_func(), void _q_reregisterTimers(void *))

private:
    static QMetaObject::Connection connectImpl(const QObject *sender, void **signal,
                                               const QObject *receiver, void **slotPtr,
                                               QtPrivate::QSlotObjectBase *slot, Qt::ConnectionType type,
                                               const int *types, const QMetaObject *senderMetaObject);

    static bool disconnectImpl(const QObject *sender, void **signal, const QObject *receiver, void **slot,
                               const QMetaObject *senderMetaObject);
	
    friend struct QMetaObject;
    friend struct QMetaObjectPrivate;
    friend class QMetaCallEvent;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QCoreApplication;
    friend class QCoreApplicationPrivate;
    friend class QWidget;
    friend class QThreadData;

};

inline QMetaObject::Connection QObject::connect(const QObject *asender, const char *asignal,
                                            const char *amember, Qt::ConnectionType atype) const
{ return connect(asender, asignal, this, amember, atype); }



class  QObjectUserData {
public:
    virtual ~QObjectUserData();
};


#ifdef Q_QDOC
T qFindChild(const QObject *o, const QString &name = QString());
QList<T> qFindChildren(const QObject *oobj, const QString &name = QString());
QList<T> qFindChildren(const QObject *o, const QRegExp &re);
#endif


// oye QObject 继承体系下的 down-cast 这么做?
// QT定了qobject_cast转型, 其实就是通过MetaObject来做判断
// 比如有了 QStyle* obj, 现在判断是否是 QCascadeSheetStyle
template <class T>
inline T qobject_cast(QObject *object)
{
	// ObjType 其实就是T类型, 去掉 const * &之类的东西
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
    Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                    "qobject_cast requires the type to have a Q_OBJECT macro");

	// 尝试拿到T class的静态MetaObject,来判断当前给的object,是否和T是同一个继承体系的
    return static_cast<T>(ObjType::staticMetaObject.cast(object));
}

template <class T>
inline T qobject_cast(const QObject *object)
{
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
    Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                      "qobject_cast requires the type to have a Q_OBJECT macro");
    return static_cast<T>(ObjType::staticMetaObject.cast(object));
}


template <class T> inline const char * qobject_interface_iid()
{ return Q_NULLPTR; }



class QSignalBlocker
{
public:
    inline explicit QSignalBlocker(QObject *o) 
    : m_o(o),
      m_blocked(o && o->blockSignals(true)),
      m_inhibited(false)
	{}
    inline explicit 
	QSignalBlocker(QObject &o) 
		: m_o(&o),
		  m_blocked(o.blockSignals(true)),
		  m_inhibited(false)
	{}
    
    inline ~QSignalBlocker()	{
	    if (m_o && !m_inhibited)
	        m_o->blockSignals(m_blocked);
	}

    inline void reblock(){
    	if (m_o) m_o->blockSignals(true);
    	m_inhibited = false;
	}
    inline void unblock() 	{
	    if (m_o) m_o->blockSignals(m_blocked);
	    m_inhibited = true;
	}
private:
    QObject * m_o;
    bool m_blocked;
    bool m_inhibited;
};


namespace QtPrivate {
    inline QObject & deref_for_methodcall(QObject &o) { return  o; }
    inline QObject & deref_for_methodcall(QObject *o) { return *o; }
}
#define Q_SET_OBJECT_NAME(obj) QT_PREPEND_NAMESPACE(QtPrivate)::deref_for_methodcall(obj).setObjectName(QLatin1String(#obj))



#endif // QOBJECT_H

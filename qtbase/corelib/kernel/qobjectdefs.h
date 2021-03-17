#ifndef QOBJECTDEFS_H
#define QOBJECTDEFS_H

#include <QtCore/qnamespace.h>

#include <QtCore/qobjectdefs_impl.h>

QT_BEGIN_NAMESPACE

typedef QArrayData QByteArrayData;


#define Q_MOC_OUTPUT_REVISION 67

#define QT_ANNOTATE_CLASS(type, x)
#define QT_ANNOTATE_CLASS(type, ...)
#define QT_ANNOTATE_CLASS2(type, a1, a2)
#define QT_ANNOTATE_FUNCTION(x)
#define QT_ANNOTATE_ACCESS_SPECIFIER(x)


// The following macros are our "extensions" to C++
// They are used, strictly speaking, only by the moc.
// oye: MOC回来scan一遍源代码,在里面找这些他认为的关键字,然后去做额外的处理

// 比下面2位仁兄更简洁的定义方法
#define slots Q_SLOTS
#define signals Q_SIGNALS

#define Q_SLOTS 						QT_ANNOTATE_ACCESS_SPECIFIER(qt_slot)
#define Q_SIGNALS 						public QT_ANNOTATE_ACCESS_SPECIFIER(qt_signal)

// Widget的源码里面很常见 -> 比如是被moc特殊处理的
#define Q_PRIVATE_SLOT(d, signature) 	QT_ANNOTATE_CLASS2(qt_private_slot, d, signature)


#define Q_EMIT
#define emit   // oye 发送信号专用



#define Q_CLASSINFO(name, value)

#define Q_PLUGIN_METADATA(x) 	QT_ANNOTATE_CLASS(qt_plugin_metadata, x)
#define Q_INTERFACES(x) 		QT_ANNOTATE_CLASS(qt_interfaces, x)

// Widget里面的属性定义
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#define Q_PROPERTY(text) QT_ANNOTATE_CLASS(qt_property, text)
#define Q_PRIVATE_PROPERTY(d, text) QT_ANNOTATE_CLASS2(qt_private_property, d, text)

#define Q_REVISION(v)

#define Q_OVERRIDE(text) QT_ANNOTATE_CLASS(qt_override, text)
#define QDOC_PROPERTY(text) QT_ANNOTATE_CLASS(qt_qdoc_property, text)
#define Q_ENUMS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_FLAGS(x) QT_ANNOTATE_CLASS(qt_enums, x)

// oye class Qt 里面的 enum 和falg的定义
#define Q_ENUM_IMPL(ENUM) \
    friend Q_DECL_CONSTEXPR const QMetaObject *qt_getEnumMetaObject(ENUM) Q_DECL_NOEXCEPT { return &staticMetaObject; } \
    friend Q_DECL_CONSTEXPR const char *qt_getEnumName(ENUM) Q_DECL_NOEXCEPT { return #ENUM; }
	
#define Q_ENUM(x) Q_ENUMS(x) Q_ENUM_IMPL(x)
#define Q_FLAG(x) Q_FLAGS(x) Q_ENUM_IMPL(x)

#define Q_ENUM_NS_IMPL(ENUM) \
    inline Q_DECL_CONSTEXPR const QMetaObject *qt_getEnumMetaObject(ENUM) Q_DECL_NOEXCEPT { return &staticMetaObject; } \
    inline Q_DECL_CONSTEXPR const char *qt_getEnumName(ENUM) Q_DECL_NOEXCEPT { return #ENUM; }
	
#define Q_ENUM_NS(x) Q_ENUMS(x) Q_ENUM_NS_IMPL(x)
#define Q_FLAG_NS(x) Q_FLAGS(x) Q_ENUM_NS_IMPL(x)

// oye 目前还没有碰到
#define Q_SCRIPTABLE 	QT_ANNOTATE_FUNCTION(qt_scriptable)
#define Q_INVOKABLE  	QT_ANNOTATE_FUNCTION(qt_invokable)
#define Q_SIGNAL 		QT_ANNOTATE_FUNCTION(qt_signal)
#define Q_SLOT 			QT_ANNOTATE_FUNCTION(qt_slot)


// full set of tr functions
#define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = Q_NULLPTR, int n = -1) \
        { return staticMetaObject.tr(s, c, n); } \
    static inline QString trUtf8(const char *s, const char *c = Q_NULLPTR, int n = -1) \
        { return staticMetaObject.tr(s, c, n); }


/*oye 修改了一些不重要的额外修饰符
	-元信息, [类名,父类名,信号,槽]
	-动态转换时元给的帮助
	-槽的分派
	-i18l转换
	-私有信号????
*/
#define Q_OBJECT \
public: \
    static const QMetaObject staticMetaObject; \
    virtual const QMetaObject *metaObject() const; \
    virtual void *qt_metacast(const char *); \
    virtual int qt_metacall(QMetaObject::Call, int, void **); \
    QT_TR_FUNCTIONS \
private: \
    static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **); \
    struct QPrivateSignal {}; \
    QT_ANNOTATE_CLASS(qt_qobject, "")


#define Q_OBJECT_FAKE Q_OBJECT QT_ANNOTATE_CLASS(qt_fake, "")

/*
oye	-有静态元对象
	-
*/
#define Q_GADGET \
public: \
    static const QMetaObject staticMetaObject; \
    void qt_check_for_QGADGET_macro(); \
    typedef void QtGadgetHelper; \
private: \
    static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **); \
    QT_ANNOTATE_CLASS(qt_qgadget, "") \
    /*end*/


// OYE 目前还没有碰到
#define Q_NAMESPACE \
    extern const QMetaObject staticMetaObject; \
    QT_ANNOTATE_CLASS(qt_qnamespace, "") \
    /*end*/



const char *qFlagLocation(const char *method);


// QT_STRINGIFY : Line是数字,变为字符串 #define QT_STRINGIFY2(x) #x
#define QLOCATION "\0" __FILE__ ":" QT_STRINGIFY(__LINE__)

//
// oye 在connect的建立联系中,用到
//
#define METHOD(a)   qFlagLocation("0"#a QLOCATION)
#define SLOT(a)     qFlagLocation("1"#a QLOCATION)
#define SIGNAL(a)   qFlagLocation("2"#a QLOCATION)


#define QMETHOD_CODE  0                        // member type codes
#define QSLOT_CODE    1
#define QSIGNAL_CODE  2



// oye 我猜想,定义数据是,一定用此宏来做,方笔
#define Q_ARG(type, data) QArgument<type >(#type, data)
#define Q_RETURN_ARG(type, data) QReturnArgument<type >(#type, data)


class QMetaMethod;
class QMetaEnum;
class QMetaProperty;
class QMetaClassInfo;


class  QGenericArgument
{
private:
    const void *_data;  //什么数据类型都可以?
    const char *_name;

public:
    inline QGenericArgument(const char *aName = Q_NULLPTR, const void *aData = Q_NULLPTR)
        : _data(aData), _name(aName) {}
    inline void *data() const { return const_cast<void *>(_data); }
    inline const char *name() const { return _name; }


};

// 只有不同类名的意义,用来区分的不同type [入参 | 返回值 ]
class QGenericReturnArgument: public QGenericArgument
{
public:
    inline QGenericReturnArgument(const char *aName = Q_NULLPTR, void *aData = Q_NULLPTR)
        : QGenericArgument(aName, aData)    {}
};

template <typename T>
class QReturnArgument: public QGenericReturnArgument
{
public:
    inline QReturnArgument(const char *aName, T &aData)
        : QGenericReturnArgument(aName, static_cast<void *>(&aData))
        {}
};
		
template <class T>
class QArgument: public QGenericArgument
{
public:
    inline QArgument(const char *aName, const T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};
template <class T>
class QArgument<T &>: public QGenericArgument
{
public:
    inline QArgument(const char *aName, T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};


typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **);	


struct QMetaObject
{
	public:

    struct { // private data
    const QMetaObject *superdata;		//oye 以组合的形式,持有父类的data,便于向上查找
    const QByteArrayData *stringdata;	//oye 其要表征的类名    QPushButton, QMainWindow之类
    const uint *data;  					//oye 魔法数据,但可以转为 QMetaObjectPrivate*   通过内部函数 priv(const uint* data)
    // 这个函数用来实现调用具体的receiver
    StaticMetacallFunction static_metacall;	
	
    const QMetaObject * const *relatedMetaObjects;	
    void *extradata; //reserved for future use
    } d;


public:

    class Connection;
    const char *className() const;	
	inline const QMetaObject *superClass() const{ return d.superdata; }

	// metaObject是否和this的Type是同一个继承体系的
    bool inherits(const QMetaObject *metaObject) const ;  // 向上不断判断 super == metaObject

	// 是否可以把Obj 转换成 this的类型
	const QObject *cast(const QObject *obj) const{
		return (obj && obj->metaObject()->inherits(this)) ? obj : nullptr;
    }
	
    QObject *cast(QObject *obj) const{
		return const_cast<QObject*>(cast(const_cast<const QObject*>(obj)));
    }
    

    QString tr(const char *s, const char *c, int n = -1) const{
    	return QCoreApplication::translate(objectClassName(this), s, c, n);
    }

    enum Call {
        InvokeMetaMethod,
        ReadProperty,
        WriteProperty,
        ResetProperty,
        QueryPropertyDesignable,
        QueryPropertyScriptable,
        QueryPropertyStored,
        QueryPropertyEditable,
        QueryPropertyUser,
        CreateInstance,
        IndexOfMethod,
        RegisterPropertyMetaType,
        RegisterMethodArgumentMetaType
    };

    int static_metacall(Call, int, void **) const;
    static int metacall(QObject *, Call, int, void **);
	

    int methodOffset() const;
    int enumeratorOffset() const;
    int propertyOffset() const;
    int classInfoOffset() const;

    int constructorCount() const;
    int methodCount() const;
    int enumeratorCount() const;
    int propertyCount() const;
    int classInfoCount() const;

    int indexOfConstructor(const char *constructor) const;
    int indexOfMethod(const char *method) const;
    int indexOfSignal(const char *signal) const;
    int indexOfSlot(const char *slot) const;
    int indexOfEnumerator(const char *name) const;
    int indexOfProperty(const char *name) const;
    int indexOfClassInfo(const char *name) const;

    QMetaMethod constructor(int index) const;
    QMetaMethod method(int index) const;
    QMetaEnum enumerator(int index) const;
    QMetaProperty property(int index) const;
    QMetaClassInfo classInfo(int index) const;
    QMetaProperty userProperty() const;

    static bool checkConnectArgs(const char *signal, const char *method);
    static bool checkConnectArgs(const QMetaMethod &signal,
                                 const QMetaMethod &method);
    static QByteArray normalizedSignature(const char *method);
    static QByteArray normalizedType(const char *type);

    // internal index-based connect
    static Connection connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index,
                        int type = 0, int *types = Q_NULLPTR);
    // internal index-based disconnect
    static bool disconnect(const QObject *sender, int signal_index,
                           const QObject *receiver, int method_index);
    static bool disconnectOne(const QObject *sender, int signal_index,
                              const QObject *receiver, int method_index);
    // internal slot-name based connect
    static void connectSlotsByName(QObject *o);

    // internal index-based signal activation
    // emit signal时的实际操作
	// oye emit的实现原理, 比如还是激活this里面的ObjectPrivate里面的ConnectionList
	// 从中找到指定的,然后寻找receiver的staticMetaCall 进行调用
    static void activate(QObject *sender, int signal_index, void **argv);
    static void activate(QObject *sender, const QMetaObject *, int local_signal_index, void **argv);
    static void activate(QObject *sender, int signal_offset, int local_signal_index, void **argv);

    static bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument());

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, ret, val0, val1, val2, val3,
                val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType type,
                             QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, type, QGenericReturnArgument(), val0, val1, val2,
                                 val3, val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, QGenericReturnArgument(), val0,
                val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }

    QObject *newInstance(QGenericArgument val0 = QGenericArgument(Q_NULLPTR),
                         QGenericArgument val1 = QGenericArgument(),
                         QGenericArgument val2 = QGenericArgument(),
                         QGenericArgument val3 = QGenericArgument(),
                         QGenericArgument val4 = QGenericArgument(),
                         QGenericArgument val5 = QGenericArgument(),
                         QGenericArgument val6 = QGenericArgument(),
                         QGenericArgument val7 = QGenericArgument(),
                         QGenericArgument val8 = QGenericArgument(),
                         QGenericArgument val9 = QGenericArgument()) const;




};

/*
	Represents a handle to a signal-slot connection.

	It can be used to disconnect that connection, 

	or check if	the connection was successful

    ###	QObjectPrivate::Connection*  

*/
class  QMetaObject::Connection {
    void *d_ptr; //QObjectPrivate::Connection*
    explicit Connection(void *data) : d_ptr(data) {  }
    friend class QObject;
    friend class QObjectPrivate;
    friend struct QMetaObject;
	
	/*! \internal Returns true if the object is still connected */
    bool isConnected_helper() const{		return static_cast<QObjectPrivate::Connection *>(d_ptr)->receiver;    }
public:
	Connection(): d_ptr(0) {};
	
    ~Connection(){
    	if (d_ptr)  	static_cast<QObjectPrivate::Connection *>(d_ptr)->deref();
	}

    typedef void *Connection::*RestrictedBool;
    operator RestrictedBool() const { return d_ptr && isConnected_helper() ? &Connection::d_ptr : Q_NULLPTR; }
	
};



namespace QtPrivate {
    /* Trait that tells is a the Object has a Q_OBJECT macro */
    template <typename Object> struct HasQ_OBJECT_Macro {
        template <typename T>
        static char test(int (T::*)(QMetaObject::Call, int, void **));
        static int test(int (Object::*)(QMetaObject::Call, int, void **));
        enum { Value =  sizeof(test(&Object::qt_metacall)) == sizeof(int) };
    };
}

QT_END_NAMESPACE

#endif // QOBJECTDEFS_H

#ifndef QOBJECT_P_H
#define QOBJECT_P_H

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qobject.h"
#include "QtCore/qpointer.h"
#include "QtCore/qsharedpointer.h"
#include "QtCore/qcoreevent.h"
#include "QtCore/qlist.h"
#include "QtCore/qvector.h"
#include "QtCore/qvariant.h"
#include "QtCore/qreadwritelock.h"



class QVariant;
class QThreadData;
class QObjectConnectionListVector;
namespace QtSharedPointer { struct ExternalRefCountData; }

/* for Qt Test */
struct QSignalSpyCallbackSet
{
    typedef void (*BeginCallback)(QObject *caller, int signal_or_method_index, void **argv);
    typedef void (*EndCallback)(QObject *caller, int signal_or_method_index);
    BeginCallback signal_begin_callback,
                    slot_begin_callback;
    EndCallback signal_end_callback,
                slot_end_callback;
};
void  qt_register_signal_spy_callbacks(const QSignalSpyCallbackSet &callback_set);

extern QSignalSpyCallbackSet  qt_signal_spy_callback_set;

enum { QObjectPrivateVersion = QT_VERSION };

class QAbstractDeclarativeData
{
public:
    static void (*destroyed)(QAbstractDeclarativeData *, QObject *);
    static void (*destroyed_qml1)(QAbstractDeclarativeData *, QObject *);
    static void (*parentChanged)(QAbstractDeclarativeData *, QObject *, QObject *);
    static void (*signalEmitted)(QAbstractDeclarativeData *, QObject *, int, void **);
    static int  (*receivers)(QAbstractDeclarativeData *, const QObject *, int);
    static bool (*isSignalConnected)(QAbstractDeclarativeData *, const QObject *, int);
    static void (*setWidgetParent)(QObject *, QObject *); // Used by the QML engine to specify parents for widgets. Set by QtWidgets.
};

// This is an implementation of QAbstractDeclarativeData that is identical with
// the implementation in QtDeclarative and QtQml for the first bit
struct QAbstractDeclarativeDataImpl : public QAbstractDeclarativeData
{
    quint32 ownedByQml1:1;
    quint32 unused: 31;
};

/*
	oye
	-1 sig-slot connection
	-2 属性,用户数据
	-3 事件过滤器
	-4 
*/
class  QObjectPrivate : public QObjectData
{
    //Q_DECLARE_PUBLIC(QObject)
    inline QObject* q_func() { return static_cast<Class *>(q_ptr); }
public:
    ExtraData *extraData;    // extra data set by the user

	// oye 每个这样的Object对象都关联了threadData, 相同线程的值必然相同
	// 我在QCoreApplication::postEvent中看到,  当给对象投递消息时, 系统会判断此对象的threadData值是否发生了改变
	// 对上的所属线程在运行时有可能发生改变
	// 若果值为NULL, 则Obj正在被析构
    QThreadData *threadData; // id of the thread that owns the object

	// oye sender 存放sig-slot connection的地方
    QObjectConnectionListVector *connectionLists;

    Connection *senders;     // linked list of connections connected to this object
    Sender *currentSender;   // object currently activating the object
    mutable quint32 connectedSignals[2];

    union {
        QObject *currentChildBeingDeleted;
        QAbstractDeclarativeData *declarativeData; //extra data used by the declarative module
    };

    // these objects are all used to indicate that a QObject was deleted
    // plus QPointer, which keeps a separate list
    QAtomicPointer<QtSharedPointer::ExternalRefCountData> sharedRefcount;  // 智能指针会用到, 判断是否被共享
	
public:
    struct ExtraData
    {
        ExtraData() {}    
        QVector<QObjectUserData *> userData;
        QList<QByteArray> propertyNames;
        QVector<QVariant> propertyValues;
        QVector<int> runningTimers;

		// QObject做基类的event_filter
		// QCoreApplicationPrivate::sendThroughApplicationEventFilters
        QList<QPointer<QObject> > eventFilters;  // QObject里面的标准方法 installEventFilter
        QString objectName;
    };

    typedef void (*StaticMetaCallFunction)(QObject *, QMetaObject::Call, int, void **);
    struct Connection
    {
        QObject *sender;
        QObject *receiver;
        union {
            StaticMetaCallFunction callFunction;
            QtPrivate::QSlotObjectBase *slotObj;
        };
        // The next pointer for the singly-linked ConnectionList
        Connection *nextConnectionList;

		// oye 居然看到了驱动的玩法
        //senders linked list
        Connection *     next;
        Connection **    prev;
		
        QAtomicPointer<const int> argumentTypes;
        QAtomicInt ref_;
        ushort method_offset;
        ushort method_relative;
        uint signal_index : 27; // In signal range (see QObjectPrivate::signalIndex())
        ushort connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        ushort isSlotObject : 1;
        ushort ownArgumentTypes : 1;
        Connection() : nextConnectionList(0), ref_(2), ownArgumentTypes(true) {
            //ref_ is 2 for the use in the internal lists, 
            // and for the use in QMetaObject::Connection
        }
        ~Connection();
        int method() const {  return method_offset + method_relative; }
        void ref() { ref_.ref(); }
        void deref() {
            if (!ref_.deref()) {
                delete this;
            }
        }
    };
    // ConnectionList is a singly-linked list
    struct ConnectionList {
        ConnectionList() : first(0), last(0) {}
        Connection *first;
        Connection *last;
    };

    struct Sender
    {
        QObject *sender;
        int signal;
        int ref;
    };


    QObjectPrivate(int version = QObjectPrivateVersion);
    virtual ~QObjectPrivate();
    void deleteChildren();

    void setParent_helper(QObject *);
    void moveToThread_helper();
    void setThreadData_helper(QThreadData *currentData, QThreadData *targetData);
    void _q_reregisterTimers(void *pointer);

    bool isSender(const QObject *receiver, const char *signal) const;
    QObjectList receiverList(const char *signal) const;
    QObjectList senderList() const;

    void addConnection(int signal, Connection *c);
    void cleanConnectionLists();

    static inline Sender *setCurrentSender(QObject *receiver,
                                    Sender *sender);
    static inline void resetCurrentSender(QObject *receiver,
                                   Sender *currentSender,
                                   Sender *previousSender);

	// QObjectPrivate 本身是内部使用的, 这里提供了一个快速转换的方法 Object -> ObjectPrivate
    static QObjectPrivate *get(QObject *o) {       return o->d_func();   }
    static const QObjectPrivate *get(const QObject *o) { return o->d_func(); }
	

    int signalIndex(const char *signalName, const QMetaObject **meta = 0) const;
    inline bool isSignalConnected(uint signalIdx, bool checkDeclarative = true) const;
    inline bool isDeclarativeSignalConnected(uint signalIdx) const;

    // To allow abitrary objects to call connectNotify()/disconnectNotify() without making
    // the API public in QObject. This is used by QQmlNotifierEndpoint.
    inline void connectNotify(const QMetaMethod &signal);
    inline void disconnectNotify(const QMetaMethod &signal);

    template <typename Func1, typename Func2>
    static inline QMetaObject::Connection connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot,
                                                  Qt::ConnectionType type = Qt::AutoConnection);

    template <typename Func1, typename Func2>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot);

    static QMetaObject::Connection connectImpl(const QObject *sender, int signal_index,
                                               const QObject *receiver, void **slot,
                                               QtPrivate::QSlotObjectBase *slotObj, Qt::ConnectionType type,
                                               const int *types, const QMetaObject *senderMetaObject);
    static QMetaObject::Connection connect(const QObject *sender, int signal_index, QtPrivate::QSlotObjectBase *slotObj, Qt::ConnectionType type);
    static bool disconnect(const QObject *sender, int signal_index, void **slot);

};


/*
  Returns true if the signal with index signal_index from object sender is connected.
  
  Signals with indices above a certain range are always considered connected (see connectedSignals
  in QObjectPrivate).

*/
inline bool QObjectPrivate::isSignalConnected(uint signal_index, bool checkDeclarative) const
{
    return signal_index >= sizeof(connectedSignals) * 8
        || (connectedSignals[signal_index >> 5] & (1 << (signal_index & 0x1f))
        || (checkDeclarative && isDeclarativeSignalConnected(signal_index)));
}

inline bool QObjectPrivate::isDeclarativeSignalConnected(uint signal_index) const
{
    return declarativeData && QAbstractDeclarativeData::isSignalConnected
            && QAbstractDeclarativeData::isSignalConnected(declarativeData, q_func(), signal_index);
}

inline QObjectPrivate::Sender *QObjectPrivate::setCurrentSender(QObject *receiver,
                                                         Sender *sender)
{
    Sender *previousSender = receiver->d_func()->currentSender;
    receiver->d_func()->currentSender = sender;
    return previousSender;
}

inline void QObjectPrivate::resetCurrentSender(QObject *receiver,
                                        Sender *currentSender,
                                        Sender *previousSender)
{
    // ref is set to zero when this object is deleted during the metacall
    if (currentSender->ref == 1)
        receiver->d_func()->currentSender = previousSender;
    // if we've recursed, we need to tell the caller about the objects deletion
    if (previousSender)
        previousSender->ref = currentSender->ref;
}

inline void QObjectPrivate::connectNotify(const QMetaMethod &signal)
{
    q_ptr->connectNotify(signal);
}

inline void QObjectPrivate::disconnectNotify(const QMetaMethod &signal)
{
    q_ptr->disconnectNotify(signal);
}

namespace QtPrivate {
template<typename Func, typename Args, typename R> class QPrivateSlotObject : public QSlotObjectBase
{
    typedef QtPrivate::FunctionPointer<Func> FuncType;
    Func function;
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
    {
        switch (which) {
            case Destroy:
                delete static_cast<QPrivateSlotObject*>(this_);
                break;
            case Call:
                FuncType::template call<Args, R>(static_cast<QPrivateSlotObject*>(this_)->function,
                                                 static_cast<typename FuncType::Object *>(QObjectPrivate::get(r)), a);
                break;
            case Compare:
                *ret = *reinterpret_cast<Func *>(a) == static_cast<QPrivateSlotObject*>(this_)->function;
                break;
            case NumOperations: ;
        }
    }
public:
    explicit QPrivateSlotObject(Func f) : QSlotObjectBase(&impl), function(f) {}
};
} //namespace QtPrivate

template <typename Func1, typename Func2>
inline QMetaObject::Connection QObjectPrivate::connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                                       const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot,
                                                       Qt::ConnectionType type)
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

    const int *types = 0;
    if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
        types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

    return QObject::connectImpl(sender, reinterpret_cast<void **>(&signal),
        receiverPrivate->q_ptr, reinterpret_cast<void **>(&slot),
        new QtPrivate::QPrivateSlotObject<Func2, typename QtPrivate::List_Left<typename SignalType::Arguments, SlotType::ArgumentCount>::Value,
                                        typename SignalType::ReturnType>(slot),
        type, types, &SignalType::Object::staticMetaObject);
}

template <typename Func1, typename Func2>
bool QObjectPrivate::disconnect(const typename QtPrivate::FunctionPointer< Func1 >::Object* sender, Func1 signal,
                                const typename QtPrivate::FunctionPointer< Func2 >::Object* receiverPrivate, Func2 slot)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    typedef QtPrivate::FunctionPointer<Func2> SlotType;
    Q_STATIC_ASSERT_X(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                      "No Q_OBJECT in the class with the signal");
    //compilation error if the arguments does not match.
    Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                      "Signal and slot arguments are not compatible.");
    return QObject::disconnectImpl(sender, reinterpret_cast<void **>(&signal),
                          receiverPrivate->q_ptr, reinterpret_cast<void **>(&slot),
                          &SignalType::Object::staticMetaObject);
}

Q_DECLARE_TYPEINFO(QObjectPrivate::Connection, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QObjectPrivate::Sender, Q_MOVABLE_TYPE);

class QSemaphore;
class QMetaCallEvent : public QEvent
{
private:
    QtPrivate::QSlotObjectBase *slotObj_;
    const QObject *sender_;
    int signalId_;
    int nargs_;
    int *types_;
    void **args_;
    QSemaphore *semaphore_;
    QObjectPrivate::StaticMetaCallFunction callFunction_;
    ushort method_offset_;
    ushort method_relative_;

public:
    QMetaCallEvent(
			ushort method_offset, 
			ushort method_relative, 
			QObjectPrivate::StaticMetaCallFunction callFunction , 
			const QObject *sender, int signalId,
           int nargs = 0, int *types = 0, void **args = 0, QSemaphore *semaphore = 0);
    /*! \internal
        \a signalId is in the signal index range (see QObjectPrivate::signalIndex()).
    */
    QMetaCallEvent(QtPrivate::QSlotObjectBase *slotObj, const QObject *sender, int signalId,
                   int nargs = 0, int *types = 0, void **args = 0, QSemaphore *semaphore = 0);

    ~QMetaCallEvent();

    inline int id() const { return method_offset_ + method_relative_; }
    inline const QObject *sender() const { return sender_; }
    inline int signalId() const { return signalId_; }
    inline void **args() const { return args_; }

	// oye 是不是要通过他来实现隐藏QMetaCallEvent类本身的信息???
    virtual void placeMetaCall(QObject *object);


};

class QBoolBlocker
{
    Q_DISABLE_COPY(QBoolBlocker)
public:
    explicit inline QBoolBlocker(bool &b, bool value=true):block(b), reset(b){block = value;}
    inline ~QBoolBlocker(){block = reset; }
private:
    bool &block;
    bool reset;
};

void  qDeleteInEventHandler(QObject *o);

/*
	前项声明
	让基类QDynamicMetaObjectData 会返回一个子类的对象指针,
*/
struct QAbstractDynamicMetaObject;
struct  QDynamicMetaObjectData
{
    virtual ~QDynamicMetaObjectData(){}
    virtual void objectDestroyed(QObject *) { delete this; }
	// 基类里面,返回子类的对象指针
    virtual QAbstractDynamicMetaObject *toDynamicMetaObject(QObject *) = 0;
    virtual int metaCall(QObject *, QMetaObject::Call, int _id,  void **) = 0;
};

struct  QAbstractDynamicMetaObject : public QDynamicMetaObjectData, public QMetaObject
{
    ~QAbstractDynamicMetaObject(){}

    virtual QAbstractDynamicMetaObject *toDynamicMetaObject(QObject *) Q_DECL_OVERRIDE { return this; }
    virtual int createProperty(const char *, const char *) { return -1; }
    virtual int metaCall(QObject *, QMetaObject::Call c, int _id, void **a) Q_DECL_OVERRIDE
    { return metaCall(c, _id, a); }
    virtual int metaCall(QMetaObject::Call, int _id, void **) { return _id; } // Compat overload
};


#endif // QOBJECT_P_H

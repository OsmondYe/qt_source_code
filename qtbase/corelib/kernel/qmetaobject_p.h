#ifndef QMETAOBJECT_P_H
#define QMETAOBJECT_P_H

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>
#ifndef QT_NO_QOBJECT
#include <private/qobject_p.h> // For QObjectPrivate::Connection
#endif
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE


void osmond_no_meaning();


enum PropertyFlags  {
    Invalid = 0x00000000,
    Readable = 0x00000001,
    Writable = 0x00000002,
    Resettable = 0x00000004,
    EnumOrFlag = 0x00000008,
    StdCppSet = 0x00000100,
//     Override = 0x00000200,
    Constant = 0x00000400,
    Final = 0x00000800,
    Designable = 0x00001000,
    ResolveDesignable = 0x00002000,
    Scriptable = 0x00004000,
    ResolveScriptable = 0x00008000,
    Stored = 0x00010000,
    ResolveStored = 0x00020000,
    Editable = 0x00040000,
    ResolveEditable = 0x00080000,
    User = 0x00100000,
    ResolveUser = 0x00200000,
    Notify = 0x00400000,
    Revisioned = 0x00800000
};

enum MethodFlags  {
    AccessPrivate = 0x00,
    AccessProtected = 0x01,
    AccessPublic = 0x02,
    AccessMask = 0x03, //mask

    MethodMethod = 0x00,
    MethodSignal = 0x04,
    MethodSlot = 0x08,
    MethodConstructor = 0x0c,
    MethodTypeMask = 0x0c,

    MethodCompatibility = 0x10,
    MethodCloned = 0x20,
    MethodScriptable = 0x40,
    MethodRevisioned = 0x80
};

enum MetaObjectFlags {
    DynamicMetaObject = 0x01,
    RequiresVariantMetaObject = 0x02,
    PropertyAccessInStaticMetaCall = 0x04 // since Qt 5.5, property code is in the static metacall
};

enum MetaDataFlags {
    IsUnresolvedType = 0x80000000,
    TypeNameIndexMask = 0x7FFFFFFF
};

enum EnumFlags {
    EnumIsFlag = 0x1,
    EnumIsScoped = 0x2
};

extern int qMetaTypeTypeInternal(const char *);

class QArgumentType
{
private:
    int _type;
    QByteArray _name;	

public:
    QArgumentType(int type)    : _type(type) {}
    QArgumentType(const QByteArray &name)
        : _type(qMetaTypeTypeInternal(name.constData())), _name(name)
    {}
    QArgumentType()      : _type(0)   {}
    int type() const  { return _type; }
    QByteArray name() const
    {
        if (_type && _name.isEmpty())
            const_cast<QArgumentType *>(this)->_name = QMetaType::typeName(_type);
        return _name;
    }
    bool operator==(const QArgumentType &other) const
    {
        if (_type && other._type)
            return _type == other._type;
        else
            return name() == other.name();
    }
    bool operator!=(const QArgumentType &other) const
    {
        if (_type && other._type)
            return _type != other._type;
        else
            return name() != other.name();
    }
};
//Q_DECLARE_TYPEINFO(QArgumentType, Q_MOVABLE_TYPE);

typedef QVarLengthArray<QArgumentType, 10> QArgumentTypeArray;

class QMetaMethodPrivate;
class QMutex;

struct QMetaObjectPrivate
{
    // revision 7 is Qt 5.0 everything lower is not supported
    enum { OutputRevision = 7 }; // Used by moc, qmetaobjectbuilder and qdbus
	// 按照一个固定的魔法数组来排列的, 顺序不能被打乱
    int revision;
    int className;
    int classInfoCount, classInfoData;
    int methodCount, methodData;
    int propertyCount, propertyData;
    int enumeratorCount, enumeratorData;
    int constructorCount, constructorData;
    int flags;
    int signalCount;

    static inline const QMetaObjectPrivate *get(const QMetaObject *metaobject)
    { return reinterpret_cast<const QMetaObjectPrivate*>(metaobject->d.data); }

    static int originalClone(const QMetaObject *obj, int local_method_index);

    static QByteArray decodeMethodSignature(const char *signature,
                                            QArgumentTypeArray &types);
    static int indexOfSignalRelative(const QMetaObject **baseObject,
                                     const QByteArray &name, int argc,
                                     const QArgumentType *types);
    static int indexOfSlotRelative(const QMetaObject **m,
                                   const QByteArray &name, int argc,
                                   const QArgumentType *types);
    static int indexOfSignal(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfSlot(const QMetaObject *m, const QByteArray &name,
                           int argc, const QArgumentType *types);
    static int indexOfMethod(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfConstructor(const QMetaObject *m, const QByteArray &name,
                                  int argc, const QArgumentType *types);
    static QMetaMethod signal(const QMetaObject *m, int signal_index);
    static int signalOffset(const QMetaObject *m);
	
    static int absoluteSignalCount(const QMetaObject *m);
    static int signalIndex(const QMetaMethod &m);
	
    static bool checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                 int methodArgc, const QArgumentType *methodTypes);
    static bool checkConnectArgs(const QMetaMethodPrivate *signal,
                                 const QMetaMethodPrivate *method);

    static QList<QByteArray> parameterTypeNamesFromSignature(const char *signature);

    //defined in qobject.cpp
    enum DisconnectType { DisconnectAll, DisconnectOne };
    static void memberIndexes(const QObject *obj, const QMetaMethod &member,
                              int *signalIndex, int *methodIndex);
    static QObjectPrivate::Connection *connect(const QObject *sender, int signal_index,
                        const QMetaObject *smeta,
                        const QObject *receiver, int method_index_relative,
                        const QMetaObject *rmeta = 0,
                        int type = 0, int *types = 0);
    static bool disconnect(const QObject *sender, int signal_index,
                           const QMetaObject *smeta,
                           const QObject *receiver, int method_index, void **slot,
                           DisconnectType = DisconnectAll);
    static inline bool disconnectHelper(QObjectPrivate::Connection *c,
                                        const QObject *receiver, int method_index, void **slot,
                                        QMutex *senderMutex, DisconnectType = DisconnectAll);
};

// For meta-object generators

enum { MetaObjectPrivateFieldCount = sizeof(QMetaObjectPrivate) / sizeof(int) };

#ifndef UTILS_H
// mirrored in moc's utils.h
static inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_'
       );
}

static inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}
#endif


QT_END_NAMESPACE

#endif


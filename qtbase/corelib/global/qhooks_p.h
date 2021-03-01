#ifndef QHOOKS_H
#define QHOOKS_H


#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE


namespace QHooks {

enum HookIndex {
    HookDataVersion = 0,
    HookDataSize = 1,
    QtVersion = 2,
    AddQObject = 3,
    RemoveQObject = 4,
    Startup = 5,
    TypeInformationVersion = 6,
    LastHookIndex
};

typedef void(*AddQObjectCallback)(QObject*);
typedef void(*RemoveQObjectCallback)(QObject*);
typedef void(*StartupCallback)();

}

extern quintptr Q_CORE_EXPORT qtHookData[];

QT_END_NAMESPACE

#endif

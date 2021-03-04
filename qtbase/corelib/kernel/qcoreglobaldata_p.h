#ifndef QCOREGLOBALDATA_P_H
#define QCOREGLOBALDATA_P_H



#include <QtCore/private/qglobal_p.h>
#include "QtCore/qmap.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qreadwritelock.h"
#include "QtCore/qhash.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qtextcodec.h"
#include "QtCore/qmutex.h"

QT_BEGIN_NAMESPACE

typedef QHash<QByteArray, QTextCodec *> QTextCodecCache;

struct QCoreGlobalData {
    QCoreGlobalData();
    ~QCoreGlobalData();

    QMap<QString, QStringList> dirSearchPaths;
    QReadWriteLock dirSearchPathsLock;

#if QT_CONFIG(textcodec)
    QList<QTextCodec*> allCodecs;
    QAtomicPointer<QTextCodec> codecForLocale;
    QTextCodecCache codecCache;
#endif

    static QCoreGlobalData *instance();
};


QT_END_NAMESPACE
#endif // QCOREGLOBALDATA_P_H


#include "qcoreglobaldata_p.h"
#include "qtextcodec.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QCoreGlobalData, globalInstance)

QCoreGlobalData::QCoreGlobalData()
#if QT_CONFIG(textcodec)
    : codecForLocale(0)
#endif
{
}

QCoreGlobalData::~QCoreGlobalData()
{
#if QT_CONFIG(textcodec)
    codecForLocale = 0;
    for (QList<QTextCodec *>::const_iterator it = allCodecs.constBegin(); it != allCodecs.constEnd(); ++it)
        delete *it;
#endif
}

QCoreGlobalData *QCoreGlobalData::instance()
{
    return globalInstance();
}

QT_END_NAMESPACE

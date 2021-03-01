#ifndef QFACTORYLOADER_P_H
#define QFACTORYLOADER_P_H

#include "QtCore/qglobal.h"
#ifndef QT_NO_QOBJECT

#include "QtCore/qobject.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qjsonobject.h"
#include "QtCore/qjsondocument.h"
#include "QtCore/qmap.h"
#include "QtCore/qendian.h"
#if QT_CONFIG(library)
#include "private/qlibrary_p.h"
#endif

QT_BEGIN_NAMESPACE

inline QJsonDocument qJsonFromRawLibraryMetaData(const char *raw)
{
    raw += strlen("QTMETADATA  ");
    // the size of the embedded JSON object can be found 8 bytes into the data (see qjson_p.h),
    // but doesn't include the size of the header (8 bytes)
    QByteArray json(raw, qFromLittleEndian<uint>(*(const uint *)(raw + 8)) + 8);
    return QJsonDocument::fromBinaryData(json);
}

class QFactoryLoaderPrivate;
class  QFactoryLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFactoryLoader)

public:
    explicit QFactoryLoader(const char *iid,
                   const QString &suffix = QString(),
                   Qt::CaseSensitivity = Qt::CaseSensitive);

#if QT_CONFIG(library)
    ~QFactoryLoader();

    void update();
    static void refreshAll();

#if defined(Q_OS_UNIX) && !defined (Q_OS_MAC)
    QLibraryPrivate *library(const QString &key) const;
#endif // Q_OS_UNIX && !Q_OS_MAC
#endif // QT_CONFIG(library)

    QMultiMap<int, QString> keyMap() const;
    int indexOf(const QString &needle) const;

    QList<QJsonObject> metaData() const;
    QObject *instance(int index) const;
};

template <class PluginInterface, class FactoryInterface, typename ...Args>
PluginInterface *qLoadPlugin(const QFactoryLoader *loader, const QString &key, Args &&...args)
{
    const int index = loader->indexOf(key);
    if (index != -1) {
        QObject *factoryObject = loader->instance(index);
        if (FactoryInterface *factory = qobject_cast<FactoryInterface *>(factoryObject))
            if (PluginInterface *result = factory->create(key, std::forward<Args>(args)...))
                return result;
    }
    return nullptr;
}

template <class PluginInterface, class FactoryInterface, typename Arg>
Q_DECL_DEPRECATED PluginInterface *qLoadPlugin1(const QFactoryLoader *loader, const QString &key, Arg &&arg)
{ return qLoadPlugin<PluginInterface, FactoryInterface>(loader, key, std::forward<Arg>(arg)); }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QFACTORYLOADER_P_H

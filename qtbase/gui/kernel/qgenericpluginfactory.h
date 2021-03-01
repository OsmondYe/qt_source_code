#ifndef QGENERICPLUGINFACTORY_H
#define QGENERICPLUGINFACTORY_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


class QString;
class QObject;

class  QGenericPluginFactory
{
public:
    static QStringList keys()
	{
		QStringList list;
	
		typedef QMultiMap<int, QString> PluginKeyMap;
		typedef PluginKeyMap::const_iterator PluginKeyMapConstIterator;
	
		const PluginKeyMap keyMap = loader()->keyMap();
		const PluginKeyMapConstIterator cend = keyMap.constEnd();
		for (PluginKeyMapConstIterator it = keyMap.constBegin(); it != cend; ++it)
			if (!list.contains(it.value()))
				list += it.value();
		return list;
	}
	
    static QObject *create(const QString& key, const QString &specification){
		return qLoadPlugin<QObject, QGenericPlugin>(loader(), key.toLower(), specification);
    }
};


Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,(QGenericPluginFactoryInterface_iid, QLatin1String("/generic"), Qt::CaseInsensitive))
     


QT_END_NAMESPACE

#endif // QGENERICPLUGINFACTORY_H

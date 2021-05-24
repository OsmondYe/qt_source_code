#ifndef QACTION_P_H
#define QACTION_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qaction.h"
#if QT_CONFIG(menu)
#include "QtWidgets/qmenu.h"
#endif
#if QT_CONFIG(graphicsview)
#include "private/qgraphicswidget_p.h"
#endif
#include "private/qobject_p.h"



class QShortcutMap;

class QActionPrivate : public QObjectPrivate
{
    //Q_DECLARE_PUBLIC(QAction)
public:
	QPointer<QActionGroup> group;	// 有些是action是共享的, 通过 group来实现
    QString text;
    QString iconText;
    QIcon icon;
    QString tooltip;
    QString statustip;
    QString whatsthis;
#ifndef QT_NO_SHORTCUT
    QKeySequence shortcut;
    QList<QKeySequence> alternateShortcuts;
#endif
    QVariant userData;
#ifndef QT_NO_SHORTCUT
    int shortcutId;
    QVector<int> alternateShortcutIds;
    Qt::ShortcutContext shortcutContext;
    uint autorepeat : 1;
#endif
    QFont font;
    QPointer<QMenu> menu;
    uint enabled : 1, forceDisabled : 1;
    uint visible : 1, forceInvisible : 1;
    uint checkable : 1;
    uint checked : 1;
    uint separator : 1;
    uint fontSet : 1;

    int iconVisibleInMenu : 3;  // Only has values -1, 0, and 1

    QAction::MenuRole menuRole;
    QAction::Priority priority;

    QList<QWidget *> widgets;
#if QT_CONFIG(graphicsview)
    QList<QGraphicsWidget *> graphicsWidgets;
#endif
public:
    QActionPrivate();
    ~QActionPrivate();

    static QActionPrivate *get(QAction *q)
    {
        return q->d_func();
    }

    bool showStatusText(QWidget *w, const QString &str);

  
#ifndef QT_NO_SHORTCUT
    void redoGrab(QShortcutMap &map);
    void redoGrabAlternate(QShortcutMap &map);
    void setShortcutEnabled(bool enable, QShortcutMap &map);

    static QShortcutMap *globalMap;
#endif // QT_NO_SHORTCUT

    void sendDataChanged();
};

#endif // QACTION_P_H

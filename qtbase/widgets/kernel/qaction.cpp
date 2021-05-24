#include "qaction.h"
#include "qactiongroup.h"

#include "qaction_p.h"
#include "qapplication.h"
#include "qevent.h"
#include "qlist.h"
#include <private/qshortcutmap_p.h>
#include <private/qapplication_p.h>
#if QT_CONFIG(menu)
#include <private/qmenu_p.h>
#endif
#include <private/qdebug_p.h>

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!qApp)) { \
        qWarning("QAction: Initialize QApplication before calling '" functionName "'."); \
        return; \
    }


/*
  internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString qt_strippedText(QString s)
{
    s.remove(QStringLiteral("..."));
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}


QActionPrivate::QActionPrivate() : group(0), enabled(1), forceDisabled(0),
                                   visible(1), forceInvisible(0), checkable(0), checked(0), separator(0), fontSet(false),
                                   iconVisibleInMenu(-1),
                                   menuRole(QAction::TextHeuristicRole),
                                   priority(QAction::NormalPriority)
{
#ifndef QT_NO_SHORTCUT
    shortcutId = 0;
    shortcutContext = Qt::WindowShortcut;
    autorepeat = true;
#endif
}

QActionPrivate::~QActionPrivate()
{
}

bool QActionPrivate::showStatusText(QWidget *widget, const QString &str)
{

    if(QObject *object = widget ? widget : parent) {
        QStatusTipEvent tip(str);
        QApplication::sendEvent(object, &tip);
        return true;
    }
    return false;
}

void QActionPrivate::sendDataChanged()
{
    QAction * const q= q_func();
    QActionEvent e(QEvent::ActionChanged, q);

	// 给每一个目前拥有此action的widget发送消息
    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *w = widgets.at(i);
        QApplication::sendEvent(w, &e);
    }
	
#if QT_CONFIG(graphicsview)
    for (int i = 0; i < graphicsWidgets.size(); ++i) {
        QGraphicsWidget *w = graphicsWidgets.at(i);
        QApplication::sendEvent(w, &e);
    }
#endif

    QApplication::sendEvent(q, &e);

    emit q->changed();
}

#ifndef QT_NO_SHORTCUT
void QActionPrivate::redoGrab(QShortcutMap &map)
{
    QAction * const q= q_func();
    if (shortcutId)
        map.removeShortcut(shortcutId, q);
    if (shortcut.isEmpty())
        return;
    shortcutId = map.addShortcut(q, shortcut, shortcutContext, qWidgetShortcutContextMatcher);
    if (!enabled)
        map.setShortcutEnabled(false, shortcutId, q);
    if (!autorepeat)
        map.setShortcutAutoRepeat(false, shortcutId, q);
}

void QActionPrivate::redoGrabAlternate(QShortcutMap &map)
{
    QAction * const q= q_func();
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.removeShortcut(id, q);
    }
    alternateShortcutIds.clear();
    if (alternateShortcuts.isEmpty())
        return;
    for(int i = 0; i < alternateShortcuts.count(); ++i) {
        const QKeySequence& alternate = alternateShortcuts.at(i);
        if (!alternate.isEmpty())
            alternateShortcutIds.append(map.addShortcut(q, alternate, shortcutContext, qWidgetShortcutContextMatcher));
        else
            alternateShortcutIds.append(0);
    }
    if (!enabled) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutEnabled(false, id, q);
        }
    }
    if (!autorepeat) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutAutoRepeat(false, id, q);
        }
    }
}

void QActionPrivate::setShortcutEnabled(bool enable, QShortcutMap &map)
{
    QAction * const q= q_func();
    if (shortcutId)
        map.setShortcutEnabled(enable, shortcutId, q);
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.setShortcutEnabled(enable, id, q);
    }
}
#endif // QT_NO_SHORTCUT


//
// QAction
//

QAction::QAction(QObject* parent)
    : QAction(*new QActionPrivate, parent)
{
}

QAction::QAction(QActionPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    QActionPrivate * const d= d_func();
    d->group = qobject_cast<QActionGroup *>(parent);
    if (d->group)
        d->group->addAction(this);
}


QAction::QAction(const QString &text, QObject* parent)
    : QAction(parent)
{
    QActionPrivate * const d= d_func();
    d->text = text;
}


QAction::QAction(const QIcon &icon, const QString &text, QObject* parent)
    : QAction(text, parent)
{
    QActionPrivate * const d= d_func();
    d->icon = icon;
}




QWidget *QAction::parentWidget() const
{
    QObject *ret = parent();
    while (ret && !ret->isWidgetType())
        ret = ret->parent();
    return (QWidget*)ret;
}


QList<QWidget *> QAction::associatedWidgets() const
{
    QActionPrivate * const d= d_func();
    return d->widgets;
}

#if QT_CONFIG(graphicsview)
/*!
  \since 4.5
  Returns a list of widgets this action has been added to.

  \sa QWidget::addAction(), associatedWidgets()
*/
QList<QGraphicsWidget *> QAction::associatedGraphicsWidgets() const
{
    QActionPrivate * const d= d_func();
    return d->graphicsWidgets;
}
#endif

#ifndef QT_NO_SHORTCUT
/*!
    \property QAction::shortcut
    \brief the action's primary shortcut key

    Valid keycodes for this property can be found in \l Qt::Key and
    \l Qt::Modifier. There is no default shortcut key.
*/
void QAction::setShortcut(const QKeySequence &shortcut)
{
    QAPP_CHECK("setShortcut");

    QActionPrivate * const d= d_func();
    if (d->shortcut == shortcut)
        return;

    d->shortcut = shortcut;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}


void QAction::setShortcuts(const QList<QKeySequence> &shortcuts)
{
    QActionPrivate * const d= d_func();

    QList <QKeySequence> listCopy = shortcuts;

    QKeySequence primary;
    if (!listCopy.isEmpty())
        primary = listCopy.takeFirst();

    if (d->shortcut == primary && d->alternateShortcuts == listCopy)
        return;

    QAPP_CHECK("setShortcuts");

    d->shortcut = primary;
    d->alternateShortcuts = listCopy;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}


void QAction::setShortcuts(QKeySequence::StandardKey key)
{
    QList <QKeySequence> list = QKeySequence::keyBindings(key);
    setShortcuts(list);
}


QKeySequence QAction::shortcut() const
{
    QActionPrivate * const d= d_func();
    return d->shortcut;
}

QList<QKeySequence> QAction::shortcuts() const
{
    QActionPrivate * const d= d_func();
    QList <QKeySequence> shortcuts;
    if (!d->shortcut.isEmpty())
        shortcuts << d->shortcut;
    if (!d->alternateShortcuts.isEmpty())
        shortcuts << d->alternateShortcuts;
    return shortcuts;
}


void QAction::setShortcutContext(Qt::ShortcutContext context)
{
    QActionPrivate * const d= d_func();
    if (d->shortcutContext == context)
        return;
    QAPP_CHECK("setShortcutContext");
    d->shortcutContext = context;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

Qt::ShortcutContext QAction::shortcutContext() const
{
    QActionPrivate * const d= d_func();
    return d->shortcutContext;
}


void QAction::setAutoRepeat(bool on)
{
    QActionPrivate * const d= d_func();
    if (d->autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->autorepeat = on;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

bool QAction::autoRepeat() const
{
    QActionPrivate * const d= d_func();
    return d->autorepeat;
}
#endif // QT_NO_SHORTCUT


void QAction::setFont(const QFont &font)
{
    QActionPrivate * const d= d_func();
    if (d->font == font)
        return;

    d->fontSet = true;
    d->font = font;
    d->sendDataChanged();
}

QFont QAction::font() const
{
    QActionPrivate * const d= d_func();
    return d->font;
}


/*!
    Destroys the object and frees allocated resources.
*/
QAction::~QAction()
{
    QActionPrivate * const d= d_func();
    for (int i = d->widgets.size()-1; i >= 0; --i) {
        QWidget *w = d->widgets.at(i);
        w->removeAction(this);
    }
#if QT_CONFIG(graphicsview)
    for (int i = d->graphicsWidgets.size()-1; i >= 0; --i) {
        QGraphicsWidget *w = d->graphicsWidgets.at(i);
        w->removeAction(this);
    }
#endif
    if (d->group)
        d->group->removeAction(this);
#ifndef QT_NO_SHORTCUT
    if (d->shortcutId && qApp) {
        qApp->d_func()->shortcutMap.removeShortcut(d->shortcutId, this);
        for(int i = 0; i < d->alternateShortcutIds.count(); ++i) {
            const int id = d->alternateShortcutIds.at(i);
            qApp->d_func()->shortcutMap.removeShortcut(id, this);
        }
    }
#endif
}


void QAction::setActionGroup(QActionGroup *group)
{
    QActionPrivate * const d= d_func();
    if(group == d->group)
        return;

    if(d->group)
        d->group->removeAction(this);
    d->group = group;
    if(group)
        group->addAction(this);
    d->sendDataChanged();
}


QActionGroup *QAction::actionGroup() const
{
    QActionPrivate * const d= d_func();
    return d->group;
}


void QAction::setIcon(const QIcon &icon)
{
    QActionPrivate * const d= d_func();
    d->icon = icon;
    d->sendDataChanged();
}

QIcon QAction::icon() const
{
    QActionPrivate * const d= d_func();
    return d->icon;
}

#if QT_CONFIG(menu)
/*!
  Returns the menu contained by this action. Actions that contain
  menus can be used to create menu items with submenus, or inserted
  into toolbars to create buttons with popup menus.

  \sa QMenu::addAction()
*/
QMenu *QAction::menu() const
{
    QActionPrivate * const d= d_func();
    return d->menu;
}

/*!
    Sets the menu contained by this action to the specified \a menu.
*/
void QAction::setMenu(QMenu *menu)
{
    QActionPrivate * const d= d_func();
    if (d->menu)
        d->menu->d_func()->setOverrideMenuAction(0); //we reset the default action of any previous menu
    d->menu = menu;
    if (menu)
        menu->d_func()->setOverrideMenuAction(this);
    d->sendDataChanged();
}
#endif // QT_CONFIG(menu)

/*!
  If \a b is true then this action will be considered a separator.

  How a separator is represented depends on the widget it is inserted
  into. Under most circumstances the text, submenu, and icon will be
  ignored for separator actions.

  \sa QAction::isSeparator()
*/
void QAction::setSeparator(bool b)
{
    QActionPrivate * const d= d_func();
    if (d->separator == b)
        return;

    d->separator = b;
    d->sendDataChanged();
}

/*!
  Returns \c true if this action is a separator action; otherwise it
  returns \c false.

  \sa QAction::setSeparator()
*/
bool QAction::isSeparator() const
{
    QActionPrivate * const d= d_func();
    return d->separator;
}

/*!
    \property QAction::text
    \brief the action's descriptive text

    If the action is added to a menu, the menu option will consist of
    the icon (if there is one), the text, and the shortcut (if there
    is one). If the text is not explicitly set in the constructor, or
    by using setText(), the action's description icon text will be
    used as text. There is no default text.

    \sa iconText
*/
void QAction::setText(const QString &text)
{
    QActionPrivate * const d= d_func();
    if (d->text == text)
        return;

    d->text = text;
    d->sendDataChanged();
}

QString QAction::text() const
{
    QActionPrivate * const d= d_func();
    QString s = d->text;
    if(s.isEmpty()) {
        s = d->iconText;
        s.replace(QLatin1Char('&'), QLatin1String("&&"));
    }
    return s;
}





/*!
    \property QAction::iconText
    \brief the action's descriptive icon text

    If QToolBar::toolButtonStyle is set to a value that permits text to
    be displayed, the text defined held in this property appears as a
    label in the relevant tool button.

    It also serves as the default text in menus and tooltips if the action
    has not been defined with setText() or setToolTip(), and will
    also be used in toolbar buttons if no icon has been defined using setIcon().

    If the icon text is not explicitly set, the action's normal text will be
    used for the icon text.

    By default, this property contains an empty string.

    \sa setToolTip(), setStatusTip()
*/
void QAction::setIconText(const QString &text)
{
    QActionPrivate * const d= d_func();
    if (d->iconText == text)
        return;

    d->iconText = text;
    d->sendDataChanged();
}

QString QAction::iconText() const
{
    QActionPrivate * const d= d_func();
    if (d->iconText.isEmpty())
        return qt_strippedText(d->text);
    return d->iconText;
}

/*!
    \property QAction::toolTip
    \brief the action's tooltip

    This text is used for the tooltip. If no tooltip is specified,
    the action's text is used.

    By default, this property contains the action's text.

    \sa setStatusTip(), setShortcut()
*/
void QAction::setToolTip(const QString &tooltip)
{
    QActionPrivate * const d= d_func();
    if (d->tooltip == tooltip)
        return;

    d->tooltip = tooltip;
    d->sendDataChanged();
}

QString QAction::toolTip() const
{
    QActionPrivate * const d= d_func();
    if (d->tooltip.isEmpty()) {
        if (!d->text.isEmpty())
            return qt_strippedText(d->text);
        return qt_strippedText(d->iconText);
    }
    return d->tooltip;
}

/*!
    \property QAction::statusTip
    \brief the action's status tip

    The status tip is displayed on all status bars provided by the
    action's top-level parent widget.

    By default, this property contains an empty string.

    \sa setToolTip(), showStatusText()
*/
void QAction::setStatusTip(const QString &statustip)
{
    QActionPrivate * const d= d_func();
    if (d->statustip == statustip)
        return;

    d->statustip = statustip;
    d->sendDataChanged();
}

QString QAction::statusTip() const
{
    QActionPrivate * const d= d_func();
    return d->statustip;
}

/*!
    \property QAction::whatsThis
    \brief the action's "What's This?" help text

    The "What's This?" text is used to provide a brief description of
    the action. The text may contain rich text. There is no default
    "What's This?" text.

    \sa QWhatsThis
*/
void QAction::setWhatsThis(const QString &whatsthis)
{
    QActionPrivate * const d= d_func();
    if (d->whatsthis == whatsthis)
        return;

    d->whatsthis = whatsthis;
    d->sendDataChanged();
}

QString QAction::whatsThis() const
{
    QActionPrivate * const d= d_func();
    return d->whatsthis;
}

/*!
    \enum QAction::Priority
    \since 4.6

    This enum defines priorities for actions in user interface.

    \value LowPriority The action should not be prioritized in
    the user interface.

    \value NormalPriority

    \value HighPriority The action should be prioritized in
    the user interface.

    \sa priority
*/


/*!
    \property QAction::priority
    \since 4.6

    \brief the actions's priority in the user interface.

    This property can be set to indicate how the action should be prioritized
    in the user interface.

    For instance, when toolbars have the Qt::ToolButtonTextBesideIcon
    mode set, then actions with LowPriority will not show the text
    labels.
*/
void QAction::setPriority(Priority priority)
{
    QActionPrivate * const d= d_func();
    if (d->priority == priority)
        return;

    d->priority = priority;
    d->sendDataChanged();
}

QAction::Priority QAction::priority() const
{
    QActionPrivate * const d= d_func();
    return d->priority;
}

/*!
    \property QAction::checkable
    \brief whether the action is a checkable action

    A checkable action is one which has an on/off state. For example,
    in a word processor, a Bold toolbar button may be either on or
    off. An action which is not a toggle action is a command action;
    a command action is simply executed, e.g. file save.
    By default, this property is \c false.

    In some situations, the state of one toggle action should depend
    on the state of others. For example, "Left Align", "Center" and
    "Right Align" toggle actions are mutually exclusive. To achieve
    exclusive toggling, add the relevant toggle actions to a
    QActionGroup with the QActionGroup::exclusive property set to
    true.

    \sa QAction::setChecked()
*/
void QAction::setCheckable(bool b)
{
    QActionPrivate * const d= d_func();
    if (d->checkable == b)
        return;

    d->checkable = b;
    d->checked = false;
    d->sendDataChanged();
}

bool QAction::isCheckable() const
{
    QActionPrivate * const d= d_func();
    return d->checkable;
}

/*!
    \fn void QAction::toggle()

    This is a convenience function for the \l checked property.
    Connect to it to change the checked state to its opposite state.
*/
void QAction::toggle()
{
    QActionPrivate * const d= d_func();
    setChecked(!d->checked);
}

/*!
    \property QAction::checked
    \brief whether the action is checked.

    Only checkable actions can be checked.  By default, this is false
    (the action is unchecked).

    \sa checkable
*/
void QAction::setChecked(bool b)
{
    QActionPrivate * const d= d_func();
    if (!d->checkable || d->checked == b)
        return;

    QPointer<QAction> guard(this);
    d->checked = b;
    d->sendDataChanged();
    if (guard)
        emit toggled(b);
}

bool QAction::isChecked() const
{
    QActionPrivate * const d= d_func();
    return d->checked;
}

/*!
    \fn void QAction::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action is disabled; otherwise it is enabled.
*/

/*!
    \property QAction::enabled
    \brief whether the action is enabled

    Disabled actions cannot be chosen by the user. They do not
    disappear from menus or toolbars, but they are displayed in a way
    which indicates that they are unavailable. For example, they might
    be displayed using only shades of gray.

    \uicontrol{What's This?} help on disabled actions is still available, provided
    that the QAction::whatsThis property is set.

    An action will be disabled when all widgets to which it is added
    (with QWidget::addAction()) are disabled or not visible. When an
    action is disabled, it is not possible to trigger it through its
    shortcut.

    By default, this property is \c true (actions are enabled).

    \sa text
*/
void QAction::setEnabled(bool b)
{
    QActionPrivate * const d= d_func();
    if (b == d->enabled && b != d->forceDisabled)
        return;
    d->forceDisabled = !b;
    if (b && (!d->visible || (d->group && !d->group->isEnabled())))
        return;
    QAPP_CHECK("setEnabled");
    d->enabled = b;
#ifndef QT_NO_SHORTCUT
    d->setShortcutEnabled(b, qApp->d_func()->shortcutMap);
#endif
    d->sendDataChanged();
}

bool QAction::isEnabled() const
{
    QActionPrivate * const d= d_func();
    return d->enabled;
}

/*!
    \property QAction::visible
    \brief whether the action can be seen (e.g. in menus and toolbars)

    If \e visible is true the action can be seen (e.g. in menus and
    toolbars) and chosen by the user; if \e visible is false the
    action cannot be seen or chosen by the user.

    Actions which are not visible are \e not grayed out; they do not
    appear at all.

    By default, this property is \c true (actions are visible).
*/
void QAction::setVisible(bool b)
{
    QActionPrivate * const d= d_func();
    if (b == d->visible && b != d->forceInvisible)
        return;
    QAPP_CHECK("setVisible");
    d->forceInvisible = !b;
    d->visible = b;
    d->enabled = b && !d->forceDisabled && (!d->group || d->group->isEnabled()) ;
#ifndef QT_NO_SHORTCUT
    d->setShortcutEnabled(d->enabled, qApp->d_func()->shortcutMap);
#endif
    d->sendDataChanged();
}


bool QAction::isVisible() const
{
    QActionPrivate * const d= d_func();
    return d->visible;
}

/*!
  \reimp
*/
bool
QAction::event(QEvent *e)
{
#ifndef QT_NO_SHORTCUT
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        Q_ASSERT_X(se->key() == d_func()->shortcut || d_func()->alternateShortcuts.contains(se->key()),
                   "QAction::event",
                   "Received shortcut event from incorrect shortcut");
        if (se->isAmbiguous())
            qWarning("QAction::eventFilter: Ambiguous shortcut overload: %s", se->key().toString(QKeySequence::NativeText).toLatin1().constData());
        else
            activate(Trigger);
        return true;
    }
#endif
    return QObject::event(e);
}

/*!
  Returns the user data as set in QAction::setData.

  \sa setData()
*/
QVariant
QAction::data() const
{
    QActionPrivate * const d= d_func();
    return d->userData;
}

/*!
  \fn void QAction::setData(const QVariant &userData)

  Sets the action's internal data to the given \a userData.

  \sa data()
*/
void
QAction::setData(const QVariant &data)
{
    QActionPrivate * const d= d_func();
    if (d->userData == data)
        return;
    d->userData = data;
    d->sendDataChanged();
}


/*!
  Updates the relevant status bar for the \a widget specified by sending a
  QStatusTipEvent to its parent widget. Returns \c true if an event was sent;
  otherwise returns \c false.

  If a null widget is specified, the event is sent to the action's parent.

  \sa statusTip
*/
bool
QAction::showStatusText(QWidget *widget)
{
    return d_func()->showStatusText(widget, statusTip());
}

/*!
  Sends the relevant signals for ActionEvent \a event.

  Action based widgets use this API to cause the QAction
  to emit signals as well as emitting their own.
*/
void QAction::activate(ActionEvent event)
{
    QActionPrivate * const d= d_func();
    if(event == Trigger) {
        QPointer<QObject> guard = this;
        if(d->checkable) {
            // the checked action of an exclusive group cannot be  unchecked
            if (d->checked && (d->group && d->group->isExclusive()
                               && d->group->checkedAction() == this)) {
                if (!guard.isNull())
                    emit triggered(true);
                return;
            }
            setChecked(!d->checked);
        }
        if (!guard.isNull())
            emit triggered(d->checked);
    } else if(event == Hover) {
        emit hovered();
    }
}


void QAction::setMenuRole(MenuRole menuRole)
{
    QActionPrivate * const d= d_func();
    if (d->menuRole == menuRole)
        return;

    d->menuRole = menuRole;
    d->sendDataChanged();
}

QAction::MenuRole QAction::menuRole() const
{
    QActionPrivate * const d= d_func();
    return d->menuRole;
}

void QAction::setIconVisibleInMenu(bool visible)
{
    QActionPrivate * const d= d_func();
    if (d->iconVisibleInMenu == -1 || visible != bool(d->iconVisibleInMenu)) {
        int oldValue = d->iconVisibleInMenu;
        d->iconVisibleInMenu = visible;
        // Only send data changed if we really need to.
        if (oldValue != -1
            || (oldValue == -1
                && visible == !QApplication::instance()->testAttribute(Qt::AA_DontShowIconsInMenus))) {
            d->sendDataChanged();
        }
    }
}

bool QAction::isIconVisibleInMenu() const
{
    QActionPrivate * const d= d_func();
    if (d->iconVisibleInMenu == -1) {
        return !QApplication::instance()->testAttribute(Qt::AA_DontShowIconsInMenus);
    }
    return d->iconVisibleInMenu;
}

#include "moc_qaction.cpp"


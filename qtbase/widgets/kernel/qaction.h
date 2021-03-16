#ifndef QACTION_H
#define QACTION_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qkeysequence.h>
#include <QtCore/qstring.h>
#include <QtWidgets/qwidget.h>
#include <QtCore/qvariant.h>
#include <QtGui/qicon.h>

class QMenu;
class QActionGroup;
class QActionPrivate;
class QGraphicsWidget;

class QAction : public QObject
{
    //Q_OBJECT
//    Q_DECLARE_PRIVATE(QAction)
//
//    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY changed)
//    Q_PROPERTY(bool checked READ isChecked WRITE setChecked DESIGNABLE isCheckable NOTIFY toggled)
//    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY changed)
//    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY changed)
//    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed)
//    Q_PROPERTY(QString iconText READ iconText WRITE setIconText NOTIFY changed)
//    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip NOTIFY changed)
//    Q_PROPERTY(QString statusTip READ statusTip WRITE setStatusTip NOTIFY changed)
//    Q_PROPERTY(QString whatsThis READ whatsThis WRITE setWhatsThis NOTIFY changed)
//    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY changed)
//#ifndef QT_NO_SHORTCUT
//    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut NOTIFY changed)
//    Q_PROPERTY(Qt::ShortcutContext shortcutContext READ shortcutContext WRITE setShortcutContext NOTIFY changed)
//    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat NOTIFY changed)
//#endif
//    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY changed)
//    Q_PROPERTY(MenuRole menuRole READ menuRole WRITE setMenuRole NOTIFY changed)
//    Q_PROPERTY(bool iconVisibleInMenu READ isIconVisibleInMenu WRITE setIconVisibleInMenu NOTIFY changed)
//    Q_PROPERTY(Priority priority READ priority WRITE setPriority)



public:
    // note this is copied into qplatformmenu.h, which must stay in sync
    enum MenuRole { NoRole = 0, TextHeuristicRole, ApplicationSpecificRole, AboutQtRole,
                    AboutRole, PreferencesRole, QuitRole };

    enum Priority { LowPriority = 0, NormalPriority = 128, HighPriority = 256};

    explicit QAction(QObject *parent = nullptr);
    explicit QAction(const QString &text, QObject *parent = nullptr);
    explicit QAction(const QIcon &icon, const QString &text, QObject *parent = nullptr);

    ~QAction();

    void setActionGroup(QActionGroup *group);
    QActionGroup *actionGroup() const;
    void setIcon(const QIcon &icon);
    QIcon icon() const;

    void setText(const QString &text);
    QString text() const;

    void setIconText(const QString &text);
    QString iconText() const;

    void setToolTip(const QString &tip);
    QString toolTip() const;

    void setStatusTip(const QString &statusTip);
    QString statusTip() const;

    void setWhatsThis(const QString &what);
    QString whatsThis() const;

    void setPriority(Priority priority);
    Priority priority() const;

#if QT_CONFIG(menu)
    QMenu *menu() const;
    void setMenu(QMenu *menu);
#endif

    void setSeparator(bool b);
    bool isSeparator() const;

#ifndef QT_NO_SHORTCUT
    void setShortcut(const QKeySequence &shortcut);
    QKeySequence shortcut() const;

    void setShortcuts(const QList<QKeySequence> &shortcuts);
    void setShortcuts(QKeySequence::StandardKey);
    QList<QKeySequence> shortcuts() const;

    void setShortcutContext(Qt::ShortcutContext context);
    Qt::ShortcutContext shortcutContext() const;

    void setAutoRepeat(bool);
    bool autoRepeat() const;
#endif

    void setFont(const QFont &font);
    QFont font() const;

    void setCheckable(bool);
    bool isCheckable() const;

    QVariant data() const;
    void setData(const QVariant &var);

    bool isChecked() const;

    bool isEnabled() const;

    bool isVisible() const;

    enum ActionEvent { Trigger, Hover };
    void activate(ActionEvent event);
    bool showStatusText(QWidget *widget = Q_NULLPTR);

    void setMenuRole(MenuRole menuRole);
    MenuRole menuRole() const;

    void setIconVisibleInMenu(bool visible);
    bool isIconVisibleInMenu() const;


    QWidget *parentWidget() const;

    QList<QWidget *> associatedWidgets() const;

protected:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    QAction(QActionPrivate &dd, QObject *parent);

public Q_SLOTS:
    void trigger() { activate(Trigger); }
    void hover() { activate(Hover); }
    void setChecked(bool);
    void toggle();
    void setEnabled(bool);
    inline void setDisabled(bool b) { setEnabled(!b); }
    void setVisible(bool);

Q_SIGNALS:
    void changed();
    void triggered(bool checked = false);
    void hovered();
    void toggled(bool);

private:
    //Q_DISABLE_COPY(QAction)

    friend class QGraphicsWidget;
    friend class QWidget;
    friend class QActionGroup;
    friend class QMenu;
    friend class QMenuPrivate;
    friend class QMenuBar;
    friend class QToolButton;
};

#endif // QACTION_H

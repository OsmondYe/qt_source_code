#ifndef QMENU_H
#define QMENU_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtWidgets/qaction.h>

QT_BEGIN_NAMESPACE

class QMenuPrivate;
class QStyleOptionMenuItem;
class QPlatformMenu;

class  QMenu : public QWidget
{
private:
//    Q_OBJECT
//    Q_DECLARE_PRIVATE(QMenu)
	inline QMenuPrivate* d_func() { return reinterpret_cast<QWidgetPrivate *>(qGetPtrHelper(d_ptr)); }

//
//    Q_PROPERTY(bool tearOffEnabled READ isTearOffEnabled WRITE setTearOffEnabled)
//    Q_PROPERTY(QString title READ title WRITE setTitle)
//    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
//    Q_PROPERTY(bool separatorsCollapsible READ separatorsCollapsible WRITE setSeparatorsCollapsible)
//    Q_PROPERTY(bool toolTipsVisible READ toolTipsVisible WRITE setToolTipsVisible)

public:
    explicit QMenu(QWidget *parent = Q_NULLPTR);
    explicit QMenu(const QString &title, QWidget *parent = Q_NULLPTR);
    ~QMenu();

    using QWidget::addAction;
    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member, 
							const QKeySequence &shortcut = 0);
    QAction *addAction(const QIcon &icon, const QString &text, 
					const QObject *receiver, const char* member, const QKeySequence &shortcut = 0);

    // addAction(QString): Connect to a QObject slot / functor or function pointer (with context)
    template<class Obj, typename Func1>
    inline typename std::enable_if<!std::is_same<const char*, Func1>::value
        && QtPrivate::IsPointerToTypeDerivedFromQObject<Obj*>::Value, QAction *>::type
        addAction(const QString &text, const Obj *object, Func1 slot, const QKeySequence &shortcut = 0)
    {
        QAction *result = addAction(text);
        result->setShortcut(shortcut);
        connect(result, &QAction::triggered, object, slot);
        return result;
    }
    // addAction(QString): Connect to a functor or function pointer (without context)
    template <typename Func1>
    inline QAction *addAction(const QString &text, Func1 slot, const QKeySequence &shortcut = 0)
    {
        QAction *result = addAction(text);
        result->setShortcut(shortcut);
        connect(result, &QAction::triggered, slot);
        return result;
    }
    // addAction(QIcon, QString): Connect to a QObject slot / functor or function pointer (with context)
    template<class Obj, typename Func1>
    inline typename std::enable_if<!std::is_same<const char*, Func1>::value
        && QtPrivate::IsPointerToTypeDerivedFromQObject<Obj*>::Value, QAction *>::type
        addAction(const QIcon &actionIcon, const QString &text, const Obj *object, Func1 slot, const QKeySequence &shortcut = 0)
    {
        QAction *result = addAction(actionIcon, text);
        result->setShortcut(shortcut);
        connect(result, &QAction::triggered, object, slot);
        return result;
    }
    // addAction(QIcon, QString): Connect to a functor or function pointer (without context)
    template <typename Func1>
    inline QAction *addAction(const QIcon &actionIcon, const QString &text, Func1 slot, const QKeySequence &shortcut = 0)
    {
        QAction *result = addAction(actionIcon, text);
        result->setShortcut(shortcut);
        connect(result, &QAction::triggered, slot);
        return result;
    }
    QAction *addMenu(QMenu *menu);
    QMenu *addMenu(const QString &title);
    QMenu *addMenu(const QIcon &icon, const QString &title);

    QAction *addSeparator();

    QAction *addSection(const QString &text);
    QAction *addSection(const QIcon &icon, const QString &text);

    QAction *insertMenu(QAction *before, QMenu *menu);
    QAction *insertSeparator(QAction *before);
    QAction *insertSection(QAction *before, const QString &text);
    QAction *insertSection(QAction *before, const QIcon &icon, const QString &text);

    bool isEmpty() const;
    void clear();

    void setTearOffEnabled(bool);
    bool isTearOffEnabled() const;

    bool isTearOffMenuVisible() const;
    void showTearOffMenu();
    void showTearOffMenu(const QPoint &pos);
    void hideTearOffMenu();

    void setDefaultAction(QAction *);
    QAction *defaultAction() const;

    void setActiveAction(QAction *act);
    QAction *activeAction() const;

    void popup(const QPoint &pos, QAction *at = Q_NULLPTR);
    QAction *exec();
    QAction *exec(const QPoint &pos, QAction *at = Q_NULLPTR);

    static QAction *exec(QList<QAction*> actions, const QPoint &pos, QAction *at = Q_NULLPTR, QWidget *parent = Q_NULLPTR);
	
    QSize sizeHint() const ovrride;

    QRect actionGeometry(QAction *) const;
    QAction *actionAt(const QPoint &) const;

    QAction *menuAction() const;

    QString title() const;
    void setTitle(const QString &title);

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    void setNoReplayFor(QWidget *widget);
    QPlatformMenu *platformMenu();
    void setPlatformMenu(QPlatformMenu *platformMenu);

    bool separatorsCollapsible() const;
    void setSeparatorsCollapsible(bool collapse);

    bool toolTipsVisible() const;
    void setToolTipsVisible(bool visible);

Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();
    void triggered(QAction *action);
    void hovered(QAction *action);

protected:
    int columnCount() const;

    void changeEvent(QEvent *) ovrride;
    void keyPressEvent(QKeyEvent *) ovrride;
    void mouseReleaseEvent(QMouseEvent *) ovrride;
    void mousePressEvent(QMouseEvent *) ovrride;
    void mouseMoveEvent(QMouseEvent *) ovrride;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *) ovrride;
#endif
    void enterEvent(QEvent *) ovrride;
    void leaveEvent(QEvent *) ovrride;
    void hideEvent(QHideEvent *) ovrride;			// 在其内,结束eventloop
    void paintEvent(QPaintEvent *) ovrride;
    void actionEvent(QActionEvent *) ovrride;
    void timerEvent(QTimerEvent *) ovrride;
    bool event(QEvent *) ovrride;
    bool focusNextPrevChild(bool next) ovrride;
    void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const;

private Q_SLOTS:
    void internalDelayedPopup();

private:
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
    Q_PRIVATE_SLOT(d_func(), void _q_overrideMenuActionDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_platformMenuAboutToShow())

protected:
    QMenu(QMenuPrivate &dd, QWidget* parent = Q_NULLPTR);

private:
    Q_DISABLE_COPY(QMenu)

    friend class QMenuBar;
    friend class QMenuBarPrivate;
    friend class QTornOffMenu;
    friend class QComboBox;
    friend class QAction;
    friend class QToolButtonPrivate;
    friend void qt_mac_emit_menuSignals(QMenu *menu, bool show);
    friend void qt_mac_menu_emit_hovered(QMenu *menu, QAction *action);
};


QT_END_NAMESPACE

#endif // QMENU_H

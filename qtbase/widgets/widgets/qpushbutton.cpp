#include "qapplication.h"
#include "qbitmap.h"
#include "qdesktopwidget.h"
#include <private/qdialog_p.h>
#include "qdrawutil.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qstylepainter.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qpushbutton.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtoolbar.h"
#include "qdebug.h"
#include "qlayoutitem.h"
#include "qdialogbuttonbox.h"



#if QT_CONFIG(menu)
#include "qmenu.h"
#include "private/qmenu_p.h"
#endif
#include "private/qpushbutton_p.h"

QT_BEGIN_NAMESPACE



QPushButton::QPushButton(QWidget *parent)
    : QAbstractButton(*new QPushButtonPrivate, parent)
{
    QPushButtonPrivate * const d = d_func();
    d->init();
}

QPushButton::QPushButton(const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setText(text);
}



QPushButton::QPushButton(const QIcon& icon, const QString &text, QWidget *parent)
    : QPushButton(*new QPushButtonPrivate, parent)
{
    setText(text);
    setIcon(icon);
}


QPushButton::QPushButton(QPushButtonPrivate &dd, QWidget *parent)
    : QAbstractButton(dd, parent)
{
    QPushButtonPrivate * const d = d_func();
    d->init();
}


QPushButton::~QPushButton()
{
}

QDialog *QPushButtonPrivate::dialogParent() const
{
    Q_Q(const QPushButton);
    const QWidget *p = q;
    while (p && !p->isWindow()) {
        p = p->parentWidget();
        if (const QDialog *dialog = qobject_cast<const QDialog *>(p))
            return const_cast<QDialog *>(dialog);
    }
    return 0;
}


void QPushButton::initStyleOption(QStyleOptionButton *option) const
{
    if (!option)
        return;

    QPushButtonPrivate * const d = d_func();
	
    option->initFrom(this);
    option->features = QStyleOptionButton::None;
    if (d->flat)
        option->features |= QStyleOptionButton::Flat;
    if (d->menu)
        option->features |= QStyleOptionButton::HasMenu;
    if (autoDefault())
        option->features |= QStyleOptionButton::AutoDefaultButton;
    if (d->defaultButton)
        option->features |= QStyleOptionButton::DefaultButton;
    if (d->down || d->menuOpen)
        option->state |= QStyle::State_Sunken;
    if (d->checked)
        option->state |= QStyle::State_On;
    if (!d->flat && !d->down)
        option->state |= QStyle::State_Raised;
    option->text = d->text;
    option->icon = d->icon;
    option->iconSize = iconSize();
}

void QPushButton::setAutoDefault(bool enable)
{
    QPushButtonPrivate * const d = d_func();
    uint state = enable ? QPushButtonPrivate::On : QPushButtonPrivate::Off;
    if (d->autoDefault != QPushButtonPrivate::Auto && d->autoDefault == state)
        return;
    d->autoDefault = state;
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

bool QPushButton::autoDefault() const
{
    QPushButtonPrivate * const d = d_func();
    if(d->autoDefault == QPushButtonPrivate::Auto)
        return ( d->dialogParent() != 0 );
    return d->autoDefault;
}

void QPushButton::setDefault(bool enable)
{
    QPushButtonPrivate * const d = d_func();
    if (d->defaultButton == enable)
        return;
    d->defaultButton = enable;
    if (d->defaultButton) {
        if (QDialog *dlg = d->dialogParent())
            dlg->d_func()->setMainDefault(this);
    }
    update();

}

bool QPushButton::isDefault() const
{
    QPushButtonPrivate * const d = d_func();
    return d->defaultButton;
}


QSize QPushButton::sizeHint() const
{
    QPushButtonPrivate * const d = d_func();
    if (d->sizeHint.isValid() && d->lastAutoDefault == autoDefault())
        return d->sizeHint;
    d->lastAutoDefault = autoDefault();
    ensurePolished();

	// oye size: Icon + Text + Menu

    int w = 0, h = 0;

    QStyleOptionButton opt;
    initStyleOption(&opt);

    // calculate contents size...
    bool showButtonBoxIcons = qobject_cast<QDialogButtonBox*>(parentWidget())
                          && style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons);

	// consider size of icon						  
    if (!icon().isNull() || showButtonBoxIcons) {
        int ih = opt.iconSize.height();
        int iw = opt.iconSize.width() + 4;
        w += iw;
        h = qMax(h, ih);
    }

	// 
    QString s(text());
    bool empty = s.isEmpty();
    if (empty)
        s = QStringLiteral("XXXX");
    QFontMetrics fm = fontMetrics();
	// calc the size of s
    QSize sz = fm.size(Qt::TextShowMnemonic, s);
    if(!empty || !w)
        w += sz.width();
    if(!empty || !h)
        h = qMax(h, sz.height());
	// oye update the style.rect, why? 
    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height

	// calc menu indicator
    if (menu())
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
    d->sizeHint = (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
                  expandedTo(QApplication::globalStrut()));
    return d->sizeHint;
}

/*!
    \reimp
 */
QSize QPushButton::minimumSizeHint() const
{
    return sizeHint();
}


/*!\reimp
*/
void QPushButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);
    p.drawControl(QStyle::CE_PushButton, option);
}


/*! \reimp */
void QPushButton::keyPressEvent(QKeyEvent *e)
{
    QPushButtonPrivate * const d = d_func();
    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (autoDefault() || d->defaultButton) {
            click();
            break;
        }
        // fall through
    default:
        QAbstractButton::keyPressEvent(e);
    }
}

/*!
    \reimp
*/
void QPushButton::focusInEvent(QFocusEvent *e)
{
    QPushButtonPrivate * const d = d_func();
    if (e->reason() != Qt::PopupFocusReason && autoDefault() && !d->defaultButton) {
        d->defaultButton = true;
#if QT_CONFIG(dialog)
        QDialog *dlg = qobject_cast<QDialog*>(window());
        if (dlg)
            dlg->d_func()->setDefault(this);
#endif
    }
    QAbstractButton::focusInEvent(e);
}

/*!
    \reimp
*/
void QPushButton::focusOutEvent(QFocusEvent *e)
{
    QPushButtonPrivate * const d = d_func();
    if (e->reason() != Qt::PopupFocusReason && autoDefault() && d->defaultButton) {
#if QT_CONFIG(dialog)
        QDialog *dlg = qobject_cast<QDialog*>(window());
        if (dlg)
            dlg->d_func()->setDefault(0);
        else
            d->defaultButton = false;
#endif
    }

    QAbstractButton::focusOutEvent(e);
#if QT_CONFIG(menu)
    if (d->menu && d->menu->isVisible())        // restore pressed status
        setDown(true);
#endif
}

#if QT_CONFIG(menu)
/*!
    Associates the popup menu \a menu with this push button. This
    turns the button into a menu button, which in some styles will
    produce a small triangle to the right of the button's text.

    Ownership of the menu is \e not transferred to the push button.

    \image fusion-pushbutton-menu.png Screenshot of a Fusion style push button with popup menu.
    A push button with popup menus shown in the \l{Qt Widget Gallery}
    {Fusion widget style}.

    \sa menu()
*/
void QPushButton::setMenu(QMenu* menu)
{
    QPushButtonPrivate * const d = d_func();
    if (menu == d->menu)
        return;

    if (menu && !d->menu) {
        connect(this, SIGNAL(pressed()), this, SLOT(_q_popupPressed()), Qt::UniqueConnection);
    }
    if (d->menu)
        removeAction(d->menu->menuAction());
    d->menu = menu;
    if (d->menu)
        addAction(d->menu->menuAction());

    d->resetLayoutItemMargins();
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

/*!
    Returns the button's associated popup menu or 0 if no popup menu
    has been set.

    \sa setMenu()
*/
QMenu* QPushButton::menu() const
{
    QPushButtonPrivate * const d = d_func();
    return d->menu;
}

/*!
    Shows (pops up) the associated popup menu. If there is no such
    menu, this function does nothing. This function does not return
    until the popup menu has been closed by the user.
*/
void QPushButton::showMenu()
{
    QPushButtonPrivate * const d = d_func();
    if (!d || !d->menu)
        return;
    setDown(true);
    d->_q_popupPressed();
}

void QPushButtonPrivate::_q_popupPressed()
{
    Q_Q(QPushButton);
    if (!down || !menu)
        return;

    menu->setNoReplayFor(q);

    QPoint menuPos = adjustedMenuPosition();

    QPointer<QPushButton> guard(q);
    QMenuPrivate::get(menu)->causedPopup.widget = guard;

    //Because of a delay in menu effects, we must keep track of the
    //menu visibility to avoid flicker on button release
    menuOpen = true;
    menu->exec(menuPos);
    if (guard) {
        menuOpen = false;
        q->setDown(false);
    }
}

QPoint QPushButtonPrivate::adjustedMenuPosition()
{
    Q_Q(QPushButton);

    bool horizontal = true;
#if !defined(QT_NO_TOOLBAR)
    QToolBar *tb = qobject_cast<QToolBar*>(parent);
    if (tb && tb->orientation() == Qt::Vertical)
        horizontal = false;
#endif

    QWidgetItem item(q);
    QRect rect = item.geometry();
    rect.setRect(rect.x() - q->x(), rect.y() - q->y(), rect.width(), rect.height());

    QSize menuSize = menu->sizeHint();
    QPoint globalPos = q->mapToGlobal(rect.topLeft());
    int x = globalPos.x();
    int y = globalPos.y();
    const QRect availableGeometry = QApplication::desktop()->availableGeometry(q);
    if (horizontal) {
        if (globalPos.y() + rect.height() + menuSize.height() <= availableGeometry.bottom()) {
            y += rect.height();
        } else if (globalPos.y() - menuSize.height() >= availableGeometry.y()) {
            y -= menuSize.height();
        }
        if (q->layoutDirection() == Qt::RightToLeft)
            x += rect.width() - menuSize.width();
    } else {
        if (globalPos.x() + rect.width() + menu->sizeHint().width() <= availableGeometry.right()) {
            x += rect.width();
        } else if (globalPos.x() - menuSize.width() >= availableGeometry.x()) {
            x -= menuSize.width();
        }
    }

    return QPoint(x,y);
}

#endif // QT_CONFIG(menu)

void QPushButtonPrivate::resetLayoutItemMargins()
{
    //Q_Q(QPushButton);
	QPushButton* const q;
    QStyleOptionButton opt;
    q->initStyleOption(&opt);
    setLayoutItemMargins(QStyle::SE_PushButtonLayoutItem, &opt);
}

void QPushButton::setFlat(bool flat)
{
    QPushButtonPrivate * const d = d_func();
    if (d->flat == flat)
        return;
    d->flat = flat;
    d->resetLayoutItemMargins();
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

bool QPushButton::isFlat() const
{
    QPushButtonPrivate * const d = d_func();
    return d->flat;
}

bool QPushButton::event(QEvent *e)
{
    QPushButtonPrivate * const d = d_func();
    if (e->type() == QEvent::ParentChange) {
        if (QDialog *dialog = d->dialogParent()) {
            if (d->defaultButton)
                dialog->d_func()->setMainDefault(this);
        }
    } else if (e->type() == QEvent::StyleChange) {
        d->resetLayoutItemMargins();
        updateGeometry();
    } else if (e->type() == QEvent::PolishRequest) {
        updateGeometry();
    }
	
    return QAbstractButton::event(e);
}

QT_END_NAMESPACE

#include "moc_qpushbutton.cpp"

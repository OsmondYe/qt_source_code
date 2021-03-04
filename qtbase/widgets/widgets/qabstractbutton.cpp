#include "private/qabstractbutton_p.h"

#if QT_CONFIG(itemviews)
#include "qabstractitemview.h"
#endif
#if QT_CONFIG(buttongroup)
#include "qbuttongroup.h"
#include "private/qbuttongroup_p.h"
#endif
#include "qabstractbutton_p.h"
#include "qevent.h"
#include "qpainter.h"
#include "qapplication.h"
#include "qstyle.h"
#include "qaction.h"


#include <algorithm>

QT_BEGIN_NAMESPACE

#define AUTO_REPEAT_DELAY  300
#define AUTO_REPEAT_INTERVAL 100

extern bool qt_tab_all_widgets();

QAbstractButtonPrivate::QAbstractButtonPrivate(QSizePolicy::ControlType type)
    :
    shortcutId(0),
		
    checkable(false), checked(false), autoRepeat(false), autoExclusive(false),
    down(false), blockRefresh(false), pressed(false),
    
    group(0),

    autoRepeatDelay(AUTO_REPEAT_DELAY),
    autoRepeatInterval(AUTO_REPEAT_INTERVAL),
    controlType(type)
{}

QList<QAbstractButton *>QAbstractButtonPrivate::queryButtonList() const
{
#if QT_CONFIG(buttongroup)
    if (group)
        return group->d_func()->buttonList;
#endif

    QList<QAbstractButton*>candidates = parent->findChildren<QAbstractButton *>();
    if (autoExclusive) {
        auto isNoMemberOfMyAutoExclusiveGroup = [](QAbstractButton *candidate) {
            return !candidate->autoExclusive()
#if QT_CONFIG(buttongroup)
                || candidate->group()
#endif
                ;
        };
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                        isNoMemberOfMyAutoExclusiveGroup),
                         candidates.end());
    }
    return candidates;
}

QAbstractButton *QAbstractButtonPrivate::queryCheckedButton() const
{
#if QT_CONFIG(buttongroup)
    if (group)
        return group->d_func()->checkedButton;
#endif

    Q_Q(const QAbstractButton);
    QList<QAbstractButton *> buttonList = queryButtonList();
    if (!autoExclusive || buttonList.count() == 1) // no group
        return 0;

    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *b = buttonList.at(i);
        if (b->d_func()->checked && b != q)
            return b;
    }
    return checked  ? const_cast<QAbstractButton *>(q) : 0;
}

void QAbstractButtonPrivate::notifyChecked()
{
#if QT_CONFIG(buttongroup)
    QAbstractButton * const q = q_func();
    if (group) {
        QAbstractButton *previous = group->d_func()->checkedButton;
        group->d_func()->checkedButton = q;
        if (group->d_func()->exclusive && previous && previous != q)
            previous->nextCheckState();
    } else
#endif
    if (autoExclusive) {
        if (QAbstractButton *b = queryCheckedButton())
            b->setChecked(false);
    }
}

void QAbstractButtonPrivate::moveFocus(int key)
{
    QList<QAbstractButton *> buttonList = queryButtonList();;
#if QT_CONFIG(buttongroup)
    bool exclusive = group ? group->d_func()->exclusive : autoExclusive;
#else
    bool exclusive = autoExclusive;
#endif
    QWidget *f = QApplication::focusWidget();
    QAbstractButton *fb = qobject_cast<QAbstractButton *>(f);
    if (!fb || !buttonList.contains(fb))
        return;

    QAbstractButton *candidate = 0;
    int bestScore = -1;
    QRect target = f->rect().translated(f->mapToGlobal(QPoint(0,0)));
    QPoint goal = target.center();
    uint focus_flag = qt_tab_all_widgets() ? Qt::TabFocus : Qt::StrongFocus;

    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *button = buttonList.at(i);
        if (button != f && button->window() == f->window() && button->isEnabled() && !button->isHidden() &&
            (autoExclusive || (button->focusPolicy() & focus_flag) == focus_flag)) {
            QRect buttonRect = button->rect().translated(button->mapToGlobal(QPoint(0,0)));
            QPoint p = buttonRect.center();

            //Priority to widgets that overlap on the same coordinate.
            //In that case, the distance in the direction will be used as significant score,
            //take also in account orthogonal distance in case two widget are in the same distance.
            int score;
            if ((buttonRect.x() < target.right() && target.x() < buttonRect.right())
                  && (key == Qt::Key_Up || key == Qt::Key_Down)) {
                //one item's is at the vertical of the other
                score = (qAbs(p.y() - goal.y()) << 16) + qAbs(p.x() - goal.x());
            } else if ((buttonRect.y() < target.bottom() && target.y() < buttonRect.bottom())
                        && (key == Qt::Key_Left || key == Qt::Key_Right) ) {
                //one item's is at the horizontal of the other
                score = (qAbs(p.x() - goal.x()) << 16) + qAbs(p.y() - goal.y());
            } else {
                score = (1 << 30) + (p.y() - goal.y()) * (p.y() - goal.y()) + (p.x() - goal.x()) * (p.x() - goal.x());
            }

            if (score > bestScore && candidate)
                continue;

            switch(key) {
            case Qt::Key_Up:
                if (p.y() < goal.y()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Down:
                if (p.y() > goal.y()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Left:
                if (p.x() < goal.x()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Right:
                if (p.x() > goal.x()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            }
        }
    }

    if (exclusive
#ifdef QT_KEYPAD_NAVIGATION
        && !QApplication::keypadNavigationEnabled()
#endif
        && candidate
        && fb->d_func()->checked
        && candidate->d_func()->checkable)
        candidate->click();

    if (candidate) {
        if (key == Qt::Key_Up || key == Qt::Key_Left)
            candidate->setFocus(Qt::BacktabFocusReason);
        else
            candidate->setFocus(Qt::TabFocusReason);
    }
}

void QAbstractButtonPrivate::fixFocusPolicy()
{
    QAbstractButton * const q = q_func();
#if QT_CONFIG(buttongroup)
    if (!group && !autoExclusive)
#else
    if (!autoExclusive)
#endif
        return;

    QList<QAbstractButton *> buttonList = queryButtonList();
    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *b = buttonList.at(i);
        if (!b->isCheckable())
            continue;
        b->setFocusPolicy((Qt::FocusPolicy) ((b == q || !q->isCheckable())
                                         ? (b->focusPolicy() | Qt::TabFocus)
                                         :  (b->focusPolicy() & ~Qt::TabFocus)));
    }
}

void QAbstractButtonPrivate::init()
{
    QAbstractButton * const q = q_func();

    q->setFocusPolicy(Qt::FocusPolicy(q->style()->styleHint(QStyle::SH_Button_FocusPolicy)));
    q->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, controlType));
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    q->setForegroundRole(QPalette::ButtonText);
    q->setBackgroundRole(QPalette::Button);
}

void QAbstractButtonPrivate::refresh()
{
    QAbstractButton * const q = q_func();

    if (blockRefresh)
        return;
    q->update();
}

void QAbstractButtonPrivate::click()
{
    QAbstractButton * const q = q_func();

    down = false;
    blockRefresh = true;
    bool changeState = true;
    if (checked && queryCheckedButton() == q) {
        // the checked button of an exclusive or autoexclusive group cannot be unchecked
#if QT_CONFIG(buttongroup)
        if (group ? group->d_func()->exclusive : autoExclusive)
#else
        if (autoExclusive)
#endif
            changeState = false;
    }

    QPointer<QAbstractButton> guard(q);
    if (changeState) {
        q->nextCheckState();
        if (!guard)
            return;
    }
    blockRefresh = false;
    refresh();
    q->repaint();
    if (guard)
        emitReleased();
    if (guard)
        emitClicked();
}

void QAbstractButtonPrivate::emitClicked()
{
    QAbstractButton * const q = q_func();
    QPointer<QAbstractButton> guard(q);
    emit q->clicked(checked);
#if QT_CONFIG(buttongroup)
    if (guard && group) {
        emit group->buttonClicked(group->id(q));
        if (guard && group)
            emit group->buttonClicked(q);
    }
#endif
}

void QAbstractButtonPrivate::emitPressed()
{
    QAbstractButton * const q = q_func();
    QPointer<QAbstractButton> guard(q);
    emit q->pressed();
#if QT_CONFIG(buttongroup)
    if (guard && group) {
        emit group->buttonPressed(group->id(q));
        if (guard && group)
            emit group->buttonPressed(q);
    }
#endif
}

void QAbstractButtonPrivate::emitReleased()
{
    QAbstractButton * const q = q_func();
    QPointer<QAbstractButton> guard(q);
    emit q->released();
#if QT_CONFIG(buttongroup)
    if (guard && group) {
        emit group->buttonReleased(group->id(q));
        if (guard && group)
            emit group->buttonReleased(q);
    }
#endif
}

void QAbstractButtonPrivate::emitToggled(bool checked)
{
    QAbstractButton * const q = q_func();
    QPointer<QAbstractButton> guard(q);
    emit q->toggled(checked);
#if QT_CONFIG(buttongroup)
    if (guard && group) {
        emit group->buttonToggled(group->id(q), checked);
        if (guard && group)
            emit group->buttonToggled(q, checked);
    }
#endif
}


QAbstractButton::QAbstractButton(QWidget *parent)
    : QWidget(*new QAbstractButtonPrivate, parent, 0)
{
    QAbstractButtonPrivate * const d = d_func();
    d->init();
}


 QAbstractButton::~QAbstractButton()
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->group)
        d->group->removeButton(this);
}


/*! \internal
 */
QAbstractButton::QAbstractButton(QAbstractButtonPrivate &dd, QWidget *parent)
    : QWidget(dd, parent, 0)
{
    QAbstractButtonPrivate * const d = d_func();
    d->init();
}

void QAbstractButton::setText(const QString &text)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->text == text)
        return;
    d->text = text;
	// oye
    //QKeySequence newMnemonic = QKeySequence::mnemonic(text);
    //setShortcut(newMnemonic);
    
    d->sizeHint = QSize();
    update();
    updateGeometry();
	
	// oye
    //QAccessibleEvent event(this, QAccessible::NameChanged);
    //QAccessible::updateAccessibility(&event);

}

QString QAbstractButton::text() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->text;
}


void QAbstractButton::setIcon(const QIcon &icon)
{
    QAbstractButtonPrivate * const d = d_func();
    d->icon = icon;
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

QIcon QAbstractButton::icon() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->icon;
}



void QAbstractButton::setShortcut(const QKeySequence &key)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->shortcutId != 0)
        releaseShortcut(d->shortcutId);
    d->shortcut = key;
    d->shortcutId = grabShortcut(key);
}

QKeySequence QAbstractButton::shortcut() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->shortcut;
}


/*!
\property QAbstractButton::checkable
\brief whether the button is checkable

By default, the button is not checkable.

\sa checked
*/
void QAbstractButton::setCheckable(bool checkable)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->checkable == checkable)
        return;

    d->checkable = checkable;
    d->checked = false;
}

bool QAbstractButton::isCheckable() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->checkable;
}

/*!
\property QAbstractButton::checked
\brief whether the button is checked

Only checkable buttons can be checked. By default, the button is unchecked.

\sa checkable
*/
void QAbstractButton::setChecked(bool checked)
{
    QAbstractButtonPrivate * const d = d_func();
    if (!d->checkable || d->checked == checked) {
        if (!d->blockRefresh)
            checkStateSet();
        return;
    }

    if (!checked && d->queryCheckedButton() == this) {
        // the checked button of an exclusive or autoexclusive group cannot be  unchecked
#if QT_CONFIG(buttongroup)
        if (d->group ? d->group->d_func()->exclusive : d->autoExclusive)
            return;
        if (d->group)
            d->group->d_func()->detectCheckedButton();
#else
        if (d->autoExclusive)
            return;
#endif
    }

    QPointer<QAbstractButton> guard(this);

    d->checked = checked;
    if (!d->blockRefresh)
        checkStateSet();
    d->refresh();

    if (guard && checked)
        d->notifyChecked();
    if (guard)
        d->emitToggled(checked);

}

bool QAbstractButton::isChecked() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->checked;
}

/*!
  \property QAbstractButton::down
  \brief whether the button is pressed down

  If this property is \c true, the button is pressed down. The signals
  pressed() and clicked() are not emitted if you set this property
  to true. The default is false.
*/

void QAbstractButton::setDown(bool down)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->down == down)
        return;
    d->down = down;
    d->refresh();
    if (d->autoRepeat && d->down)
        d->repeatTimer.start(d->autoRepeatDelay, this);
    else
        d->repeatTimer.stop();
}

bool QAbstractButton::isDown() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->down;
}

/*!
\property QAbstractButton::autoRepeat
\brief whether autoRepeat is enabled

If autoRepeat is enabled, then the pressed(), released(), and clicked() signals are emitted at
regular intervals when the button is down. autoRepeat is off by default.
The initial delay and the repetition interval are defined in milliseconds by \l
autoRepeatDelay and \l autoRepeatInterval.

Note: If a button is pressed down by a shortcut key, then auto-repeat is enabled and timed by the
system and not by this class. The pressed(), released(), and clicked() signals will be emitted
like in the normal case.
*/

void QAbstractButton::setAutoRepeat(bool autoRepeat)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->autoRepeat == autoRepeat)
        return;
    d->autoRepeat = autoRepeat;
    if (d->autoRepeat && d->down)
        d->repeatTimer.start(d->autoRepeatDelay, this);
    else
        d->repeatTimer.stop();
}

bool QAbstractButton::autoRepeat() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->autoRepeat;
}

/*!
    \property QAbstractButton::autoRepeatDelay
    \brief the initial delay of auto-repetition
    \since 4.2

    If \l autoRepeat is enabled, then autoRepeatDelay defines the initial
    delay in milliseconds before auto-repetition kicks in.

    \sa autoRepeat, autoRepeatInterval
*/

void QAbstractButton::setAutoRepeatDelay(int autoRepeatDelay)
{
    QAbstractButtonPrivate * const d = d_func();
    d->autoRepeatDelay = autoRepeatDelay;
}

int QAbstractButton::autoRepeatDelay() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->autoRepeatDelay;
}

/*!
    \property QAbstractButton::autoRepeatInterval
    \brief the interval of auto-repetition
    \since 4.2

    If \l autoRepeat is enabled, then autoRepeatInterval defines the
    length of the auto-repetition interval in millisecons.

    \sa autoRepeat, autoRepeatDelay
*/

void QAbstractButton::setAutoRepeatInterval(int autoRepeatInterval)
{
    QAbstractButtonPrivate * const d = d_func();
    d->autoRepeatInterval = autoRepeatInterval;
}

int QAbstractButton::autoRepeatInterval() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->autoRepeatInterval;
}



/*!
\property QAbstractButton::autoExclusive
\brief whether auto-exclusivity is enabled

If auto-exclusivity is enabled, checkable buttons that belong to the
same parent widget behave as if they were part of the same
exclusive button group. In an exclusive button group, only one button
can be checked at any time; checking another button automatically
unchecks the previously checked one.

The property has no effect on buttons that belong to a button
group.

autoExclusive is off by default, except for radio buttons.

\sa QRadioButton
*/
void QAbstractButton::setAutoExclusive(bool autoExclusive)
{
    QAbstractButtonPrivate * const d = d_func();
    d->autoExclusive = autoExclusive;
}

bool QAbstractButton::autoExclusive() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->autoExclusive;
}

#if QT_CONFIG(buttongroup)
/*!
  Returns the group that this button belongs to.

  If the button is not a member of any QButtonGroup, this function
  returns 0.

  \sa QButtonGroup
*/
QButtonGroup *QAbstractButton::group() const
{
    QAbstractButtonPrivate* const d = d_func();
    return d->group;
}
#endif // QT_CONFIG(buttongroup)

/*!
Performs an animated click: the button is pressed immediately, and
released \a msec milliseconds later (the default is 100 ms).

Calling this function again before the button is released resets
the release timer.

All signals associated with a click are emitted as appropriate.

This function does nothing if the button is \l{setEnabled()}{disabled.}

\sa click()
*/
void QAbstractButton::animateClick(int msec)
{
    if (!isEnabled())
        return;
    QAbstractButtonPrivate * const d = d_func();
    if (d->checkable && focusPolicy() & Qt::ClickFocus)
        setFocus();
    setDown(true);
    repaint();
    if (!d->animateTimer.isActive())
        d->emitPressed();
    d->animateTimer.start(msec, this);
}

/*!
Performs a click.

All the usual signals associated with a click are emitted as
appropriate. If the button is checkable, the state of the button is
toggled.

This function does nothing if the button is \l{setEnabled()}{disabled.}

\sa animateClick()
 */
void QAbstractButton::click()
{
    if (!isEnabled())
        return;
    QAbstractButtonPrivate * const d = d_func();
    QPointer<QAbstractButton> guard(this);
    d->down = true;
    d->emitPressed();
    if (guard) {
        d->down = false;
        nextCheckState();
        if (guard)
            d->emitReleased();
        if (guard)
            d->emitClicked();
    }
}

/*! \fn void QAbstractButton::toggle()

    Toggles the state of a checkable button.

     \sa checked
*/
void QAbstractButton::toggle()
{
    QAbstractButtonPrivate * const d = d_func();
    setChecked(!d->checked);
}


/*! This virtual handler is called when setChecked() is used,
unless it is called from within nextCheckState(). It allows
subclasses to reset their intermediate button states.

\sa nextCheckState()
 */
void QAbstractButton::checkStateSet()
{
}

/*! This virtual handler is called when a button is clicked. The
default implementation calls setChecked(!isChecked()) if the button
isCheckable(). It allows subclasses to implement intermediate button
states.

\sa checkStateSet()
*/
void QAbstractButton::nextCheckState()
{
    if (isCheckable())
        setChecked(!isChecked());
}

/*!
Returns \c true if \a pos is inside the clickable button rectangle;
otherwise returns \c false.

By default, the clickable area is the entire widget. Subclasses
may reimplement this function to provide support for clickable
areas of different shapes and sizes.
*/
bool QAbstractButton::hitButton(const QPoint &pos) const
{
    return rect().contains(pos);
}

/*! \reimp */
bool QAbstractButton::event(QEvent *e)
{
    // as opposed to other widgets, disabled buttons accept mouse
    // events. This avoids surprising click-through scenarios
    if (!isEnabled()) {
        switch(e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::HoverMove:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::ContextMenu:
#if QT_CONFIG(wheelevent)
        case QEvent::Wheel:
#endif
            return true;
        default:
            break;
        }
    }

#ifndef QT_NO_SHORTCUT
    if (e->type() == QEvent::Shortcut) {
        QAbstractButtonPrivate * const d = d_func();
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        if (d->shortcutId != se->shortcutId())
            return false;
        if (!se->isAmbiguous()) {
            if (!d->animateTimer.isActive())
                animateClick();
        } else {
            if (focusPolicy() != Qt::NoFocus)
                setFocus(Qt::ShortcutFocusReason);
            window()->setAttribute(Qt::WA_KeyboardFocusChange);
        }
        return true;
    }
#endif
    return QWidget::event(e);
}

/*! \reimp */
void QAbstractButton::mousePressEvent(QMouseEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    if (hitButton(e->pos())) {
        setDown(true);
        d->pressed = true;
        repaint();
        d->emitPressed();
        e->accept();
    } else {
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::mouseReleaseEvent(QMouseEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();

    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }

    d->pressed = false;

    if (!d->down) {
        // refresh is required by QMacStyle to resume the default button animation
        d->refresh();
        e->ignore();
        return;
    }

    if (hitButton(e->pos())) {
        d->repeatTimer.stop();
        d->click();
        e->accept();
    } else {
        setDown(false);
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::mouseMoveEvent(QMouseEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    if (!(e->buttons() & Qt::LeftButton) || !d->pressed) {
        e->ignore();
        return;
    }

    if (hitButton(e->pos()) != d->down) {
        setDown(!d->down);
        repaint();
        if (d->down)
            d->emitPressed();
        else
            d->emitReleased();
        e->accept();
    } else if (!hitButton(e->pos())) {
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::keyPressEvent(QKeyEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    bool next = true;
    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        e->ignore();
        break;
    case Qt::Key_Select:
    case Qt::Key_Space:
        if (!e->isAutoRepeat()) {
            setDown(true);
            repaint();
            d->emitPressed();
        }
        break;
    case Qt::Key_Up:
        next = false;
        Q_FALLTHROUGH();
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Down: {
#ifdef QT_KEYPAD_NAVIGATION
        if ((QApplication::keypadNavigationEnabled()
                && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right))
                || (!QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional
                || (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down))) {
            e->ignore();
            return;
        }
#endif
        QWidget *pw = parentWidget();
        if (d->autoExclusive
#if QT_CONFIG(buttongroup)
        || d->group
#endif
#if QT_CONFIG(itemviews)
        || (pw && qobject_cast<QAbstractItemView *>(pw->parentWidget()))
#endif
        ) {
            // ### Using qobject_cast to check if the parent is a viewport of
            // QAbstractItemView is a crude hack, and should be revisited and
            // cleaned up when fixing task 194373. It's here to ensure that we
            // keep compatibility outside QAbstractItemView.
            d->moveFocus(e->key());
            if (hasFocus()) // nothing happend, propagate
                e->ignore();
        } else {
            // Prefer parent widget, use this if parent is absent
            QWidget *w = pw ? pw : this;
            bool reverse = (w->layoutDirection() == Qt::RightToLeft);
            if ((e->key() == Qt::Key_Left && !reverse)
                || (e->key() == Qt::Key_Right && reverse)) {
                next = false;
            }
            focusNextPrevChild(next);
        }
        break;
    }
    default:
#ifndef QT_NO_SHORTCUT
        if (e->matches(QKeySequence::Cancel) && d->down) {
            setDown(false);
            repaint();
            d->emitReleased();
            return;
        }
#endif
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::keyReleaseEvent(QKeyEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();

    if (!e->isAutoRepeat())
        d->repeatTimer.stop();

    switch (e->key()) {
    case Qt::Key_Select:
    case Qt::Key_Space:
        if (!e->isAutoRepeat() && d->down)
            d->click();
        break;
    default:
        e->ignore();
    }
}

/*!\reimp
 */
void QAbstractButton::timerEvent(QTimerEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    if (e->timerId() == d->repeatTimer.timerId()) {
        d->repeatTimer.start(d->autoRepeatInterval, this);
        if (d->down) {
            QPointer<QAbstractButton> guard(this);
            nextCheckState();
            if (guard)
                d->emitReleased();
            if (guard)
                d->emitClicked();
            if (guard)
                d->emitPressed();
        }
    } else if (e->timerId() == d->animateTimer.timerId()) {
        d->animateTimer.stop();
        d->click();
    }
}

/*! \reimp */
void QAbstractButton::focusInEvent(QFocusEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled())
#endif
    d->fixFocusPolicy();
    QWidget::focusInEvent(e);
}

/*! \reimp */
void QAbstractButton::focusOutEvent(QFocusEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    if (e->reason() != Qt::PopupFocusReason && d->down) {
        d->down = false;
        d->emitReleased();
    }
    QWidget::focusOutEvent(e);
}

/*! \reimp */
void QAbstractButton::changeEvent(QEvent *e)
{
    QAbstractButtonPrivate * const d = d_func();
    switch (e->type()) {
    case QEvent::EnabledChange:
        if (!isEnabled() && d->down) {
            d->down = false;
            d->emitReleased();
        }
        break;
    default:
        d->sizeHint = QSize();
        break;
    }
    QWidget::changeEvent(e);
}


QSize QAbstractButton::iconSize() const
{
    QAbstractButtonPrivate* const d = d_func();
    if (d->iconSize.isValid())
        return d->iconSize;
    int e = style()->pixelMetric(QStyle::PM_ButtonIconSize, 0, this);
    return QSize(e, e);
}

void QAbstractButton::setIconSize(const QSize &size)
{
    QAbstractButtonPrivate * const d = d_func();
    if (d->iconSize == size)
        return;

    d->iconSize = size;
    d->sizeHint = QSize();
    updateGeometry();
    if (isVisible()) {
        update();
    }
}



QT_END_NAMESPACE

#include "moc_qabstractbutton.cpp"

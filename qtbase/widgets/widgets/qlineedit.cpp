#include "qlineedit.h"
#include "qlineedit_p.h"

#include "qaction.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qdrag.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qstylehints.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include "qpainter.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qstringlist.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtimer.h"
#include "qvalidator.h"
#include "qvariant.h"
#include "qvector.h"
#include "qdebug.h"
#if QT_CONFIG(textedit)
#include "qtextedit.h"
#include <private/qtextedit_p.h>
#endif
#include <private/qwidgettextcontrol_p.h>

#include "qabstractitemview.h"
#include "private/qstylesheetstyle_p.h"

#ifndef QT_NO_SHORTCUT
#include "private/qapplication_p.h"
#include "private/qshortcutmap_p.h"
#include "qkeysequence.h"

#define ACCEL_KEY(k) (!qApp->d_func()->shortcutMap.hasShortcutForKeySequence(k) ? \
                      QLatin1Char('\t') + QKeySequence(k).toString(QKeySequence::NativeText) : QString())

#endif

#include <limits.h>

void QLineEdit::initStyleOption(QStyleOptionFrame *option) const
{
    QLineEditPrivate * const d = d_func();
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = d->frame ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this)
                                 : 0;
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (d->control->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrame::None;
}

QLineEdit::QLineEdit(QWidget* parent)
    : QLineEdit(QString(), parent)
{
}


QLineEdit::QLineEdit(const QString& contents, QWidget* parent)
    : QWidget(*new QLineEditPrivate, parent, 0)
{
    QLineEditPrivate *  d = d_func();
    d->init(contents);
}

QString QLineEdit::text() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->text();
}

void QLineEdit::setText(const QString& text)
{
    QLineEditPrivate *  d = d_func();
    d->control->setText(text);
}


QString QLineEdit::placeholderText() const
{
    QLineEditPrivate * const d = d_func();
    return d->placeholderText;
}

void QLineEdit::setPlaceholderText(const QString& placeholderText)
{
    QLineEditPrivate *  d = d_func();
    if (d->placeholderText != placeholderText) {
        d->placeholderText = placeholderText;
        if (d->shouldShowPlaceholderText())
            update();
    }
}


QString QLineEdit::displayText() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->displayText();
}

int QLineEdit::maxLength() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->maxLength();
}

void QLineEdit::setMaxLength(int maxLength)
{
    QLineEditPrivate *  d = d_func();
    d->control->setMaxLength(maxLength);
}


bool QLineEdit::hasFrame() const
{
    QLineEditPrivate * const d = d_func();
    return d->frame;
}


#if QT_CONFIG(action)
void QLineEdit::addAction(QAction *action, ActionPosition position)
{
    QLineEditPrivate *  d = d_func();
    QWidget::addAction(action);
    d->addAction(action, 0, position);
}

QAction *QLineEdit::addAction(const QIcon &icon, ActionPosition position)
{
    QAction *result = new QAction(icon, QString(), this);
    addAction(result, position);
    return result;
}
#endif // QT_CONFIG(action)


static const char clearButtonActionNameC[] = "_q_qlineeditclearaction";

// oye action与命令模式
void QLineEdit::setClearButtonEnabled(bool enable)
{
#if QT_CONFIG(action)
    QLineEditPrivate *  d = d_func();
    if (enable == isClearButtonEnabled())
        return;
    if (enable) {
        QAction *clearAction = new QAction(d->clearButtonIcon(), QString(), this);
        clearAction->setEnabled(!isReadOnly());
        clearAction->setObjectName(QLatin1String(clearButtonActionNameC));
        d->addAction(clearAction, 0, QLineEdit::TrailingPosition, QLineEditPrivate::SideWidgetClearButton | QLineEditPrivate::SideWidgetFadeInWithText);
    } else {
        QAction *clearAction = findChild<QAction *>(QLatin1String(clearButtonActionNameC));
        Q_ASSERT(clearAction);
        d->removeAction(clearAction);
        delete clearAction;
    }
#endif // QT_CONFIG(action)
}

bool QLineEdit::isClearButtonEnabled() const
{
    return findChild<QAction *>(QLatin1String(clearButtonActionNameC));
}

void QLineEdit::setFrame(bool enable)
{
    QLineEditPrivate *  d = d_func();
    d->frame = enable;
    update();   // oye update会发送PaintEvent, 从而让widget重绘
    updateGeometry();
}


QLineEdit::EchoMode QLineEdit::echoMode() const
{
    QLineEditPrivate * const d = d_func();
    return (EchoMode) d->control->echoMode();
}

void QLineEdit::setEchoMode(EchoMode mode)
{
    QLineEditPrivate *  d = d_func();
    if (mode == (EchoMode)d->control->echoMode())
        return;
    Qt::InputMethodHints imHints = inputMethodHints();
    imHints.setFlag(Qt::ImhHiddenText, mode == Password || mode == NoEcho);
    imHints.setFlag(Qt::ImhNoAutoUppercase, mode != Normal);
    imHints.setFlag(Qt::ImhNoPredictiveText, mode != Normal);
    imHints.setFlag(Qt::ImhSensitiveData, mode != Normal);
    setInputMethodHints(imHints);
    d->control->setEchoMode(mode);
    update();
}


#ifndef QT_NO_VALIDATOR
const QValidator * QLineEdit::validator() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->validator();
}

void QLineEdit::setValidator(const QValidator *v)
{
    QLineEditPrivate *  d = d_func();
    d->control->setValidator(v);
}
#endif // QT_NO_VALIDATOR

#if QT_CONFIG(completer)
/*!
    \since 4.2

    Sets this line edit to provide auto completions from the completer, \a c.
    The completion mode is set using QCompleter::setCompletionMode().

    To use a QCompleter with a QValidator or QLineEdit::inputMask, you need to
    ensure that the model provided to QCompleter contains valid entries. You can
    use the QSortFilterProxyModel to ensure that the QCompleter's model contains
    only valid entries.

    If \a c == 0, setCompleter() removes the current completer, effectively
    disabling auto completion.

    \sa QCompleter
*/
void QLineEdit::setCompleter(QCompleter *c)
{
    QLineEditPrivate *  d = d_func();
    if (c == d->control->completer())
        return;
    if (d->control->completer()) {
        disconnect(d->control->completer(), 0, this, 0);
        d->control->completer()->setWidget(0);
        if (d->control->completer()->parent() == this)
            delete d->control->completer();
    }
    d->control->setCompleter(c);
    if (!c)
        return;
    if (c->widget() == 0)
        c->setWidget(this);
    if (hasFocus()) {
        QObject::connect(d->control->completer(), SIGNAL(activated(QString)),
                         this, SLOT(setText(QString)));
        QObject::connect(d->control->completer(), SIGNAL(highlighted(QString)),
                         this, SLOT(_q_completionHighlighted(QString)));
    }
}

/*!
    \since 4.2

    Returns the current QCompleter that provides completions.
*/
QCompleter *QLineEdit::completer() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->completer();
}

#endif // QT_CONFIG(completer)


QSize QLineEdit::sizeHint() const
{
    QLineEditPrivate * const d = d_func();
    ensurePolished();
    QFontMetrics fm(font());
    int h = qMax(fm.height(), 14) 
			+ 2*d->verticalMargin  // 顶和底都计算
            + d->topTextMargin + d->bottomTextMargin  // 本类自己定义的
            + d->topmargin + d->bottommargin;   	  // widget基类定义的
            
    int w = fm.width(QLatin1Char('x')) * 17     // 最少给17个字符
			+ 2*d->horizontalMargin
            + d->effectiveLeftTextMargin() + d->effectiveRightTextMargin()
            + d->leftmargin + d->rightmargin; // "some"
    QStyleOptionFrame opt;
    initStyleOption(&opt);
	// 根据类型QStyle::CT_LineEdit和已有的参数opt,并结合现在刚计算出来的size(w,h),
	// 根据style来给出 合适的size,(其至少又3个参数决定)
    return (style()->sizeFromContents(
								QStyle::CT_LineEdit, 
								&opt, 
								QSize(w, h).expandedTo(QApplication::globalStrut()), 
								this)
			);
}



QSize QLineEdit::minimumSizeHint() const
{
    QLineEditPrivate * const d = d_func();
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    int h = fm.height() + qMax(2*d->verticalMargin, fm.leading())
            + d->topTextMargin + d->bottomTextMargin
            + d->topmargin + d->bottommargin;
    int w = fm.maxWidth()
            + d->effectiveLeftTextMargin() + d->effectiveRightTextMargin()
            + d->leftmargin + d->rightmargin;
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                      expandedTo(QApplication::globalStrut()), this));
}


int QLineEdit::cursorPosition() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->cursorPosition();
}

void QLineEdit::setCursorPosition(int pos)
{
    QLineEditPrivate *  d = d_func();
    d->control->setCursorPosition(pos);
}


int QLineEdit::cursorPositionAt(const QPoint &pos)
{
    QLineEditPrivate *  d = d_func();
    return d->xToPos(pos.x());
}

// default: AlignLeft | AlignVCenter
Qt::Alignment QLineEdit::alignment() const
{
    QLineEditPrivate * const d = d_func();
    return QFlag(d->alignment);
}

void QLineEdit::setAlignment(Qt::Alignment alignment)
{
    QLineEditPrivate *  d = d_func();
    d->alignment = alignment;
    update();
}


/*!
    Moves the cursor forward \a steps characters. If \a mark is true
    each character moved over is added to the selection; if \a mark is
    false the selection is cleared.

    \sa cursorBackward()
*/

void QLineEdit::cursorForward(bool mark, int steps)
{
    QLineEditPrivate *  d = d_func();
    d->control->cursorForward(mark, steps);
}


/*!
    Moves the cursor back \a steps characters. If \a mark is true each
    character moved over is added to the selection; if \a mark is
    false the selection is cleared.

    \sa cursorForward()
*/
void QLineEdit::cursorBackward(bool mark, int steps)
{
    cursorForward(mark, -steps);
}

/*!
    Moves the cursor one word forward. If \a mark is true, the word is
    also selected.

    \sa cursorWordBackward()
*/
void QLineEdit::cursorWordForward(bool mark)
{
    QLineEditPrivate *  d = d_func();
    d->control->cursorWordForward(mark);
}

/*!
    Moves the cursor one word backward. If \a mark is true, the word
    is also selected.

    \sa cursorWordForward()
*/

void QLineEdit::cursorWordBackward(bool mark)
{
    QLineEditPrivate *  d = d_func();
    d->control->cursorWordBackward(mark);
}


/*!
    If no text is selected, deletes the character to the left of the
    text cursor and moves the cursor one position to the left. If any
    text is selected, the cursor is moved to the beginning of the
    selected text and the selected text is deleted.

    \sa del()
*/
void QLineEdit::backspace()
{
    QLineEditPrivate *  d = d_func();
    d->control->backspace();
}

/*!
    If no text is selected, deletes the character to the right of the
    text cursor. If any text is selected, the cursor is moved to the
    beginning of the selected text and the selected text is deleted.

    \sa backspace()
*/

void QLineEdit::del()
{
    QLineEditPrivate *  d = d_func();
    d->control->del();
}

/*!
    Moves the text cursor to the beginning of the line unless it is
    already there. If \a mark is true, text is selected towards the
    first position; otherwise, any selected text is unselected if the
    cursor is moved.

    \sa end()
*/

void QLineEdit::home(bool mark)
{
    QLineEditPrivate *  d = d_func();
    d->control->home(mark);
}

/*!
    Moves the text cursor to the end of the line unless it is already
    there. If \a mark is true, text is selected towards the last
    position; otherwise, any selected text is unselected if the cursor
    is moved.

    \sa home()
*/

void QLineEdit::end(bool mark)
{
    QLineEditPrivate *  d = d_func();
    d->control->end(mark);
}


/*!
    \property QLineEdit::modified
    \brief whether the line edit's contents has been modified by the user

    The modified flag is never read by QLineEdit; it has a default value
    of false and is changed to true whenever the user changes the line
    edit's contents.

    This is useful for things that need to provide a default value but
    do not start out knowing what the default should be (perhaps it
    depends on other fields on the form). Start the line edit without
    the best default, and when the default is known, if modified()
    returns \c false (the user hasn't entered any text), insert the
    default value.

    Calling setText() resets the modified flag to false.
*/

bool QLineEdit::isModified() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->isModified();
}

void QLineEdit::setModified(bool modified)
{
    QLineEditPrivate *  d = d_func();
    d->control->setModified(modified);
}

/*!
    \property QLineEdit::hasSelectedText
    \brief whether there is any text selected

    hasSelectedText() returns \c true if some or all of the text has been
    selected by the user; otherwise returns \c false.

    By default, this property is \c false.

    \sa selectedText()
*/


bool QLineEdit::hasSelectedText() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->hasSelectedText();
}

/*!
    \property QLineEdit::selectedText
    \brief the selected text

    If there is no selected text this property's value is
    an empty string.

    By default, this property contains an empty string.

    \sa hasSelectedText()
*/

QString QLineEdit::selectedText() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->selectedText();
}

/*!
    Returns the index of the first selected character in the
    line edit or -1 if no text is selected.

    \sa selectedText()
*/

int QLineEdit::selectionStart() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->selectionStart();
}




/*!
    Selects text from position \a start and for \a length characters.
    Negative lengths are allowed.

    \sa deselect(), selectAll(), selectedText()
*/

void QLineEdit::setSelection(int start, int length)
{
    QLineEditPrivate *  d = d_func();
    if (Q_UNLIKELY(start < 0 || start > (int)d->control->end())) {
        qWarning("QLineEdit::setSelection: Invalid start position (%d)", start);
        return;
    }

    d->control->setSelection(start, length);

    if (d->control->hasSelectedText()){
        QStyleOptionFrame opt;
        initStyleOption(&opt);
        if (!style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
            d->setCursorVisible(false);
    }
}


/*!
    \property QLineEdit::undoAvailable
    \brief whether undo is available

    Undo becomes available once the user has modified the text in the line edit.

    By default, this property is \c false.
*/

bool QLineEdit::isUndoAvailable() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->isUndoAvailable();
}

/*!
    \property QLineEdit::redoAvailable
    \brief whether redo is available

    Redo becomes available once the user has performed one or more undo operations
    on text in the line edit.

    By default, this property is \c false.
*/

bool QLineEdit::isRedoAvailable() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->isRedoAvailable();
}

/*!
    \property QLineEdit::dragEnabled
    \brief whether the lineedit starts a drag if the user presses and
    moves the mouse on some selected text

    Dragging is disabled by default.
*/

bool QLineEdit::dragEnabled() const
{
    QLineEditPrivate * const d = d_func();
    return d->dragEnabled;
}

void QLineEdit::setDragEnabled(bool b)
{
    QLineEditPrivate *  d = d_func();
    d->dragEnabled = b;
}

/*!
  \property QLineEdit::cursorMoveStyle
  \brief the movement style of cursor in this line edit
  \since 4.8

  When this property is set to Qt::VisualMoveStyle, the line edit will use visual
  movement style. Pressing the left arrow key will always cause the cursor to move
  left, regardless of the text's writing direction. The same behavior applies to
  right arrow key.

  When the property is Qt::LogicalMoveStyle (the default), within a LTR text block,
  increase cursor position when pressing left arrow key, decrease cursor position
  when pressing the right arrow key. If the text block is right to left, the opposite
  behavior applies.
*/

Qt::CursorMoveStyle QLineEdit::cursorMoveStyle() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->cursorMoveStyle();
}

void QLineEdit::setCursorMoveStyle(Qt::CursorMoveStyle style)
{
    QLineEditPrivate *  d = d_func();
    d->control->setCursorMoveStyle(style);
}

/*!
    \property QLineEdit::acceptableInput
    \brief whether the input satisfies the inputMask and the
    validator.

    By default, this property is \c true.

    \sa setInputMask(), setValidator()
*/
bool QLineEdit::hasAcceptableInput() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->hasAcceptableInput();
}

/*!
    Sets the margins around the text inside the frame to have the
    sizes \a left, \a top, \a right, and \a bottom.
    \since 4.5

    See also getTextMargins().
*/
void QLineEdit::setTextMargins(int left, int top, int right, int bottom)
{
    QLineEditPrivate *  d = d_func();
    d->leftTextMargin = left;
    d->topTextMargin = top;
    d->rightTextMargin = right;
    d->bottomTextMargin = bottom;
    updateGeometry();
    update();
}

/*!
    \since 4.6
    Sets the \a margins around the text inside the frame.

    See also textMargins().
*/
void QLineEdit::setTextMargins(const QMargins &margins)
{
    setTextMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}

/*!
    Returns the widget's text margins for \a left, \a top, \a right, and \a bottom.
    \since 4.5

    \sa setTextMargins()
*/
void QLineEdit::getTextMargins(int *left, int *top, int *right, int *bottom) const
{
    QLineEditPrivate * const d = d_func();
    if (left)
        *left = d->leftTextMargin;
    if (top)
        *top = d->topTextMargin;
    if (right)
        *right = d->rightTextMargin;
    if (bottom)
        *bottom = d->bottomTextMargin;
}

/*!
    \since 4.6
    Returns the widget's text margins.

    \sa setTextMargins()
*/
QMargins QLineEdit::textMargins() const
{
    QLineEditPrivate * const d = d_func();
    return QMargins(d->leftTextMargin, d->topTextMargin, d->rightTextMargin, d->bottomTextMargin);
}

QString QLineEdit::inputMask() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->inputMask();
}

void QLineEdit::setInputMask(const QString &inputMask)
{
    QLineEditPrivate *  d = d_func();
    d->control->setInputMask(inputMask);
}

void QLineEdit::selectAll()
{
    QLineEditPrivate *  d = d_func();
    d->control->selectAll();
}

void QLineEdit::deselect()
{
    QLineEditPrivate *  d = d_func();
    d->control->deselect();
}


/*!
    Deletes any selected text, inserts \a newText, and validates the
    result. If it is valid, it sets it as the new contents of the line
    edit.

    \sa setText(), clear()
*/
void QLineEdit::insert(const QString &newText)
{
//     q->resetInputContext(); //#### FIX ME IN QT
    QLineEditPrivate *  d = d_func();
    d->control->insert(newText);
}

/*!
    Clears the contents of the line edit.

    \sa setText(), insert()
*/
void QLineEdit::clear()
{
    QLineEditPrivate *  d = d_func();
    d->resetInputMethod();
    d->control->clear();
}

/*!
    Undoes the last operation if undo is \l{QLineEdit::undoAvailable}{available}. Deselects any current
    selection, and updates the selection start to the current cursor
    position.
*/
void QLineEdit::undo()
{
    QLineEditPrivate *  d = d_func();
    d->resetInputMethod();
    d->control->undo();
}

/*!
    Redoes the last operation if redo is \l{QLineEdit::redoAvailable}{available}.
*/
void QLineEdit::redo()
{
    QLineEditPrivate *  d = d_func();
    d->resetInputMethod();
    d->control->redo();
}


/*!
    \property QLineEdit::readOnly
    \brief whether the line edit is read only.

    In read-only mode, the user can still copy the text to the
    clipboard, or drag and drop the text (if echoMode() is \l Normal),
    but cannot edit it.

    QLineEdit does not show a cursor in read-only mode.

    By default, this property is \c false.

    \sa setEnabled()
*/

bool QLineEdit::isReadOnly() const
{
    QLineEditPrivate * const d = d_func();
    return d->control->isReadOnly();
}

void QLineEdit::setReadOnly(bool enable)
{
    QLineEditPrivate *  d = d_func();
    if (d->control->isReadOnly() != enable) {
        d->control->setReadOnly(enable);
        d->setClearButtonEnabled(!enable);
        setAttribute(Qt::WA_MacShowFocusRect, !enable);
        setAttribute(Qt::WA_InputMethodEnabled, d->shouldEnableInputMethod());
#ifndef QT_NO_CURSOR
        setCursor(enable ? Qt::ArrowCursor : Qt::IBeamCursor);
#endif
        QEvent event(QEvent::ReadOnlyChange);
        QCoreApplication::sendEvent(this, &event);
        update();
    }
}


#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it, if there
    is any, and if echoMode() is \l Normal.

    If the current validator disallows deleting the selected text,
    cut() will copy without deleting.

    \sa copy(), paste(), setValidator()
*/

void QLineEdit::cut()
{
    if (hasSelectedText()) {
        copy();
        del();
    }
}


/*!
    Copies the selected text to the clipboard, if there is any, and if
    echoMode() is \l Normal.

    \sa cut(), paste()
*/

void QLineEdit::copy() const
{
    QLineEditPrivate * const d = d_func();
    d->control->copy();
}

/*!
    Inserts the clipboard's text at the cursor position, deleting any
    selected text, providing the line edit is not \l{QLineEdit::readOnly}{read-only}.

    If the end result would not be acceptable to the current
    \l{setValidator()}{validator}, nothing happens.

    \sa copy(), cut()
*/

void QLineEdit::paste()
{
    QLineEditPrivate *  d = d_func();
    d->control->paste();
}

#endif // !QT_NO_CLIPBOARD

/*! \reimp
*/
bool QLineEdit::event(QEvent * e)
{
    QLineEditPrivate *  d = d_func();
    if (e->type() == QEvent::Timer) {
        int timerId = ((QTimerEvent*)e)->timerId();
        if (timerId == d->dndTimer.timerId()) {
            d->drag();
        }
        else if (timerId == d->tripleClickTimer.timerId())
            d->tripleClickTimer.stop();
    } else if (e->type() == QEvent::ContextMenu) {
        if (d->control->composeMode())
            return true;
    } else if (e->type() == QEvent::WindowActivate) {
        QTimer::singleShot(0, this, SLOT(_q_handleWindowActivate()));
#ifndef QT_NO_SHORTCUT
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        d->control->processShortcutOverrideEvent(ke);
#endif
    } else if (e->type() == QEvent::KeyRelease) {
        d->control->updateCursorBlinking();
    } else if (e->type() == QEvent::Show) {
        //In order to get the cursor blinking if QComboBox::setEditable is called when the combobox has focus
        if (hasFocus()) {
            d->control->setBlinkingCursorEnabled(true);
            QStyleOptionFrame opt;
            initStyleOption(&opt);
            if ((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
                || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
                d->setCursorVisible(true);
        }
#if QT_CONFIG(action)
    } else if (e->type() == QEvent::ActionRemoved) {
        d->removeAction(static_cast<QActionEvent *>(e)->action());
#endif
    } else if (e->type() == QEvent::Resize) {
        d->positionSideWidgets();
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled()) {
        if (e->type() == QEvent::EnterEditFocus) {
            end(false);
            d->setCursorVisible(true);
            d->control->setCursorBlinkEnabled(true);
        } else if (e->type() == QEvent::LeaveEditFocus) {
            d->setCursorVisible(false);
            d->control->setCursorBlinkEnabled(false);
            if (d->control->hasAcceptableInput() || d->control->fixup())
                emit editingFinished();
        }
    }
#endif
    return QWidget::event(e);
}

/*! \reimp
*/
void QLineEdit::mousePressEvent(QMouseEvent* e)
{
    QLineEditPrivate *  d = d_func();

    d->mousePressPos = e->pos();

    if (d->sendMouseEventToInputContext(e))
        return;
    if (e->button() == Qt::RightButton)
        return;
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
        setEditFocus(true);
        // Get the completion list to pop up.
        if (d->control->completer())
            d->control->completer()->complete();
    }
#endif
    if (d->tripleClickTimer.isActive() && (e->pos() - d->tripleClick).manhattanLength() <
         QApplication::startDragDistance()) {
        selectAll();
        return;
    }
    bool mark = e->modifiers() & Qt::ShiftModifier;
    int cursor = d->xToPos(e->pos().x());
#ifndef QT_NO_DRAGANDDROP
    if (!mark && d->dragEnabled && d->control->echoMode() == Normal &&
         e->button() == Qt::LeftButton && d->inSelection(e->pos().x())) {
        if (!d->dndTimer.isActive())
            d->dndTimer.start(QApplication::startDragTime(), this);
    } else
#endif
    {
        d->control->moveCursor(cursor, mark);
    }
}

/*! \reimp
*/
void QLineEdit::mouseMoveEvent(QMouseEvent * e)
{
    QLineEditPrivate *  d = d_func();

    if (e->buttons() & Qt::LeftButton) {
#ifndef QT_NO_DRAGANDDROP
        if (d->dndTimer.isActive()) {
            if ((d->mousePressPos - e->pos()).manhattanLength() > QApplication::startDragDistance())
                d->drag();
        } else
#endif
        {
            const bool select = (d->imHints & Qt::ImhNoPredictiveText);
#ifndef QT_NO_IM
            if (d->control->composeMode() && select) {
                int startPos = d->xToPos(d->mousePressPos.x());
                int currentPos = d->xToPos(e->pos().x());
                if (startPos != currentPos)
                    d->control->setSelection(startPos, currentPos - startPos);

            } else
#endif
            {
                d->control->moveCursor(d->xToPos(e->pos().x()), select);
            }
        }
    }

    d->sendMouseEventToInputContext(e);
}

/*! \reimp
*/
void QLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    QLineEditPrivate *  d = d_func();
    if (d->sendMouseEventToInputContext(e))
        return;
#ifndef QT_NO_DRAGANDDROP
    if (e->button() == Qt::LeftButton) {
        if (d->dndTimer.isActive()) {
            d->dndTimer.stop();
            deselect();
            return;
        }
    }
#endif
#ifndef QT_NO_CLIPBOARD
    if (QApplication::clipboard()->supportsSelection()) {
        if (e->button() == Qt::LeftButton) {
            d->control->copy(QClipboard::Selection);
        } else if (!d->control->isReadOnly() && e->button() == Qt::MidButton) {
            deselect();
            insert(QApplication::clipboard()->text(QClipboard::Selection));
        }
    }
#endif

    if (!isReadOnly() && rect().contains(e->pos()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*! \reimp
*/
void QLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
    QLineEditPrivate *  d = d_func();

    if (e->button() == Qt::LeftButton) {
        int position = d->xToPos(e->pos().x());

        // exit composition mode
#ifndef QT_NO_IM
        if (d->control->composeMode()) {
            int preeditPos = d->control->cursor();
            int posInPreedit = position - d->control->cursor();
            int preeditLength = d->control->preeditAreaText().length();
            bool positionOnPreedit = false;

            if (posInPreedit >= 0 && posInPreedit <= preeditLength)
                positionOnPreedit = true;

            int textLength = d->control->end();
            d->control->commitPreedit();
            int sizeChange = d->control->end() - textLength;

            if (positionOnPreedit) {
                if (sizeChange == 0)
                    position = -1; // cancel selection, word disappeared
                else
                    // ensure not selecting after preedit if event happened there
                    position = qBound(preeditPos, position, preeditPos + sizeChange);
            } else if (position > preeditPos) {
                // adjust positions after former preedit by how much text changed
                position += (sizeChange - preeditLength);
            }
        }
#endif

        if (position >= 0)
            d->control->selectWordAtPos(position);

        d->tripleClickTimer.start(QApplication::doubleClickInterval(), this);
        d->tripleClick = e->pos();
    } else {
        d->sendMouseEventToInputContext(e);
    }
}

void QLineEdit::keyPressEvent(QKeyEvent *event)
{
    QLineEditPrivate *  d = d_func();
    #ifdef QT_KEYPAD_NAVIGATION
    bool select = false;
    switch (event->key()) {
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled()) {
                if (hasEditFocus()) {
                    setEditFocus(false);
                    if (d->control->completer() && d->control->completer()->popup()->isVisible())
                        d->control->completer()->popup()->hide();
                    select = true;
                }
            }
            break;
        case Qt::Key_Back:
        case Qt::Key_No:
            if (!QApplication::keypadNavigationEnabled() || !hasEditFocus()) {
                event->ignore();
                return;
            }
            break;
        default:
            if (QApplication::keypadNavigationEnabled()) {
                if (!hasEditFocus() && !(event->modifiers() & Qt::ControlModifier)) {
                    if (!event->text().isEmpty() && event->text().at(0).isPrint()
                        && !isReadOnly())
                        setEditFocus(true);
                    else {
                        event->ignore();
                        return;
                    }
                }
            }
    }



    if (QApplication::keypadNavigationEnabled() && !select && !hasEditFocus()) {
        setEditFocus(true);
        if (event->key() == Qt::Key_Select)
            return; // Just start. No action.
    }
#endif
    d->control->processKeyEvent(event);
    if (event->isAccepted()) {
        if (layoutDirection() != d->control->layoutDirection())
            setLayoutDirection(d->control->layoutDirection());
        d->control->updateCursorBlinking();
    }
}


QRect QLineEdit::cursorRect() const
{
    QLineEditPrivate * const d = d_func();
    return d->cursorRect();
}

/*! \reimp
 */
void QLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    QLineEditPrivate *  d = d_func();
    if (d->control->isReadOnly()) {
        e->ignore();
        return;
    }

    if (echoMode() == PasswordEchoOnEdit && !d->control->passwordEchoEditing()) {
        // Clear the edit and reset to normal echo mode while entering input
        // method data; the echo mode switches back when the edit loses focus.
        // ### changes a public property, resets current content.
        d->updatePasswordEchoEditing(true);
        clear();
    }

#ifdef QT_KEYPAD_NAVIGATION
    // Focus in if currently in navigation focus on the widget
    // Only focus in on preedits, to allow input methods to
    // commit text as they focus out without interfering with focus
    if (QApplication::keypadNavigationEnabled()
        && hasFocus() && !hasEditFocus()
        && !e->preeditString().isEmpty())
        setEditFocus(true);
#endif

    d->control->processInputMethodEvent(e);

#if QT_CONFIG(completer)
    if (!e->commitString().isEmpty())
        d->control->complete(Qt::Key_unknown);
#endif
}

/*!\reimp
*/
QVariant QLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return inputMethodQuery(property, QVariant());
}

/*!\internal
*/
QVariant QLineEdit::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    QLineEditPrivate * const d = d_func();
    switch(property) {
    case Qt::ImCursorRectangle:
        return d->cursorRect();
    case Qt::ImAnchorRectangle:
        return d->adjustedControlRect(d->control->anchorRect());
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return QVariant(d->xToPos(pt.x(), QTextLine::CursorBetweenCharacters));
        return QVariant(d->control->cursor()); }
    case Qt::ImSurroundingText:
        return QVariant(d->control->surroundingText());
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->control->selectionStart() == d->control->selectionEnd())
            return QVariant(d->control->cursor());
        else if (d->control->selectionStart() == d->control->cursor())
            return QVariant(d->control->selectionEnd());
        else
            return QVariant(d->control->selectionStart());
    default:
        return QWidget::inputMethodQuery(property);
    }
}

/*!\reimp
*/

void QLineEdit::focusInEvent(QFocusEvent *e)
{
    QLineEditPrivate *  d = d_func();
    if (e->reason() == Qt::TabFocusReason ||
         e->reason() == Qt::BacktabFocusReason  ||
         e->reason() == Qt::ShortcutFocusReason) {
        if (!d->control->inputMask().isEmpty())
            d->control->moveCursor(d->control->nextMaskBlank(0));
        else if (!d->control->hasSelectedText())
            selectAll();
    } else if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled() || (hasEditFocus() && ( e->reason() == Qt::PopupFocusReason))) {
#endif
    d->control->setBlinkingCursorEnabled(true);
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    if((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
       || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
        d->setCursorVisible(true);
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (d->control->echoMode() == Password || d->control->echoMode() == NoEcho)
        qt_mac_secure_keyboard(true);
#endif
#ifdef QT_KEYPAD_NAVIGATION
        d->control->setCancelText(d->control->text());
    }
#endif
#if QT_CONFIG(completer)
    if (d->control->completer()) {
        d->control->completer()->setWidget(this);
        QObject::connect(d->control->completer(), SIGNAL(activated(QString)),
                         this, SLOT(setText(QString)));
        QObject::connect(d->control->completer(), SIGNAL(highlighted(QString)),
                         this, SLOT(_q_completionHighlighted(QString)));
    }
#endif
    update();
}

/*!\reimp
*/

void QLineEdit::focusOutEvent(QFocusEvent *e)
{
    QLineEditPrivate *  d = d_func();
    if (d->control->passwordEchoEditing()) {
        // Reset the echomode back to PasswordEchoOnEdit when the widget loses
        // focus.
        d->updatePasswordEchoEditing(false);
    }

    Qt::FocusReason reason = e->reason();
    if (reason != Qt::ActiveWindowFocusReason &&
        reason != Qt::PopupFocusReason)
        deselect();

    d->setCursorVisible(false);
    d->control->setBlinkingCursorEnabled(false);
#ifdef QT_KEYPAD_NAVIGATION
    // editingFinished() is already emitted on LeaveEditFocus
    if (!QApplication::keypadNavigationEnabled())
#endif
    if (reason != Qt::PopupFocusReason
        || !(QApplication::activePopupWidget() && QApplication::activePopupWidget()->parentWidget() == this)) {
            if (hasAcceptableInput() || d->control->fixup())
                emit editingFinished();
    }
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (d->control->echoMode() == Password || d->control->echoMode() == NoEcho)
        qt_mac_secure_keyboard(false);
#endif
#ifdef QT_KEYPAD_NAVIGATION
    d->control->setCancelText(QString());
#endif
#if QT_CONFIG(completer)
    if (d->control->completer()) {
        QObject::disconnect(d->control->completer(), 0, this, 0);
    }
#endif
    QWidget::focusOutEvent(e);
}

/*!\reimp
*/
void QLineEdit::paintEvent(QPaintEvent *)
{
    QLineEditPrivate *  d = d_func();
    QPainter p(this);
    QPalette pal = palette();   // oye 获取本widget的调色板 

    QStyleOptionFrame panel;  // LineEdit把自己当做一个panel来绘制
    initStyleOption(&panel);
	// 先画雏形PE_PanelLineEdit
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
	// panel里面必然有基准坐标, 然后根据此基准坐标,返回子元素,也就是说EditContents的Rect
    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	// 计算this内,自己定义的偏移
    r.setX(r.x() + d->effectiveLeftTextMargin()); 
    r.setY(r.y() + d->topTextMargin);
    r.setRight(r.right() - d->effectiveRightTextMargin());
    r.setBottom(r.bottom() - d->bottomTextMargin);
    p.setClipRect(r); // 有效绘图只能限制在这个区域

    QFontMetrics fm = fontMetrics();
    Qt::Alignment va = QStyle::visualAlignment(d->control->layoutDirection(), QFlag(d->alignment));
    switch (va & Qt::AlignVertical_Mask) {
     case Qt::AlignBottom:
         d->vscroll = r.y() + r.height() - fm.height() - d->verticalMargin;
         break;
     case Qt::AlignTop:
         d->vscroll = r.y() + d->verticalMargin;
         break;
     default:
         //center
         d->vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
         break;
    }
	//实际有限的绘制区域
    QRect lineRect(r.x() + d->horizontalMargin, d->vscroll, r.width() - 2*d->horizontalMargin, fm.height());
	//绘制 text_place_holder
    if (d->shouldShowPlaceholderText() && !d->placeholderText.isEmpty()) {        
        const Qt::LayoutDirection layoutDir = d->placeholderText.isRightToLeft() 
			? Qt::RightToLeft : Qt::LeftToRight;
        const Qt::Alignment alignPhText = QStyle::visualAlignment(layoutDir, QFlag(d->alignment));
        QColor col = pal.text().color();  // 调色板去文本色, 前景字符画刷
        col.setAlpha(128);
        QPen oldpen = p.pen();  // 从painter里取笔,保存旧笔,稍后复原
        p.setPen(col);			// 给笔上色
        Qt::LayoutDirection oldLayoutDir = p.layoutDirection();
        p.setLayoutDirection(layoutDir);
		//返回含有...结尾的字符
        const QString elidedText = fm.elidedText(d->placeholderText, Qt::ElideRight, lineRect.width());
        p.drawText(lineRect, alignPhText, elidedText); // 写出 placeholder text
		//
        p.setPen(oldpen);
        p.setLayoutDirection(oldLayoutDir);
        
    }

	// current index
    int cix = qRound(d->control->cursorToX());

    // horizontal scrolling. d->hscroll is the left indent from the beginning
    // of the text line to the left edge of lineRect. we update this value
    // depending on the delta from the last paint event; in effect this means
    // the below code handles all scrolling based on the textline (widthUsed),
    // the line edit rect (lineRect) and the cursor position (cix).
    int widthUsed = qRound(d->control->naturalTextWidth()) + 1;
    if (widthUsed <= lineRect.width()) {
        // text fits in lineRect; use hscroll for alignment
        switch (va & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
        case Qt::AlignRight:
            d->hscroll = widthUsed - lineRect.width() + 1;
            break;
        case Qt::AlignHCenter:
            d->hscroll = (widthUsed - lineRect.width()) / 2;
            break;
        default:
            // Left
            d->hscroll = 0;
            break;
        }
    } else if (cix - d->hscroll >= lineRect.width()) {
        // text doesn't fit, cursor is to the right of lineRect (scroll right)
        d->hscroll = cix - lineRect.width() + 1;
    } else if (cix - d->hscroll < 0 && d->hscroll < widthUsed) {
        // text doesn't fit, cursor is to the left of lineRect (scroll left)
        d->hscroll = cix;
    } else if (widthUsed - d->hscroll < lineRect.width()) {
        // text doesn't fit, text document is to the left of lineRect; align
        // right
        d->hscroll = widthUsed - lineRect.width() + 1;
    } else {
        //in case the text is bigger than the lineedit, the hscroll can never be negative
        d->hscroll = qMax(0, d->hscroll);
    }

    // the y offset is there to keep the baseline constant in case we have script changes in the text.
    QPoint topLeft = lineRect.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());


	
    // draw text, selections and cursors
#ifndef QT_NO_STYLE_STYLESHEET
	// qss发威的地方, 判断是否可以强转,如果可以,更新调色盘, 通过调色盘完美隔离了系统新增了css功能
    if (QStyleSheetStyle* cssStyle = qobject_cast<QStyleSheetStyle*>(style())) {
        cssStyle->styleSheetPalette(this, &panel, &pal);
    }
#endif
    p.setPen(pal.text().color());

    int flags = QWidgetLineControl::DrawText;

#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled() || hasEditFocus())
#endif
    if (d->control->hasSelectedText() || (d->cursorVisible && !d->control->inputMask().isEmpty() && !d->control->isReadOnly())){
        flags |= QWidgetLineControl::DrawSelections;
        // Palette only used for selections/mask and may not be in sync
        if (d->control->palette() != pal
           || d->control->palette().currentColorGroup() != pal.currentColorGroup())
            d->control->setPalette(pal);  // qss 修改后的pal 传递给内从控件
    }

    // Asian users see an IM selection text as cursor on candidate
    // selection phase of input method, so the ordinary cursor should be
    // invisible if we have a preedit string.
    if (d->cursorVisible && !d->control->isReadOnly())
        flags |= QWidgetLineControl::DrawCursor;

    d->control->setCursorWidth(style()->pixelMetric(QStyle::PM_TextCursorWidth));
    d->control->draw(&p, topLeft, r, flags);

}


#ifndef QT_NO_DRAGANDDROP
/*!\reimp
*/
void QLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
    QLineEditPrivate *  d = d_func();
    if (!d->control->isReadOnly() && e->mimeData()->hasFormat(QLatin1String("text/plain"))) {
        e->acceptProposedAction();
        d->control->moveCursor(d->xToPos(e->pos().x()), false);
        d->cursorVisible = true;
        update();
    }
}

/*!\reimp */
void QLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
    QLineEdit::dragMoveEvent(e);
}

/*!\reimp */
void QLineEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    QLineEditPrivate *  d = d_func();
    if (d->cursorVisible) {
        d->cursorVisible = false;
        update();
    }
}

/*!\reimp */
void QLineEdit::dropEvent(QDropEvent* e)
{
    QLineEditPrivate *  d = d_func();
    QString str = e->mimeData()->text();

    if (!str.isNull() && !d->control->isReadOnly()) {
        if (e->source() == this && e->dropAction() == Qt::CopyAction)
            deselect();
        int cursorPos = d->xToPos(e->pos().x());
        int selStart = cursorPos;
        int oldSelStart = d->control->selectionStart();
        int oldSelEnd = d->control->selectionEnd();
        d->control->moveCursor(cursorPos, false);
        d->cursorVisible = false;
        e->acceptProposedAction();
        insert(str);
        if (e->source() == this) {
            if (e->dropAction() == Qt::MoveAction) {
                if (selStart > oldSelStart && selStart <= oldSelEnd)
                    setSelection(oldSelStart, str.length());
                else if (selStart > oldSelEnd)
                    setSelection(selStart - str.length(), str.length());
                else
                    setSelection(selStart, str.length());
            } else {
                setSelection(selStart, str.length());
            }
        }
    } else {
        e->ignore();
        update();
    }
}

#endif // QT_NO_DRAGANDDROP

#ifndef QT_NO_CONTEXTMENU
/*!
    Shows the standard context menu created with
    createStandardContextMenu().

    If you do not want the line edit to have a context menu, you can set
    its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
    customize the context menu, reimplement this function. If you want
    to extend the standard context menu, reimplement this function, call
    createStandardContextMenu() and extend the menu returned.

    \snippet code/src_gui_widgets_qlineedit.cpp 0

    The \a event parameter is used to obtain the position where
    the mouse cursor was when the event was generated.

    \sa setContextMenuPolicy()
*/
void QLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(event->globalPos());
    }
}

static inline void setActionIcon(QAction *action, const QString &name)
{
    const QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        action->setIcon(icon);
}

/*!  This function creates the standard context menu which is shown
        when the user clicks on the line edit with the right mouse
        button. It is called from the default contextMenuEvent() handler.
        The popup menu's ownership is transferred to the caller.
*/

QMenu *QLineEdit::createStandardContextMenu()
{
    QLineEditPrivate *  d = d_func();
    QMenu *popup = new QMenu(this);
    popup->setObjectName(QLatin1String("qt_edit_menu"));
    QAction *action = 0;

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Undo") + ACCEL_KEY(QKeySequence::Undo));
        action->setEnabled(d->control->isUndoAvailable());
        setActionIcon(action, QStringLiteral("edit-undo"));
        connect(action, SIGNAL(triggered()), SLOT(undo()));

        action = popup->addAction(QLineEdit::tr("&Redo") + ACCEL_KEY(QKeySequence::Redo));
        action->setEnabled(d->control->isRedoAvailable());
        setActionIcon(action, QStringLiteral("edit-redo"));
        connect(action, SIGNAL(triggered()), SLOT(redo()));

        popup->addSeparator();
    }

#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Cu&t") + ACCEL_KEY(QKeySequence::Cut));
        action->setEnabled(!d->control->isReadOnly() && d->control->hasSelectedText()
                && d->control->echoMode() == QLineEdit::Normal);
        setActionIcon(action, QStringLiteral("edit-cut"));
        connect(action, SIGNAL(triggered()), SLOT(cut()));
    }

    action = popup->addAction(QLineEdit::tr("&Copy") + ACCEL_KEY(QKeySequence::Copy));
    action->setEnabled(d->control->hasSelectedText()
            && d->control->echoMode() == QLineEdit::Normal);
    setActionIcon(action, QStringLiteral("edit-copy"));
    connect(action, SIGNAL(triggered()), SLOT(copy()));

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Paste") + ACCEL_KEY(QKeySequence::Paste));
        action->setEnabled(!d->control->isReadOnly() && !QApplication::clipboard()->text().isEmpty());
        setActionIcon(action, QStringLiteral("edit-paste"));
        connect(action, SIGNAL(triggered()), SLOT(paste()));
    }
#endif

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Delete"));
        action->setEnabled(!d->control->isReadOnly() && !d->control->text().isEmpty() && d->control->hasSelectedText());
        setActionIcon(action, QStringLiteral("edit-delete"));
        connect(action, SIGNAL(triggered()), d->control, SLOT(_q_deleteSelected()));
    }

    if (!popup->isEmpty())
        popup->addSeparator();

    action = popup->addAction(QLineEdit::tr("Select All") + ACCEL_KEY(QKeySequence::SelectAll));
    action->setEnabled(!d->control->text().isEmpty() && !d->control->allSelected());
    d->selectAllAction = action;
    connect(action, SIGNAL(triggered()), SLOT(selectAll()));

    if (!d->control->isReadOnly() && QGuiApplication::styleHints()->useRtlExtensions()) {
        popup->addSeparator();
        QUnicodeControlCharacterMenu *ctrlCharacterMenu = new QUnicodeControlCharacterMenu(this, popup);
        popup->addMenu(ctrlCharacterMenu);
    }
    return popup;
}
#endif // QT_NO_CONTEXTMENU

/*! \reimp */
void QLineEdit::changeEvent(QEvent *ev)
{
    QLineEditPrivate *  d = d_func();
    switch(ev->type())
    {
    case QEvent::ActivationChange:
        if (!palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        break;
    case QEvent::FontChange:
        d->control->setFont(font());
        break;
    case QEvent::StyleChange:
        {
            QStyleOptionFrame opt;
            initStyleOption(&opt);
            d->control->setPasswordCharacter(style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, this));
            d->control->setPasswordMaskDelay(style()->styleHint(QStyle::SH_LineEdit_PasswordMaskDelay, &opt, this));
        }
        update();
        break;
    case QEvent::LayoutDirectionChange:
#if QT_CONFIG(toolbutton)
        for (const auto &e : d->trailingSideWidgets) { // Refresh icon to show arrow in right direction.
            if (e.flags & QLineEditPrivate::SideWidgetClearButton)
                static_cast<QLineEditIconButton *>(e.widget)->setIcon(d->clearButtonIcon());
        }
#endif
        d->positionSideWidgets();
        break;
    default:
        break;
    }
    QWidget::changeEvent(ev);
}

#include "moc_qlineedit.cpp"

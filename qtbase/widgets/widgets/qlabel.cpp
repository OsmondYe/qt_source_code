#include "qpainter.h"
#include "qevent.h"
#include "qdrawutil.h"
#include "qapplication.h"
#if QT_CONFIG(abstractbutton)
#include "qabstractbutton.h"
#endif
#include "qstyle.h"
#include "qstyleoption.h"
#include <limits.h>
#include "qaction.h"
#include "qclipboard.h"
#include <qdebug.h>
#include <qurl.h>
#include "qlabel_p.h"
#include "private/qstylesheetstyle_p.h"
#include <qmath.h>

#ifndef QT_NO_ACCESSIBILITY
#include <qaccessible.h>
#endif


QLabelPrivate::QLabelPrivate()
    : QFramePrivate(),
      sh(),
      msh(),
      text(),
      pixmap(Q_NULLPTR),
      scaledpixmap(Q_NULLPTR),
      cachedimage(Q_NULLPTR),
      picture(Q_NULLPTR),
      movie(),
      control(Q_NULLPTR),
      shortcutCursor(),
      cursor(),
      buddy(),
      shortcutId(0),
      textformat(Qt::AutoText),
      textInteractionFlags(Qt::LinksAccessibleByMouse),
      sizePolicy(),
      margin(0),
      align(Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs),
      indent(-1),
      valid_hints(false),
      scaledcontents(false),
      textLayoutDirty(false),
      textDirty(false),
      isRichText(false),
      isTextLabel(false),
      hasShortcut(/*???*/),
#ifndef QT_NO_CURSOR
      validCursor(false),
      onAnchor(false),
#endif
      openExternalLinks(false)
{
}


const QPicture *QLabel::picture() const
{
    QLabelPrivate * const d = d_func();
    return d->picture;
}


QLabel::QLabel(QWidget *parent, Qt::WindowFlags f)
    : QFrame(*new QLabelPrivate(), parent, f)
{
    QLabelPrivate * const d = d_func();
    d->init();
}

QLabel::QLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
    setText(text);
}

QLabel::~QLabel()
{
    QLabelPrivate * const d = d_func();
    d->clearContents();
}

void QLabelPrivate::init()
{
    QLabel * const q = q_func();

    q->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred,
                                 QSizePolicy::Label));
    setLayoutItemMargins(QStyle::SE_LabelLayoutItem);
}


void QLabel::setText(const QString &text)
{
    QLabelPrivate * const d = d_func();
    if (d->text == text)
        return;

    QWidgetTextControl *oldControl = d->control;
    d->control = 0;

    d->clearContents();
    d->text = text;
    d->isTextLabel = true;
    d->textDirty = true;
    d->isRichText = d->textformat == Qt::RichText
                    || (d->textformat == Qt::AutoText && Qt::mightBeRichText(d->text));

    d->control = oldControl;

    if (d->needTextControl()) {
        d->ensureTextControl();
    } else {
        delete d->control;
        d->control = 0;
    }

    if (d->isRichText) {
        setMouseTracking(true);
    } else {
        // Note: mouse tracking not disabled intentionally
    }

#ifndef QT_NO_SHORTCUT
    if (d->buddy)
        d->updateShortcut();
#endif

    d->updateLabel();


}

QString QLabel::text() const
{
    QLabelPrivate * const d = d_func();
    return d->text;
}

/*!
    Clears any label contents.
*/

void QLabel::clear()
{
    QLabelPrivate * const d = d_func();
    d->clearContents();
    d->updateLabel();
}

/*!
    \property QLabel::pixmap
    \brief the label's pixmap

    If no pixmap has been set this will return 0.

    Setting the pixmap clears any previous content. The buddy
    shortcut, if any, is disabled.
*/
void QLabel::setPixmap(const QPixmap &pixmap)
{
    QLabelPrivate * const d = d_func();
    if (!d->pixmap || d->pixmap->cacheKey() != pixmap.cacheKey()) {
        d->clearContents();
        d->pixmap = new QPixmap(pixmap);
    }

    if (d->pixmap->depth() == 1 && !d->pixmap->mask())
        d->pixmap->setMask(*((QBitmap *)d->pixmap));

    d->updateLabel();
}

const QPixmap *QLabel::pixmap() const
{
    QLabelPrivate * const d = d_func();
    return d->pixmap;
}

#ifndef QT_NO_PICTURE
/*!
    Sets the label contents to \a picture. Any previous content is
    cleared.

    The buddy shortcut, if any, is disabled.

    \sa picture(), setBuddy()
*/

void QLabel::setPicture(const QPicture &picture)
{
    QLabelPrivate * const d = d_func();
    d->clearContents();
    d->picture = new QPicture(picture);

    d->updateLabel();
}
#endif // QT_NO_PICTURE

/*!
    Sets the label contents to plain text containing the textual
    representation of integer \a num. Any previous content is cleared.
    Does nothing if the integer's string representation is the same as
    the current contents of the label.

    The buddy shortcut, if any, is disabled.

    \sa setText(), QString::setNum(), setBuddy()
*/

void QLabel::setNum(int num)
{
    QString str;
    str.setNum(num);
    setText(str);
}

/*!
    \overload

    Sets the label contents to plain text containing the textual
    representation of double \a num. Any previous content is cleared.
    Does nothing if the double's string representation is the same as
    the current contents of the label.

    The buddy shortcut, if any, is disabled.

    \sa setText(), QString::setNum(), setBuddy()
*/

void QLabel::setNum(double num)
{
    QString str;
    str.setNum(num);
    setText(str);
}

/*!
    \property QLabel::alignment
    \brief the alignment of the label's contents

    By default, the contents of the label are left-aligned and vertically-centered.

    \sa text
*/

void QLabel::setAlignment(Qt::Alignment alignment)
{
    QLabelPrivate * const d = d_func();
    if (alignment == (d->align & (Qt::AlignVertical_Mask|Qt::AlignHorizontal_Mask)))
        return;
    d->align = (d->align & ~(Qt::AlignVertical_Mask|Qt::AlignHorizontal_Mask))
               | (alignment & (Qt::AlignVertical_Mask|Qt::AlignHorizontal_Mask));

    d->updateLabel();
}


Qt::Alignment QLabel::alignment() const
{
    QLabelPrivate * const d = d_func();
    return QFlag(d->align & (Qt::AlignVertical_Mask|Qt::AlignHorizontal_Mask));
}



void QLabel::setWordWrap(bool on)
{
    QLabelPrivate * const d = d_func();
    if (on)
        d->align |= Qt::TextWordWrap;
    else
        d->align &= ~Qt::TextWordWrap;

    d->updateLabel();
}

bool QLabel::wordWrap() const
{
    QLabelPrivate * const d = d_func();
    return d->align & Qt::TextWordWrap;
}

/*!
    \property QLabel::indent
    \brief the label's text indent in pixels

    If a label displays text, the indent applies to the left edge if
    alignment() is Qt::AlignLeft, to the right edge if alignment() is
    Qt::AlignRight, to the top edge if alignment() is Qt::AlignTop, and
    to the bottom edge if alignment() is Qt::AlignBottom.

    If indent is negative, or if no indent has been set, the label
    computes the effective indent as follows: If frameWidth() is 0,
    the effective indent becomes 0. If frameWidth() is greater than 0,
    the effective indent becomes half the width of the "x" character
    of the widget's current font().

    By default, the indent is -1, meaning that an effective indent is
    calculating in the manner described above.

    \sa alignment, margin, frameWidth(), font()
*/

void QLabel::setIndent(int indent)
{
    QLabelPrivate * const d = d_func();
    d->indent = indent;
    d->updateLabel();
}

int QLabel::indent() const
{
    QLabelPrivate * const d = d_func();
    return d->indent;
}


/*!
    \property QLabel::margin
    \brief the width of the margin

    The margin is the distance between the innermost pixel of the
    frame and the outermost pixel of contents.

    The default margin is 0.

    \sa indent
*/
int QLabel::margin() const
{
    QLabelPrivate * const d = d_func();
    return d->margin;
}

void QLabel::setMargin(int margin)
{
    QLabelPrivate * const d = d_func();
    if (d->margin == margin)
        return;
    d->margin = margin;
    d->updateLabel();
}

/*!
    Returns the size that will be used if the width of the label is \a
    w. If \a w is -1, the sizeHint() is returned. If \a w is 0 minimumSizeHint() is returned
*/
QSize QLabelPrivate::sizeForWidth(int w) const
{
    Q_Q(const QLabel);
    if(q->minimumWidth() > 0)
        w = qMax(w, q->minimumWidth());
    QSize contentsMargin(leftmargin + rightmargin, topmargin + bottommargin);

    QRect br;

    int hextra = 2 * margin;
    int vextra = hextra;
    QFontMetrics fm = q->fontMetrics();

    if (pixmap && !pixmap->isNull()) {
        br = pixmap->rect();
        br.setSize(br.size() / pixmap->devicePixelRatio());
#ifndef QT_NO_PICTURE
    } else if (picture && !picture->isNull()) {
        br = picture->boundingRect();
#endif
#if QT_CONFIG(movie)
    } else if (movie && !movie->currentPixmap().isNull()) {
        br = movie->currentPixmap().rect();
        br.setSize(br.size() / movie->currentPixmap().devicePixelRatio());
#endif
    } else if (isTextLabel) {
        int align = QStyle::visualAlignment(textDirection(), QFlag(this->align));
        // Add indentation
        int m = indent;

        if (m < 0 && q->frameWidth()) // no indent, but we do have a frame
            m = fm.width(QLatin1Char('x')) - margin*2;
        if (m > 0) {
            if ((align & Qt::AlignLeft) || (align & Qt::AlignRight))
                hextra += m;
            if ((align & Qt::AlignTop) || (align & Qt::AlignBottom))
                vextra += m;
        }

        if (control) {
            ensureTextLayouted();
            const qreal oldTextWidth = control->textWidth();
            // Calculate the length of document if w is the width
            if (align & Qt::TextWordWrap) {
                if (w >= 0) {
                    w = qMax(w-hextra-contentsMargin.width(), 0); // strip margin and indent
                    control->setTextWidth(w);
                } else {
                    control->adjustSize();
                }
            } else {
                control->setTextWidth(-1);
            }

            QSizeF controlSize = control->size();
            br = QRect(QPoint(0, 0), QSize(qCeil(controlSize.width()), qCeil(controlSize.height())));

            // restore state
            control->setTextWidth(oldTextWidth);
        } else {
            // Turn off center alignment in order to avoid rounding errors for centering,
            // since centering involves a division by 2. At the end, all we want is the size.
            int flags = align & ~(Qt::AlignVCenter | Qt::AlignHCenter);
            if (hasShortcut) {
                flags |= Qt::TextShowMnemonic;
                QStyleOption opt;
                opt.initFrom(q);
                if (!q->style()->styleHint(QStyle::SH_UnderlineShortcut, &opt, q))
                    flags |= Qt::TextHideMnemonic;
            }

            bool tryWidth = (w < 0) && (align & Qt::TextWordWrap);
            if (tryWidth)
                w = qMin(fm.averageCharWidth() * 80, q->maximumSize().width());
            else if (w < 0)
                w = 2000;
            w -= (hextra + contentsMargin.width());
            br = fm.boundingRect(0, 0, w ,2000, flags, text);
            if (tryWidth && br.height() < 4*fm.lineSpacing() && br.width() > w/2)
                br = fm.boundingRect(0, 0, w/2, 2000, flags, text);
            if (tryWidth && br.height() < 2*fm.lineSpacing() && br.width() > w/4)
                br = fm.boundingRect(0, 0, w/4, 2000, flags, text);
        }
    } else {
        br = QRect(QPoint(0, 0), QSize(fm.averageCharWidth(), fm.lineSpacing()));
    }

    const QSize contentsSize(br.width() + hextra, br.height() + vextra);
    return (contentsSize + contentsMargin).expandedTo(q->minimumSize());
}


/*!
  \reimp
*/

int QLabel::heightForWidth(int w) const
{
    QLabelPrivate * const d = d_func();
    if (d->isTextLabel)
        return d->sizeForWidth(w).height();
    return QWidget::heightForWidth(w);
}

/*!
    \property QLabel::openExternalLinks
    \since 4.2

    Specifies whether QLabel should automatically open links using
    QDesktopServices::openUrl() instead of emitting the
    linkActivated() signal.

    \b{Note:} The textInteractionFlags set on the label need to include
    either LinksAccessibleByMouse or LinksAccessibleByKeyboard.

    The default value is false.

    \sa textInteractionFlags()
*/
bool QLabel::openExternalLinks() const
{
    QLabelPrivate * const d = d_func();
    return d->openExternalLinks;
}

void QLabel::setOpenExternalLinks(bool open)
{
    QLabelPrivate * const d = d_func();
    d->openExternalLinks = open;
    if (d->control)
        d->control->setOpenExternalLinks(open);
}

/*!
    \property QLabel::textInteractionFlags
    \since 4.2

    Specifies how the label should interact with user input if it displays text.

    If the flags contain Qt::LinksAccessibleByKeyboard the focus policy is also
    automatically set to Qt::StrongFocus. If Qt::TextSelectableByKeyboard is set
    then the focus policy is set to Qt::ClickFocus.

    The default value is Qt::LinksAccessibleByMouse.
*/
void QLabel::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    QLabelPrivate * const d = d_func();
    if (d->textInteractionFlags == flags)
        return;
    d->textInteractionFlags = flags;
    if (flags & Qt::LinksAccessibleByKeyboard)
        setFocusPolicy(Qt::StrongFocus);
    else if (flags & (Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse))
        setFocusPolicy(Qt::ClickFocus);
    else
        setFocusPolicy(Qt::NoFocus);

    if (d->needTextControl()) {
        d->ensureTextControl();
    } else {
        delete d->control;
        d->control = 0;
    }

    if (d->control)
        d->control->setTextInteractionFlags(d->textInteractionFlags);
}

Qt::TextInteractionFlags QLabel::textInteractionFlags() const
{
    QLabelPrivate * const d = d_func();
    return d->textInteractionFlags;
}

/*!
    Selects text from position \a start and for \a length characters.

    \sa selectedText()

    \b{Note:} The textInteractionFlags set on the label need to include
    either TextSelectableByMouse or TextSelectableByKeyboard.

    \since 4.7
*/
void QLabel::setSelection(int start, int length)
{
    QLabelPrivate * const d = d_func();
    if (d->control) {
        d->ensureTextPopulated();
        QTextCursor cursor = d->control->textCursor();
        cursor.setPosition(start);
        cursor.setPosition(start + length, QTextCursor::KeepAnchor);
        d->control->setTextCursor(cursor);
    }
}

/*!
    \property QLabel::hasSelectedText
    \brief whether there is any text selected

    hasSelectedText() returns \c true if some or all of the text has been
    selected by the user; otherwise returns \c false.

    By default, this property is \c false.

    \sa selectedText()

    \b{Note:} The textInteractionFlags set on the label need to include
    either TextSelectableByMouse or TextSelectableByKeyboard.

    \since 4.7
*/
bool QLabel::hasSelectedText() const
{
    QLabelPrivate * const d = d_func();
    if (d->control)
        return d->control->textCursor().hasSelection();
    return false;
}

/*!
    \property QLabel::selectedText
    \brief the selected text

    If there is no selected text this property's value is
    an empty string.

    By default, this property contains an empty string.

    \sa hasSelectedText()

    \b{Note:} The textInteractionFlags set on the label need to include
    either TextSelectableByMouse or TextSelectableByKeyboard.

    \since 4.7
*/
QString QLabel::selectedText() const
{
    QLabelPrivate * const d = d_func();
    if (d->control)
        return d->control->textCursor().selectedText();
    return QString();
}

/*!
    selectionStart() returns the index of the first selected character in the
    label or -1 if no text is selected.

    \sa selectedText()

    \b{Note:} The textInteractionFlags set on the label need to include
    either TextSelectableByMouse or TextSelectableByKeyboard.

    \since 4.7
*/
int QLabel::selectionStart() const
{
    QLabelPrivate * const d = d_func();
    if (d->control && d->control->textCursor().hasSelection())
        return d->control->textCursor().selectionStart();
    return -1;
}

/*!\reimp
*/
QSize QLabel::sizeHint() const
{
    QLabelPrivate * const d = d_func();
    if (!d->valid_hints)
        (void) QLabel::minimumSizeHint();
    return d->sh;
}

/*!
  \reimp
*/
QSize QLabel::minimumSizeHint() const
{
    QLabelPrivate * const d = d_func();
    if (d->valid_hints) {
        if (d->sizePolicy == sizePolicy())
            return d->msh;
    }

    ensurePolished();
    d->valid_hints = true;
    d->sh = d->sizeForWidth(-1); // wrap ? golden ratio : min doc size
    QSize msh(-1, -1);

    if (!d->isTextLabel) {
        msh = d->sh;
    } else {
        msh.rheight() = d->sizeForWidth(QWIDGETSIZE_MAX).height(); // height for one line
        msh.rwidth() = d->sizeForWidth(0).width(); // wrap ? size of biggest word : min doc size
        if (d->sh.height() < msh.height())
            msh.rheight() = d->sh.height();
    }
    d->msh = msh;
    d->sizePolicy = sizePolicy();
    return msh;
}

/*!\reimp
*/
void QLabel::mousePressEvent(QMouseEvent *ev)
{
    QLabelPrivate * const d = d_func();
    d->sendControlEvent(ev);
}

/*!\reimp
*/
void QLabel::mouseMoveEvent(QMouseEvent *ev)
{
    QLabelPrivate * const d = d_func();
    d->sendControlEvent(ev);
}

/*!\reimp
*/
void QLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    QLabelPrivate * const d = d_func();
    d->sendControlEvent(ev);
}

#ifndef QT_NO_CONTEXTMENU
/*!\reimp
*/
void QLabel::contextMenuEvent(QContextMenuEvent *ev)
{
    QLabelPrivate * const d = d_func();
    if (!d->isTextLabel) {
        ev->ignore();
        return;
    }
    QMenu *menu = d->createStandardContextMenu(ev->pos());
    if (!menu) {
        ev->ignore();
        return;
    }
    ev->accept();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(ev->globalPos());
}
#endif // QT_NO_CONTEXTMENU

/*!
    \reimp
*/
void QLabel::focusInEvent(QFocusEvent *ev)
{
    QLabelPrivate * const d = d_func();
    if (d->isTextLabel) {
        d->ensureTextControl();
        d->sendControlEvent(ev);
    }
    QFrame::focusInEvent(ev);
}

/*!
    \reimp
*/
void QLabel::focusOutEvent(QFocusEvent *ev)
{
    QLabelPrivate * const d = d_func();
    if (d->control) {
        d->sendControlEvent(ev);
        QTextCursor cursor = d->control->textCursor();
        Qt::FocusReason reason = ev->reason();
        if (reason != Qt::ActiveWindowFocusReason
            && reason != Qt::PopupFocusReason
            && cursor.hasSelection()) {
            cursor.clearSelection();
            d->control->setTextCursor(cursor);
        }
    }

    QFrame::focusOutEvent(ev);
}

/*!\reimp
*/
bool QLabel::focusNextPrevChild(bool next)
{
    QLabelPrivate * const d = d_func();
    if (d->control && d->control->setFocusToNextOrPreviousAnchor(next))
        return true;
    return QFrame::focusNextPrevChild(next);
}

/*!\reimp
*/
void QLabel::keyPressEvent(QKeyEvent *ev)
{
    QLabelPrivate * const d = d_func();
    d->sendControlEvent(ev);
}

/*!\reimp
*/
bool QLabel::event(QEvent *e)
{
    QLabelPrivate * const d = d_func();
    QEvent::Type type = e->type();

#ifndef QT_NO_SHORTCUT
    if (type == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == d->shortcutId) {
            QWidget * w = d->buddy;
            if (!w)
                return QFrame::event(e);
            if (w->focusPolicy() != Qt::NoFocus)
                w->setFocus(Qt::ShortcutFocusReason);
            QAbstractButton *button = qobject_cast<QAbstractButton *>(w);
            if (button && !se->isAmbiguous())
                button->animateClick();
            else
                window()->setAttribute(Qt::WA_KeyboardFocusChange);
            return true;
        }
    } else
#endif
    if (type == QEvent::Resize) {
        if (d->control)
            d->textLayoutDirty = true;
    } 
		else if (e->type() == QEvent::StyleChange) 
    {
        d->setLayoutItemMargins(QStyle::SE_LabelLayoutItem);
        d->updateLabel();
    }

    return QFrame::event(e);
}

/*!\reimp
*/
void QLabel::paintEvent(QPaintEvent *)
{
    QLabelPrivate * const d = d_func();
    QStyle *style = QWidget::style();
    QPainter painter(this);
    drawFrame(&painter);
    QRect cr = contentsRect();
    cr.adjust(d->margin, d->margin, -d->margin, -d->margin);
    int align = QStyle::visualAlignment(d->isTextLabel ? d->textDirection()
                                                       : layoutDirection(), QFlag(d->align));

#if QT_CONFIG(movie)
    if (d->movie) {
        if (d->scaledcontents)
            style->drawItemPixmap(&painter, cr, align, d->movie->currentPixmap().scaled(cr.size()));
        else
            style->drawItemPixmap(&painter, cr, align, d->movie->currentPixmap());
    }
    else
#endif
    if (d->isTextLabel) {
        QRectF lr = d->layoutRect().toAlignedRect();
        QStyleOption opt;
        opt.initFrom(this);
#ifndef QT_NO_STYLE_STYLESHEET
        if (QStyleSheetStyle* cssStyle = qobject_cast<QStyleSheetStyle*>(style)) {
            cssStyle->styleSheetPalette(this, &opt, &opt.palette);
        }
#endif
        if (d->control) {
#ifndef QT_NO_SHORTCUT
            const bool underline = (bool)style->styleHint(QStyle::SH_UnderlineShortcut, 0, this, 0);
            if (d->shortcutId != 0
                && underline != d->shortcutCursor.charFormat().fontUnderline()) {
                QTextCharFormat fmt;
                fmt.setFontUnderline(underline);
                d->shortcutCursor.mergeCharFormat(fmt);
            }
#endif
            d->ensureTextLayouted();

            QAbstractTextDocumentLayout::PaintContext context;
            // Adjust the palette
            context.palette = opt.palette;

            if (foregroundRole() != QPalette::Text && isEnabled())
                context.palette.setColor(QPalette::Text, context.palette.color(foregroundRole()));

            painter.save();
            painter.translate(lr.topLeft());
            painter.setClipRect(lr.translated(-lr.x(), -lr.y()));
            d->control->setPalette(context.palette);
            d->control->drawContents(&painter, QRectF(), this);
            painter.restore();
        } else {
            int flags = align | (d->textDirection() == Qt::LeftToRight ? Qt::TextForceLeftToRight
                                                                       : Qt::TextForceRightToLeft);
            if (d->hasShortcut) {
                flags |= Qt::TextShowMnemonic;
                if (!style->styleHint(QStyle::SH_UnderlineShortcut, &opt, this))
                    flags |= Qt::TextHideMnemonic;
            }
            style->drawItemText(&painter, lr.toRect(), flags, opt.palette, isEnabled(), d->text, foregroundRole());
        }
    } else
#ifndef QT_NO_PICTURE
    if (d->picture) {
        QRect br = d->picture->boundingRect();
        int rw = br.width();
        int rh = br.height();
        if (d->scaledcontents) {
            painter.save();
            painter.translate(cr.x(), cr.y());
            painter.scale((double)cr.width()/rw, (double)cr.height()/rh);
            painter.drawPicture(-br.x(), -br.y(), *d->picture);
            painter.restore();
        } else {
            int xo = 0;
            int yo = 0;
            if (align & Qt::AlignVCenter)
                yo = (cr.height()-rh)/2;
            else if (align & Qt::AlignBottom)
                yo = cr.height()-rh;
            if (align & Qt::AlignRight)
                xo = cr.width()-rw;
            else if (align & Qt::AlignHCenter)
                xo = (cr.width()-rw)/2;
            painter.drawPicture(cr.x()+xo-br.x(), cr.y()+yo-br.y(), *d->picture);
        }
    } else
#endif
    if (d->pixmap && !d->pixmap->isNull()) {
        QPixmap pix;
        if (d->scaledcontents) {
            QSize scaledSize = cr.size() * devicePixelRatioF();
            if (!d->scaledpixmap || d->scaledpixmap->size() != scaledSize) {
                if (!d->cachedimage)
                    d->cachedimage = new QImage(d->pixmap->toImage());
                delete d->scaledpixmap;
                QImage scaledImage =
                    d->cachedimage->scaled(scaledSize,
                                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                d->scaledpixmap = new QPixmap(QPixmap::fromImage(scaledImage));
                d->scaledpixmap->setDevicePixelRatio(devicePixelRatioF());
            }
            pix = *d->scaledpixmap;
        } else
            pix = *d->pixmap;
        QStyleOption opt;
        opt.initFrom(this);
        if (!isEnabled())
            pix = style->generatedIconPixmap(QIcon::Disabled, pix, &opt);
        style->drawItemPixmap(&painter, cr, align, pix);
    }
}


void QLabelPrivate::updateLabel()
{
    QLabel * const q = q_func();
    valid_hints = false;

    if (isTextLabel) {
        QSizePolicy policy = q->sizePolicy();
        const bool wrap = align & Qt::TextWordWrap;
        policy.setHeightForWidth(wrap);
        if (policy != q->sizePolicy())  // ### should be replaced by WA_WState_OwnSizePolicy idiom
            q->setSizePolicy(policy);
        textLayoutDirty = true;
    }
    q->updateGeometry();
    q->update(q->contentsRect());
}

#ifndef QT_NO_SHORTCUT
/*!
    Sets this label's buddy to \a buddy.

    When the user presses the shortcut key indicated by this label,
    the keyboard focus is transferred to the label's buddy widget.

    The buddy mechanism is only available for QLabels that contain
    text in which one character is prefixed with an ampersand, '&'.
    This character is set as the shortcut key. See the \l
    QKeySequence::mnemonic() documentation for details (to display an
    actual ampersand, use '&&').

    In a dialog, you might create two data entry widgets and a label
    for each, and set up the geometry layout so each label is just to
    the left of its data entry widget (its "buddy"), for example:
    \snippet code/src_gui_widgets_qlabel.cpp 2

    With the code above, the focus jumps to the Name field when the
    user presses Alt+N, and to the Phone field when the user presses
    Alt+P.

    To unset a previously set buddy, call this function with \a buddy
    set to 0.

    \sa buddy(), setText(), QShortcut, setAlignment()
*/

void QLabel::setBuddy(QWidget *buddy)
{
    QLabelPrivate * const d = d_func();
    d->buddy = buddy;
    if (d->isTextLabel) {
        if (d->shortcutId)
            releaseShortcut(d->shortcutId);
        d->shortcutId = 0;
        d->textDirty = true;
        if (buddy)
            d->updateShortcut(); // grab new shortcut
        d->updateLabel();
    }
}


/*!
    Returns this label's buddy, or 0 if no buddy is currently set.

    \sa setBuddy()
*/

QWidget * QLabel::buddy() const
{
    QLabelPrivate * const d = d_func();
    return d->buddy;
}

void QLabelPrivate::updateShortcut()
{
    QLabel * const q = q_func();
    Q_ASSERT(shortcutId == 0);
    // Introduce an extra boolean to indicate the presence of a shortcut in the
    // text. We cannot use the shortcutId itself because on the mac mnemonics are
    // off by default, so QKeySequence::mnemonic always returns an empty sequence.
    // But then we do want to hide the ampersands, so we can't use shortcutId.
    hasShortcut = false;

    if (!text.contains(QLatin1Char('&')))
        return;
    hasShortcut = true;
    shortcutId = q->grabShortcut(QKeySequence::mnemonic(text));
}

#endif // QT_NO_SHORTCUT

#if QT_CONFIG(movie)
void QLabelPrivate::_q_movieUpdated(const QRect& rect)
{
    QLabel * const q = q_func();
    if (movie && movie->isValid()) {
        QRect r;
        if (scaledcontents) {
            QRect cr = q->contentsRect();
            QRect pixmapRect(cr.topLeft(), movie->currentPixmap().size());
            if (pixmapRect.isEmpty())
                return;
            r.setRect(cr.left(), cr.top(),
                      (rect.width() * cr.width()) / pixmapRect.width(),
                      (rect.height() * cr.height()) / pixmapRect.height());
        } else {
            r = q->style()->itemPixmapRect(q->contentsRect(), align, movie->currentPixmap());
            r.translate(rect.x(), rect.y());
            r.setWidth(qMin(r.width(), rect.width()));
            r.setHeight(qMin(r.height(), rect.height()));
        }
        q->update(r);
    }
}

void QLabelPrivate::_q_movieResized(const QSize& size)
{
    QLabel * const q = q_func();
    q->update(); //we need to refresh the whole background in case the new size is smaler
    valid_hints = false;
    _q_movieUpdated(QRect(QPoint(0,0), size));
    q->updateGeometry();
}

/*!
    Sets the label contents to \a movie. Any previous content is
    cleared. The label does NOT take ownership of the movie.

    The buddy shortcut, if any, is disabled.

    \sa movie(), setBuddy()
*/

void QLabel::setMovie(QMovie *movie)
{
    QLabelPrivate * const d = d_func();
    d->clearContents();

    if (!movie)
        return;

    d->movie = movie;
    connect(movie, SIGNAL(resized(QSize)), this, SLOT(_q_movieResized(QSize)));
    connect(movie, SIGNAL(updated(QRect)), this, SLOT(_q_movieUpdated(QRect)));

    // Assume that if the movie is running,
    // resize/update signals will come soon enough
    if (movie->state() != QMovie::Running)
        d->updateLabel();
}

#endif // QT_CONFIG(movie)


void QLabelPrivate::clearContents()
{
    delete control;
    control = 0;
    isTextLabel = false;
    hasShortcut = false;

#ifndef QT_NO_PICTURE
    delete picture;
    picture = 0;
#endif
    delete scaledpixmap;
    scaledpixmap = 0;
    delete cachedimage;
    cachedimage = 0;
    delete pixmap;
    pixmap = 0;

    text.clear();
    QLabel * const q = q_func();
#ifndef QT_NO_SHORTCUT
    if (shortcutId)
        q->releaseShortcut(shortcutId);
    shortcutId = 0;
#endif
#if QT_CONFIG(movie)
    if (movie) {
        QObject::disconnect(movie, SIGNAL(resized(QSize)), q, SLOT(_q_movieResized(QSize)));
        QObject::disconnect(movie, SIGNAL(updated(QRect)), q, SLOT(_q_movieUpdated(QRect)));
    }
    movie = 0;
#endif
#ifndef QT_NO_CURSOR
    if (onAnchor) {
        if (validCursor)
            q->setCursor(cursor);
        else
            q->unsetCursor();
    }
    validCursor = false;
    onAnchor = false;
#endif
}


QMovie *QLabel::movie() const
{
    QLabelPrivate * const d = d_func();
    return d->movie;
}

Qt::TextFormat QLabel::textFormat() const
{
    QLabelPrivate * const d = d_func();
    return d->textformat;
}

void QLabel::setTextFormat(Qt::TextFormat format)
{
    QLabelPrivate * const d = d_func();
    if (format != d->textformat) {
        d->textformat = format;
        QString t = d->text;
        if (!t.isNull()) {
            d->text.clear();
            setText(t);
        }
    }
}


void QLabel::changeEvent(QEvent *ev)
{
    QLabelPrivate * const d = d_func();
	// 文字显示委托给了d->control
    if(ev->type() == QEvent::FontChange || ev->type() == QEvent::ApplicationFontChange) {
        if (d->isTextLabel) {
            if (d->control)
                d->control->document()->setDefaultFont(font());
            d->updateLabel();
        }
    } else if (ev->type() == QEvent::PaletteChange && d->control) {
        d->control->setPalette(palette());
	// margin改变了
    } else if (ev->type() == QEvent::ContentsRectChange) {
        d->updateLabel();
    }
    QFrame::changeEvent(ev);
}

/*!
    \property QLabel::scaledContents
    \brief whether the label will scale its contents to fill all
    available space.

    When enabled and the label shows a pixmap, it will scale the
    pixmap to fill the available space.

    This property's default is false.
*/
bool QLabel::hasScaledContents() const
{
    QLabelPrivate * const d = d_func();
    return d->scaledcontents;
}

void QLabel::setScaledContents(bool enable)
{
    QLabelPrivate * const d = d_func();
    if ((bool)d->scaledcontents == enable)
        return;
    d->scaledcontents = enable;
    if (!enable) {
        delete d->scaledpixmap;
        d->scaledpixmap = 0;
        delete d->cachedimage;
        d->cachedimage = 0;
    }
    update(contentsRect());
}

Qt::LayoutDirection QLabelPrivate::textDirection() const
{
    if (control) {
        QTextOption opt = control->document()->defaultTextOption();
        return opt.textDirection();
    }

    return text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
}


// Returns the rect that is available for us to draw the document
QRect QLabelPrivate::documentRect() const
{
    Q_Q(const QLabel);
    Q_ASSERT_X(isTextLabel, "documentRect", "document rect called for label that is not a text label!");
    QRect cr = q->contentsRect();
    cr.adjust(margin, margin, -margin, -margin);
    const int align = QStyle::visualAlignment(isTextLabel ? textDirection()
                                                          : q->layoutDirection(), QFlag(this->align));
    int m = indent;
    if (m < 0 && q->frameWidth()) // no indent, but we do have a frame
        m = q->fontMetrics().width(QLatin1Char('x')) / 2 - margin;
    if (m > 0) {
        if (align & Qt::AlignLeft)
            cr.setLeft(cr.left() + m);
        if (align & Qt::AlignRight)
            cr.setRight(cr.right() - m);
        if (align & Qt::AlignTop)
            cr.setTop(cr.top() + m);
        if (align & Qt::AlignBottom)
            cr.setBottom(cr.bottom() - m);
    }
    return cr;
}

void QLabelPrivate::ensureTextPopulated() const
{
    if (!textDirty)
        return;
    if (control) {
        QTextDocument *doc = control->document();
        if (textDirty) {
#ifndef QT_NO_TEXTHTMLPARSER
            if (isRichText)
                doc->setHtml(text);
            else
                doc->setPlainText(text);
#else
            doc->setPlainText(text);
#endif
            doc->setUndoRedoEnabled(false);

#ifndef QT_NO_SHORTCUT
            if (hasShortcut) {
                // Underline the first character that follows an ampersand (and remove the others ampersands)
                int from = 0;
                bool found = false;
                QTextCursor cursor;
                while (!(cursor = control->document()->find((QLatin1String("&")), from)).isNull()) {
                    cursor.deleteChar(); // remove the ampersand
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    from = cursor.position();
                    if (!found && cursor.selectedText() != QLatin1String("&")) { //not a second &
                        found = true;
                        shortcutCursor = cursor;
                    }
                }
            }
#endif
        }
    }
    textDirty = false;
}

void QLabelPrivate::ensureTextLayouted() const
{
    if (!textLayoutDirty)
        return;
    ensureTextPopulated();
    if (control) {
        QTextDocument *doc = control->document();
        QTextOption opt = doc->defaultTextOption();

        opt.setAlignment(QFlag(this->align));

        if (this->align & Qt::TextWordWrap)
            opt.setWrapMode(QTextOption::WordWrap);
        else
            opt.setWrapMode(QTextOption::ManualWrap);

        doc->setDefaultTextOption(opt);

        QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
        fmt.setMargin(0);
        doc->rootFrame()->setFrameFormat(fmt);
        doc->setTextWidth(documentRect().width());
    }
    textLayoutDirty = false;
}

void QLabelPrivate::ensureTextControl() const
{
    Q_Q(const QLabel);
    if (!isTextLabel)
        return;
    if (!control) {
        control = new QWidgetTextControl(const_cast<QLabel *>(q));
        control->document()->setUndoRedoEnabled(false);
        control->document()->setDefaultFont(q->font());
        control->setTextInteractionFlags(textInteractionFlags);
        control->setOpenExternalLinks(openExternalLinks);
        control->setPalette(q->palette());
        control->setFocus(q->hasFocus());
        QObject::connect(control, SIGNAL(updateRequest(QRectF)),
                         q, SLOT(update()));
        QObject::connect(control, SIGNAL(linkHovered(QString)),
                         q, SLOT(_q_linkHovered(QString)));
        QObject::connect(control, SIGNAL(linkActivated(QString)),
                         q, SIGNAL(linkActivated(QString)));
        textLayoutDirty = true;
        textDirty = true;
    }
}

void QLabelPrivate::sendControlEvent(QEvent *e)
{
    QLabel * const q = q_func();
    if (!isTextLabel || !control || textInteractionFlags == Qt::NoTextInteraction) {
        e->ignore();
        return;
    }
    control->processEvent(e, -layoutRect().topLeft(), q);
}

void QLabelPrivate::_q_linkHovered(const QString &anchor)
{
    QLabel * const q = q_func();
#ifndef QT_NO_CURSOR
    if (anchor.isEmpty()) { // restore cursor
        if (validCursor)
            q->setCursor(cursor);
        else
            q->unsetCursor();
        onAnchor = false;
    } else if (!onAnchor) {
        validCursor = q->testAttribute(Qt::WA_SetCursor);
        if (validCursor) {
            cursor = q->cursor();
        }
        q->setCursor(Qt::PointingHandCursor);
        onAnchor = true;
    }
#endif
    emit q->linkHovered(anchor);
}

// Return the layout rect - this is the rect that is given to the layout painting code
// This may be different from the document rect since vertical alignment is not
// done by the text layout code
QRectF QLabelPrivate::layoutRect() const
{
    QRectF cr = documentRect();
    if (!control)
        return cr;
    ensureTextLayouted();
    // Caculate y position manually
    qreal rh = control->document()->documentLayout()->documentSize().height();
    qreal yo = 0;
    if (align & Qt::AlignVCenter)
        yo = qMax((cr.height()-rh)/2, qreal(0));
    else if (align & Qt::AlignBottom)
        yo = qMax(cr.height()-rh, qreal(0));
    return QRectF(cr.x(), yo + cr.y(), cr.width(), cr.height());
}

// Returns the point in the document rect adjusted with p
QPoint QLabelPrivate::layoutPoint(const QPoint& p) const
{
    QRect lr = layoutRect().toRect();
    return p - lr.topLeft();
}

#ifndef QT_NO_CONTEXTMENU
QMenu *QLabelPrivate::createStandardContextMenu(const QPoint &pos)
{
    QString linkToCopy;
    QPoint p;
    if (control && isRichText) {
        p = layoutPoint(pos);
        linkToCopy = control->document()->documentLayout()->anchorAt(p);
    }

    if (linkToCopy.isEmpty() && !control)
        return 0;

    return control->createStandardContextMenu(p, q_func());
}
#endif



#include "moc_qlabel.cpp"

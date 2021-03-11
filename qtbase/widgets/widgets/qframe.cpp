#include "qframe.h"
#include "qbitmap.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qapplication.h"

#include "qframe_p.h"

QFramePrivate::QFramePrivate()
    : frect(0, 0, 0, 0),
      frameStyle(QFrame::NoFrame | QFrame::Plain),
      lineWidth(1),
      midLineWidth(0),
      frameWidth(0),
      leftFrameWidth(0), rightFrameWidth(0),
      topFrameWidth(0), bottomFrameWidth(0)
{
}

QFramePrivate::~QFramePrivate()
{
}

inline void QFramePrivate::init()
{
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
}

QFrame::QFrame(QWidget* parent, Qt::WindowFlags f)
    : QWidget(*new QFramePrivate, parent, f)
{
    QFramePrivate * const d = d_func();
    d->init();
}

QFrame::QFrame(QFramePrivate &dd, QWidget* parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f)
{
    QFramePrivate * const d = d_func();
    d->init();
}

void QFrame::initStyleOption(QStyleOptionFrame *option) const
{
    if (!option)
        return;

    QFramePrivate * const d = d_func();
    option->initFrom(this);		//先给个基本值, 然后再开始修改

    int frameShape  = d->frameStyle & QFrame::Shape_Mask;
    int frameShadow = d->frameStyle & QFrame::Shadow_Mask;
    option->frameShape = Shape(int(option->frameShape) | frameShape);
    option->rect = frameRect();
    switch (frameShape) {
        case QFrame::Box:
        case QFrame::HLine:
        case QFrame::VLine:
        case QFrame::StyledPanel:
        case QFrame::Panel:
            option->lineWidth = d->lineWidth;
            option->midLineWidth = d->midLineWidth;
            break;
        default:
            // most frame styles do not handle customized line and midline widths
            // (see updateFrameWidth()).
            option->lineWidth = d->frameWidth;
            break;
    }

    if (frameShadow == Sunken)
        option->state |= QStyle::State_Sunken;
    else if (frameShadow == Raised)
        option->state |= QStyle::State_Raised;
}

int QFrame::frameStyle() const
{
    QFramePrivate * const d = d_func();
    return d->frameStyle;
}

QFrame::Shape QFrame::frameShape() const
{
    QFramePrivate * const d = d_func();
    return (Shape) (d->frameStyle & Shape_Mask);
}

void QFrame::setFrameShape(QFrame::Shape s)
{
    QFramePrivate * const d = d_func();
    setFrameStyle((d->frameStyle & Shadow_Mask) | s);
}

QFrame::Shadow QFrame::frameShadow() const
{
    QFramePrivate * const d = d_func();
    return (Shadow) (d->frameStyle & Shadow_Mask);
}

void QFrame::setFrameShadow(QFrame::Shadow s)
{
    QFramePrivate * const d = d_func();
    setFrameStyle((d->frameStyle & Shape_Mask) | s);
}

void QFrame::setFrameStyle(int style)
{
    QFramePrivate * const d = d_func();
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp;
		// z
        switch (style & Shape_Mask) {
        case HLine:
            sp = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::Line);
            break;
        case VLine:
            sp = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum, QSizePolicy::Line);
            break;
        default:
            sp = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Frame);
        }
        setSizePolicy(sp);
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    d->frameStyle = (short)style;
    update();
    d->updateFrameWidth();
}

void QFrame::setLineWidth(int w)
{
    QFramePrivate * const d = d_func();
    if (short(w) == d->lineWidth)
        return;
    d->lineWidth = short(w);
    d->updateFrameWidth();
}

int QFrame::lineWidth() const
{
    QFramePrivate * const d = d_func();
    return d->lineWidth;
}

void QFrame::setMidLineWidth(int w)
{
    QFramePrivate * const d = d_func();
    if (short(w) == d->midLineWidth)
        return;
    d->midLineWidth = short(w);
    d->updateFrameWidth();
}

int QFrame::midLineWidth() const
{
    QFramePrivate * const d = d_func();
    return d->midLineWidth;
}

void QFramePrivate::updateStyledFrameWidths()
{
    QFrame * const q = q_func();
    QStyleOptionFrame opt;
    q->initStyleOption(&opt);

    QRect cr = q->style()->subElementRect(QStyle::SE_ShapedFrameContents, &opt, q);
    leftFrameWidth = cr.left() - opt.rect.left();
    topFrameWidth = cr.top() - opt.rect.top();
    rightFrameWidth = opt.rect.right() - cr.right(),
    bottomFrameWidth = opt.rect.bottom() - cr.bottom();
    frameWidth = qMax(qMax(leftFrameWidth, rightFrameWidth),
                      qMax(topFrameWidth, bottomFrameWidth));
}


void QFramePrivate::updateFrameWidth()
{
    QFrame * const q = q_func();
    QRect fr = q->frameRect(); // R_content - frame
    updateStyledFrameWidths();
    q->setFrameRect(fr);  // 在其内,+frame_width
	
	// call它 会最终导致q->updateGeometry
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
}

int QFrame::frameWidth() const
{
    QFramePrivate * const d = d_func();
    return d->frameWidth;
}

QRect QFrame::frameRect() const
{
    QFramePrivate * const d = d_func();
    QRect fr = contentsRect();
    fr.adjust(-d->leftFrameWidth, -d->topFrameWidth, d->rightFrameWidth, d->bottomFrameWidth);
    return fr;
}

void QFrame::setFrameRect(const QRect &r)
{
    QFramePrivate * const d = d_func();
    QRect cr = r.isValid() ? r : rect();
    cr.adjust(d->leftFrameWidth, d->topFrameWidth, -d->rightFrameWidth, -d->bottomFrameWidth);
	
	// base class
    setContentsMargins(cr.left(), cr.top(), rect().right() - cr.right(), rect().bottom() - cr.bottom());
}

QSize QFrame::sizeHint() const
{
    QFramePrivate * const d = d_func();
    switch (d->frameStyle & Shape_Mask) {
    case HLine:
        return QSize(-1,3);
    case VLine:
        return QSize(3,-1);
    default:
        return QWidget::sizeHint();
    }
}

void QFrame::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    drawFrame(&paint);
}

void QFrame::drawFrame(QPainter *p)
{
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    style()->drawControl(QStyle::CE_ShapedFrame, &opt, p, this);
}

void QFrame::changeEvent(QEvent *ev)
{
    QFramePrivate * const d = d_func();
    if (ev->type() == QEvent::StyleChange)
        d->updateFrameWidth();  // 进而重绘边框
    QWidget::changeEvent(ev);
}

bool QFrame::event(QEvent *e)
{
    if (e->type() == QEvent::ParentChange)
        d_func()->updateFrameWidth();
    bool result = QWidget::event(e);
    //this has to be done after the widget has been polished
    if (e->type() == QEvent::Polish)
        d_func()->updateFrameWidth();
    return result;
}

#include "moc_qframe.cpp"

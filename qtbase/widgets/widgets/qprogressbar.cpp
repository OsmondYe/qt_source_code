#include "qprogressbar.h"

#include <qlocale.h>
#include <qevent.h>
#include <qpainter.h>
#include <qstylepainter.h>
#include <qstyleoption.h>
#include <private/qwidget_p.h>
#ifndef QT_NO_ACCESSIBILITY
#include <qaccessible.h>
#endif
#include <limits.h>

class QProgressBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QProgressBar)

public:
    QProgressBarPrivate();

    void init();
    void initDefaultFormat();
    inline void resetLayoutItemMargins();

    int minimum;
    int maximum;
    int value;
    Qt::Alignment alignment;
    uint textVisible : 1;
    uint defaultFormat: 1;
    int lastPaintedValue;
    Qt::Orientation orientation;
    bool invertedAppearance;
    QProgressBar::Direction textDirection;
    QString format;
    inline int bound(int val) const { return qMax(minimum-1, qMin(maximum, val)); }
    bool repaintRequired() const;
};

QProgressBarPrivate::QProgressBarPrivate()
    : minimum(0), maximum(100), value(-1), alignment(Qt::AlignLeft), textVisible(true),
      defaultFormat(true), lastPaintedValue(-1), orientation(Qt::Horizontal), invertedAppearance(false),
      textDirection(QProgressBar::TopToBottom)
{
    initDefaultFormat();
}

void QProgressBarPrivate::initDefaultFormat()
{
    if (defaultFormat)
        format = QLatin1String("%p") + locale.percent();
}

void QProgressBarPrivate::init()
{
    Q_Q(QProgressBar);
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    resetLayoutItemMargins();
}

void QProgressBarPrivate::resetLayoutItemMargins()
{
    Q_Q(QProgressBar);
    QStyleOptionProgressBar option;
    q->initStyleOption(&option);
    setLayoutItemMargins(QStyle::SE_ProgressBarLayoutItem, &option);
}

/*!
    Initialize \a option with the values from this QProgressBar. This method is useful
    for subclasses when they need a QStyleOptionProgressBar,
    but don't want to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QProgressBar::initStyleOption(QStyleOptionProgressBar *option) const
{
    if (!option)
        return;
    Q_D(const QProgressBar);
    option->initFrom(this);

    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    option->minimum = d->minimum;
    option->maximum = d->maximum;
    option->progress = d->value;
    option->textAlignment = d->alignment;
    option->textVisible = d->textVisible;
    option->text = text();
    option->orientation = d->orientation;  // ### Qt 6: remove this member from QStyleOptionProgressBar
    option->invertedAppearance = d->invertedAppearance;
    option->bottomToTop = d->textDirection == QProgressBar::BottomToTop;
}

bool QProgressBarPrivate::repaintRequired() const
{
    Q_Q(const QProgressBar);
    if (value == lastPaintedValue)
        return false;

    const auto valueDifference = qAbs(qint64(value) - lastPaintedValue);
    // Check if the text needs to be repainted
    if (value == minimum || value == maximum)
        return true;

    const auto totalSteps = qint64(maximum) - minimum;
    if (textVisible) {
        if ((format.contains(QLatin1String("%v"))))
            return true;
        if ((format.contains(QLatin1String("%p"))
             && valueDifference >= qAbs(totalSteps / 100)))
            return true;
    }

    // Check if the bar needs to be repainted
    QStyleOptionProgressBar opt;
    q->initStyleOption(&opt);
    int cw = q->style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &opt, q);
    QRect groove = q->style()->subElementRect(QStyle::SE_ProgressBarGroove, &opt, q);
    // This expression is basically
    // (valueDifference / (maximum - minimum) > cw / groove.width())
    // transformed to avoid integer division.
    int grooveBlock = (q->orientation() == Qt::Horizontal) ? groove.width() : groove.height();
    return valueDifference * grooveBlock > cw * totalSteps;
}

QProgressBar::QProgressBar(QWidget *parent)
    : QWidget(*(new QProgressBarPrivate), parent, 0)
{
    d_func()->init();
}

/*!
    Destructor.
*/
QProgressBar::~QProgressBar()
{
}

/*!
    Reset the progress bar. The progress bar "rewinds" and shows no
    progress.
*/

void QProgressBar::reset()
{
    Q_D(QProgressBar);
    if (d->minimum == INT_MIN)
        d->value = INT_MIN;
    else
        d->value = d->minimum - 1;
    repaint();
}

/*!
    \property QProgressBar::minimum
    \brief the progress bar's minimum value

    When setting this property, the \l maximum is adjusted if
    necessary to ensure that the range remains valid. If the
    current value falls outside the new range, the progress bar is reset
    with reset().
*/
void QProgressBar::setMinimum(int minimum)
{
    setRange(minimum, qMax(d_func()->maximum, minimum));
}

int QProgressBar::minimum() const
{
    return d_func()->minimum;
}


/*!
    \property QProgressBar::maximum
    \brief the progress bar's maximum value

    When setting this property, the \l minimum is adjusted if
    necessary to ensure that the range remains valid. If the
    current value falls outside the new range, the progress bar is reset
    with reset().
*/

void QProgressBar::setMaximum(int maximum)
{
    setRange(qMin(d_func()->minimum, maximum), maximum);
}

int QProgressBar::maximum() const
{
    return d_func()->maximum;
}

/*!
    \property QProgressBar::value
    \brief the progress bar's current value

    Attempting to change the current value to one outside
    the minimum-maximum range has no effect on the current value.
*/
void QProgressBar::setValue(int value)
{
    Q_D(QProgressBar);
    if (d->value == value
            || ((value > d->maximum || value < d->minimum)
                && (d->maximum != 0 || d->minimum != 0)))
        return;
    d->value = value;
    emit valueChanged(value);
#ifndef QT_NO_ACCESSIBILITY
    if (isVisible()) {
        QAccessibleValueChangeEvent event(this, value);
        QAccessible::updateAccessibility(&event);
    }
#endif
    if (d->repaintRequired())
        repaint();
}

int QProgressBar::value() const
{
    return d_func()->value;
}

/*!
    Sets the progress bar's minimum and maximum values to \a minimum and
    \a maximum respectively.

    If \a maximum is smaller than \a minimum, \a minimum becomes the only
    legal value.

    If the current value falls outside the new range, the progress bar is reset
    with reset().

    The QProgressBar can be set to undetermined state by using setRange(0, 0).

    \sa minimum, maximum
*/
void QProgressBar::setRange(int minimum, int maximum)
{
    Q_D(QProgressBar);
    if (minimum != d->minimum || maximum != d->maximum) {
        d->minimum = minimum;
        d->maximum = qMax(minimum, maximum);

        if (d->value < qint64(d->minimum) - 1 || d->value > d->maximum)
            reset();
        else
            update();
    }
}

/*!
    \property QProgressBar::textVisible
    \brief whether the current completed percentage should be displayed

    This property may be ignored by the style (e.g., QMacStyle never draws the text).

    \sa textDirection
*/
void QProgressBar::setTextVisible(bool visible)
{
    Q_D(QProgressBar);
    if (d->textVisible != visible) {
        d->textVisible = visible;
        repaint();
    }
}

bool QProgressBar::isTextVisible() const
{
    return d_func()->textVisible;
}

/*!
    \property QProgressBar::alignment
    \brief the alignment of the progress bar
*/
void QProgressBar::setAlignment(Qt::Alignment alignment)
{
    if (d_func()->alignment != alignment) {
        d_func()->alignment = alignment;
        repaint();
    }
}

Qt::Alignment QProgressBar::alignment() const
{
    return d_func()->alignment;
}

/*!
    \reimp
*/
void QProgressBar::paintEvent(QPaintEvent *)
{
    QStylePainter paint(this);
    QStyleOptionProgressBar opt;
    initStyleOption(&opt);
    paint.drawControl(QStyle::CE_ProgressBar, opt);
    d_func()->lastPaintedValue = d_func()->value;
}

/*!
    \reimp
*/
QSize QProgressBar::sizeHint() const
{
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    QStyleOptionProgressBar opt;
    initStyleOption(&opt);
    int cw = style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &opt, this);
    QSize size = QSize(qMax(9, cw) * 7 + fm.width(QLatin1Char('0')) * 4, fm.height() + 8);
    if (opt.orientation == Qt::Vertical)
        size = size.transposed();
    return style()->sizeFromContents(QStyle::CT_ProgressBar, &opt, size, this);
}

/*!
    \reimp
*/
QSize QProgressBar::minimumSizeHint() const
{
    QSize size;
    if (orientation() == Qt::Horizontal)
        size = QSize(sizeHint().width(), fontMetrics().height() + 2);
    else
        size = QSize(fontMetrics().height() + 2, sizeHint().height());
    return size;
}

/*!
    \property QProgressBar::text
    \brief the descriptive text shown with the progress bar

    The text returned is the same as the text displayed in the center
    (or in some styles, to the left) of the progress bar.

    The progress shown in the text may be smaller than the minimum value,
    indicating that the progress bar is in the "reset" state before any
    progress is set.

    In the default implementation, the text either contains a percentage
    value that indicates the progress so far, or it is blank because the
    progress bar is in the reset state.
*/
QString QProgressBar::text() const
{
    Q_D(const QProgressBar);
    if ((d->maximum == 0 && d->minimum == 0) || d->value < d->minimum
            || (d->value == INT_MIN && d->minimum == INT_MIN))
        return QString();

    qint64 totalSteps = qint64(d->maximum) - d->minimum;

    QString result = d->format;
    QLocale locale = d->locale; // Omit group separators for compatibility with previous versions that were non-localized.
    locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
    result.replace(QLatin1String("%m"), locale.toString(totalSteps));
    result.replace(QLatin1String("%v"), locale.toString(d->value));

    // If max and min are equal and we get this far, it means that the
    // progress bar has one step and that we are on that step. Return
    // 100% here in order to avoid division by zero further down.
    if (totalSteps == 0) {
        result.replace(QLatin1String("%p"), locale.toString(100));
        return result;
    }

    const auto progress = static_cast<int>((qint64(d->value) - d->minimum) * 100.0 / totalSteps);
    result.replace(QLatin1String("%p"), locale.toString(progress));
    return result;
}


void QProgressBar::setOrientation(Qt::Orientation orientation)
{
    Q_D(QProgressBar);
    if (d->orientation == orientation)
        return;
    d->orientation = orientation;
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        setSizePolicy(sizePolicy().transposed());
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    d->resetLayoutItemMargins();
    update();
    updateGeometry();
}

Qt::Orientation QProgressBar::orientation() const
{
    Q_D(const QProgressBar);
    return d->orientation;
}

void QProgressBar::setInvertedAppearance(bool invert)
{
    Q_D(QProgressBar);
    d->invertedAppearance = invert;
    update();
}

bool QProgressBar::invertedAppearance() const
{
    Q_D(const QProgressBar);
    return d->invertedAppearance;
}

void QProgressBar::setTextDirection(QProgressBar::Direction textDirection)
{
    Q_D(QProgressBar);
    d->textDirection = textDirection;
    update();
}

QProgressBar::Direction QProgressBar::textDirection() const
{
    Q_D(const QProgressBar);
    return d->textDirection;
}

/*! \reimp */
bool QProgressBar::event(QEvent *e)
{
    Q_D(QProgressBar);
    switch (e->type()) {
    case QEvent::StyleChange:
        d->resetLayoutItemMargins();
        break;
    case QEvent::LocaleChange:
        d->initDefaultFormat();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void QProgressBar::setFormat(const QString &format)
{
    Q_D(QProgressBar);
    if (d->format == format)
        return;
    d->format = format;
    d->defaultFormat = false;
    update();
}

void QProgressBar::resetFormat()
{
    Q_D(QProgressBar);
    d->defaultFormat = true;
    d->initDefaultFormat();
    update();
}

QString QProgressBar::format() const
{
    Q_D(const QProgressBar);
    return d->format;
}

#include "moc_qprogressbar.cpp"

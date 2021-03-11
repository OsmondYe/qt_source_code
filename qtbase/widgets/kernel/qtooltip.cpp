#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>
#if QT_CONFIG(effects)
#include <private/qeffects_p.h>
#endif
#include <qtextdocument.h>
#include <qdebug.h>
#include <private/qstylesheetstyle_p.h>

#ifndef QT_NO_TOOLTIP
#include <qlabel.h>
#include <qtooltip.h>

class QTipLabel : public QLabel
{
    //Q_OBJECT
	static QTipLabel *instance;   // 隐藏了一个Singleton
	
	QWidget *styleSheetParent;
	QWidget *widget;
	QRect rect;
	
	QBasicTimer hideTimer, expireTimer;
	
	bool fadingOut;
public:
    QTipLabel(const QString &text, QWidget *w, int msecDisplayTime);
    ~QTipLabel(){ instance = 0;}

    bool eventFilter(QObject *, QEvent *) override;

    void reuseTip(const QString &text, int msecDisplayTime);
    void hideTip();
    void hideTipImmediately();
    void setTipRect(QWidget *w, const QRect &r);
    void restartExpireTimer(int msecDisplayTime);
    bool tipChanged(const QPoint &pos, const QString &text, QObject *o);
    void placeTip(const QPoint &pos, QWidget *w);

    static int getTipScreen(const QPoint &pos, QWidget *w);
protected:
    void timerEvent(QTimerEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

public slots:
    /** \internal
      Cleanup the _q_stylesheet_parent propery.
     */
    void styleSheetParentDestroyed() {
        setProperty("_q_stylesheet_parent", QVariant());
        styleSheetParent = 0;
    }

private:
};

QTipLabel *QTipLabel::instance = 0; 

QTipLabel::QTipLabel(const QString &text, QWidget *w, int msecDisplayTime)
    : QLabel(w, Qt::ToolTip | Qt::BypassGraphicsProxyWidget), styleSheetParent(0), widget(0)
{
    delete instance;
    instance = this;
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
    ensurePolished(); // 上色
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    qApp->installEventFilter(this);  // app级别安装filt   er 本类有 eventFilter 来处理
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
    setMouseTracking(true);
    fadingOut = false;
    reuseTip(text, msecDisplayTime);
}

void QTipLabel::restartExpireTimer(int msecDisplayTime)
{
    int time = 10000 + 40 * qMax(0, text().length()-100);
    if (msecDisplayTime > 0)
        time = msecDisplayTime;
    expireTimer.start(time, this);
    hideTimer.stop();
}

void QTipLabel::reuseTip(const QString &text, int msecDisplayTime)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (styleSheetParent){
        disconnect(styleSheetParent, SIGNAL(destroyed()),
                   QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
        styleSheetParent = 0;
    }
#endif

    setWordWrap(Qt::mightBeRichText(text));
    setText(text);
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();
    resize(sizeHint() + extra);
    restartExpireTimer(msecDisplayTime);
}

void QTipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);	// painter构造时需要QPaintDevice, 恰好QWidget本身就是QPaintDevice的子类
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

void QTipLabel::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(e);
}

void QTipLabel::mouseMoveEvent(QMouseEvent *e)
{
    if (rect.isNull())
        return;
    QPoint pos = e->globalPos();
    if (widget)
        pos = widget->mapFromGlobal(pos);
    if (!rect.contains(pos))
        hideTip();
    QLabel::mouseMoveEvent(e);
}

void QTipLabel::hideTip()
{
    if (!hideTimer.isActive())
        hideTimer.start(300, this);
}

void QTipLabel::hideTipImmediately()
{
    close(); // to trigger QEvent::Close which stops the animation
    deleteLater();
}

void QTipLabel::setTipRect(QWidget *w, const QRect &r)
{
    if (Q_UNLIKELY(!r.isNull() && !w)) {
        qWarning("QToolTip::setTipRect: Cannot pass null widget if rect is set");
        return;
    }
    widget = w;
    rect = r;
}

void QTipLabel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId()
        || e->timerId() == expireTimer.timerId())
    {
        hideTimer.stop();
        expireTimer.stop();
        hideTipImmediately();
    }
}

bool QTipLabel::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Leave:
        hideTip();
        break;

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Close: // For QTBUG-55523 (QQC) specifically: Hide tooltip when windows are closed
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        hideTipImmediately();
        break;

    case QEvent::MouseMove:
        if (o == widget && !rect.isNull() && !rect.contains(static_cast<QMouseEvent*>(e)->pos()))
            hideTip();
    default:
        break;
    }
    return false;
}

int QTipLabel::getTipScreen(const QPoint &pos, QWidget *w)
{
    if (QApplication::desktop()->isVirtualDesktop())
        return QApplication::desktop()->screenNumber(pos);
    else
        return QApplication::desktop()->screenNumber(w);
}

void QTipLabel::placeTip(const QPoint &pos, QWidget *w)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (testAttribute(Qt::WA_StyleSheet) || (w && qobject_cast<QStyleSheetStyle *>(w->style()))) {
        //the stylesheet need to know the real parent
        QTipLabel::instance->setProperty("_q_stylesheet_parent", QVariant::fromValue(w));
        //we force the style to be the QStyleSheetStyle, and force to clear the cache as well.
        QTipLabel::instance->setStyleSheet(QLatin1String("/* */"));

        // Set up for cleaning up this later...
        QTipLabel::instance->styleSheetParent = w;
        if (w) {
            connect(w, SIGNAL(destroyed()),
                QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
        }
    }
#endif //QT_NO_STYLE_STYLESHEET

	// 要哪一块屏幕的尺寸
    QRect screen = QApplication::desktop()->screenGeometry(getTipScreen(pos, w));


    QPoint p = pos;
    p += QPoint(2, 16);
    if (p.x() + this->width() > screen.x() + screen.width())
        p.rx() -= 4 + this->width();
    if (p.y() + this->height() > screen.y() + screen.height())
        p.ry() -= 24 + this->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + this->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - this->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + this->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - this->height());
    this->move(p);
}

bool QTipLabel::tipChanged(const QPoint &pos, const QString &text, QObject *o)
{
    if (QTipLabel::instance->text() != text)
        return true;

    if (o != widget)
        return true;

    if (!rect.isNull())
        return !rect.contains(pos);
    else
       return false;
}


void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect, int msecDisplayTime)
{
    if (QTipLabel::instance && QTipLabel::instance->isVisible()){ // a tip does already exist
        if (text.isEmpty()){ // empty text means hide current tip
            QTipLabel::instance->hideTip();
            return;
        }
        else if (!QTipLabel::instance->fadingOut){
            // If the tip has changed, reuse the one
            // that is showing (removes flickering)
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (QTipLabel::instance->tipChanged(localPos, text, w)){
                QTipLabel::instance->reuseTip(text, msecDisplayTime);
                QTipLabel::instance->setTipRect(w, rect);
                QTipLabel::instance->placeTip(pos, w);
            }
            return;
        }
    }

    if (!text.isEmpty()){ // no tip can be reused, create new tip:

        // On windows, we can't use the widget as parent otherwise the window will be
        // raised when the tooltip will be shown
        // oye 所以Windows上就直接把桌面作为QTipLabel的父widget 
        new QTipLabel(text, QApplication::desktop()->screen(QTipLabel::getTipScreen(pos, w)), msecDisplayTime);

        QTipLabel::instance->setTipRect(w, rect);
        QTipLabel::instance->placeTip(pos, w);
        QTipLabel::instance->setObjectName(QLatin1String("qtooltip_label"));

		// 上特效
        if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip))
            qFadeEffect(QTipLabel::instance);
        else if (QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))
            qScrollEffect(QTipLabel::instance);
        else
            QTipLabel::instance->showNormal();

    }
}


bool QToolTip::isVisible()
{
    return (QTipLabel::instance != 0 && QTipLabel::instance->isVisible());
}


QString QToolTip::text()
{
    if (QTipLabel::instance)
        return QTipLabel::instance->text();
    return QString();
}


//Q_GLOBAL_STATIC(QPalette, tooltip_palette)
QPalette tooltip_palette;


QPalette QToolTip::palette()
{
    return *tooltip_palette();
}


QFont QToolTip::font()
{
    return QApplication::font("QTipLabel");
}


void QToolTip::setPalette(const QPalette &palette)
{
    *tooltip_palette() = palette;
    if (QTipLabel::instance)
        QTipLabel::instance->setPalette(palette);
}


void QToolTip::setFont(const QFont &font)
{
    QApplication::setFont(font, "QTipLabel");
}

#include "qtooltip.moc"
#endif // QT_NO_TOOLTIP

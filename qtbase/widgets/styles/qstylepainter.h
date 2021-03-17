#ifndef QSTYLEPAINTER_H
#define QSTYLEPAINTER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qwidget.h>

// 相比painter, 暂存了widget和style
class QStylePainter : public QPainter
{
private:
    QWidget *widget;
    QStyle *wstyle;
public:
	inline void drawPrimitive(QStyle::PrimitiveElement pe, const QStyleOption &opt);
    inline void drawControl(QStyle::ControlElement ce, const QStyleOption &opt);
    inline void drawComplexControl(QStyle::ComplexControl cc, const QStyleOptionComplex &opt);
    inline void drawItemText(const QRect &r, int flags, const QPalette &pal, bool enabled,
                             const QString &text, QPalette::ColorRole textRole = QPalette::NoRole);
    inline void drawItemPixmap(const QRect &r, int flags, const QPixmap &pixmap);
    inline QStyle *style() const { return wstyle; }

public:
	// 让基类去默认构造, 这样基类就不会调用begin, 其被延迟到派生来来调用
    inline QStylePainter() : QPainter(), widget(Q_NULLPTR), wstyle(Q_NULLPTR) {}
    inline explicit QStylePainter(QWidget *w) { begin(w, w); }
    inline QStylePainter(QPaintDevice *pd, QWidget *w) { begin(pd, w); }
	inline bool begin(QWidget *w) { return begin(w, w); }
    inline bool begin(QPaintDevice *pd, QWidget *w) {
        widget = w;
        wstyle = w->style(); // widget本身自含有style
        return QPainter::begin(pd);
    };


    //Q_DISABLE_COPY(QStylePainter)
};

void QStylePainter::drawPrimitive(QStyle::PrimitiveElement pe, const QStyleOption &opt)
{
    wstyle->drawPrimitive(pe, &opt, this, widget);
}

void QStylePainter::drawControl(QStyle::ControlElement ce, const QStyleOption &opt)
{
    wstyle->drawControl(ce, &opt, this, widget);
}

void QStylePainter::drawComplexControl(QStyle::ComplexControl cc, const QStyleOptionComplex &opt)
{
    wstyle->drawComplexControl(cc, &opt, this, widget);
}

void QStylePainter::drawItemText(const QRect &r, int flags, const QPalette &pal, bool enabled,
                                 const QString &text, QPalette::ColorRole textRole)
{
    wstyle->drawItemText(this, r, flags, pal, enabled, text, textRole);
}

void QStylePainter::drawItemPixmap(const QRect &r, int flags, const QPixmap &pixmap)
{
    wstyle->drawItemPixmap(this, r, flags, pixmap);
}


#endif // QSTYLEPAINTER_H

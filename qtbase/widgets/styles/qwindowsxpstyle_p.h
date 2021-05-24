#ifndef QWINDOWSXPSTYLE_P_H
#define QWINDOWSXPSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <private/qwindowsstyle_p.h>


class QWindowsXPStylePrivate;
class QWindowsXPStyle : public QWindowsStyle
{
    //Q_OBJECT
public:
    QWindowsXPStyle();
    QWindowsXPStyle(QWindowsXPStylePrivate &dd);
    ~QWindowsXPStyle();

    void unpolish(QApplication*);
    void polish(QApplication*);
    void polish(QWidget*);
    void polish(QPalette&);
    void unpolish(QWidget*);

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *option, QPainter *p,
                       const QWidget *widget = 0) const;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *p,
                     const QWidget *wwidget = 0) const;
    QRect subElementRect(SubElement r, const QStyleOption *option, const QWidget *widget = 0) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *option, SubControl sc,
                         const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option, QPainter *p,
                            const QWidget *widget = 0) const;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *option, const QSize &contentsSize,
                           const QWidget *widget = 0) const;
    int pixelMetric(PixelMetric pm, const QStyleOption *option = 0,
                    const QWidget *widget = 0) const;
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const;

    QPalette standardPalette() const;
    QPixmap standardPixmap(StandardPixmap standardIcon, const QStyleOption *option,
                           const QWidget *widget = 0) const;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const;

private:
    //Q_DISABLE_COPY(QWindowsXPStyle)
    //Q_DECLARE_PRIVATE(QWindowsXPStyle)
    friend class QStyleFactory;
};

#endif // QWINDOWSXPSTYLE_P_H

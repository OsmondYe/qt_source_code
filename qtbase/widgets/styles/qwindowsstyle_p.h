#ifndef QWINDOWSSTYLE_P_H
#define QWINDOWSSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>

QT_BEGIN_NAMESPACE

class QWindowsStylePrivate;

class QWindowsStyle : public QCommonStyle
{
    //Q_OBJECT
public:
    QWindowsStyle();
    ~QWindowsStyle();

    void polish(QApplication*) Q_DECL_OVERRIDE;
    void unpolish(QApplication*) Q_DECL_OVERRIDE;

    void polish(QWidget*) Q_DECL_OVERRIDE;
    void unpolish(QWidget*) Q_DECL_OVERRIDE;

    void polish(QPalette &) Q_DECL_OVERRIDE;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const Q_DECL_OVERRIDE;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const Q_DECL_OVERRIDE;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const Q_DECL_OVERRIDE;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const Q_DECL_OVERRIDE;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = 0) const Q_DECL_OVERRIDE;

    int pixelMetric(PixelMetric pm, const QStyleOption *option = 0, const QWidget *widget = 0) const Q_DECL_OVERRIDE;

    int styleHint(StyleHint hint, const QStyleOption *opt = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const Q_DECL_OVERRIDE;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = 0) const Q_DECL_OVERRIDE;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const Q_DECL_OVERRIDE;

protected:
    bool eventFilter(QObject *o, QEvent *e) Q_DECL_OVERRIDE;
    QWindowsStyle(QWindowsStylePrivate &dd);

private:
    //Q_DISABLE_COPY(QWindowsStyle)
    //Q_DECLARE_PRIVATE(QWindowsStyle)
};



QT_END_NAMESPACE

#endif // QWINDOWSSTYLE_P_H

#ifndef QWINDOWSSTYLE_P_H
#define QWINDOWSSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>

class QWindowsStylePrivate;

// win os 上的样式基类
class QWindowsStyle : public QCommonStyle
{
    //Q_OBJECT
public:
    QWindowsStyle();
    ~QWindowsStyle();

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const override;
	
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = 0) const override;

    int pixelMetric(PixelMetric pm, const QStyleOption *option = 0, const QWidget *widget = 0) const override;

    int styleHint(StyleHint hint, const QStyleOption *opt = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const override;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = 0) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const override;

	void polish(QApplication*) override;
	void unpolish(QApplication*) override;

	void polish(QWidget*) override;
	void unpolish(QWidget*) override;

	void polish(QPalette &) override;

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    QWindowsStyle(QWindowsStylePrivate &dd);

private:
    //Q_DISABLE_COPY(QWindowsStyle)
    //Q_DECLARE_PRIVATE(QWindowsStyle)
};



#endif // QWINDOWSSTYLE_P_H

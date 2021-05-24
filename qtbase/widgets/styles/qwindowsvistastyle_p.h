#ifndef QWINDOWSVISTASTYLE_P_H
#define QWINDOWSVISTASTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <private/qwindowsxpstyle_p.h>


class QWindowsVistaStylePrivate;
class QWindowsVistaStyle : public QWindowsXPStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = 0) const;
	//
    void drawControl(ControlElement element, 
					 const QStyleOption *option,
                     QPainter *painter, 
                     const QWidget *widget
                     ) override const;

	/*
		ComplexControl 要画的控件类别, 静态预定义
		option, 类对象自己对 控件一些尺寸,样式等的数据规定
					 */
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) override const;
	
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const;

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &pos, const QWidget *widget = 0) const;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = 0) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;
	
	int styleHint(StyleHint hint, const QStyleOption *opt = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const;

    void polish(QWidget *widget);
    void unpolish(QWidget *widget);
    void polish(QPalette &pal);
    void polish(QApplication *app);
    void unpolish(QApplication *app);
    QPalette standardPalette() const;
	QWindowsVistaStyle();
	~QWindowsVistaStyle();
private:
    //Q_DISABLE_COPY(QWindowsVistaStyle)
    //Q_DECLARE_PRIVATE(QWindowsVistaStyle)
    //friend class QStyleFactory;
};

#endif // QWINDOWSVISTASTYLE_P_H

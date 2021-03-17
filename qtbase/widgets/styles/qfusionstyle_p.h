#ifndef QFUSIONSTYLE_P_H
#define QFUSIONSTYLE_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>

QT_BEGIN_NAMESPACE


#if QT_CONFIG(style_fusion)

class QFusionStylePrivate;
class QFusionStyle : public QCommonStyle
{
    //Q_OBJECT
    //Q_DECLARE_PRIVATE(QFusionStyle)

public:
    QFusionStyle();
    ~QFusionStyle();

    QPalette standardPalette () const override;
    void drawPrimitive(PrimitiveElement elem,
                       const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = 0) const override;
    void drawControl(ControlElement ce, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = 0) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const override;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = 0) const override;
    void drawItemPixmap(QPainter *painter, const QRect &rect,
                        int alignment, const QPixmap &pixmap) const override;
    void drawItemText(QPainter *painter, const QRect &rect,
                      int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
    void polish(QWidget *widget) override;
    void polish(QApplication *app) override;
    void polish(QPalette &pal) override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

protected:
    QFusionStyle(QFusionStylePrivate &dd);

};

#endif // style_fusion

QT_END_NAMESPACE

#endif //QFUSIONSTYLE_P_H

#ifndef QCOMMONSTYLE_H
#define QCOMMONSTYLE_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

class QCommonStylePrivate;

class  QCommonStyle: public QStyle
{
    

public:
    QCommonStyle();
    ~QCommonStyle();

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = Q_NULLPTR) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = Q_NULLPTR) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = Q_NULLPTR) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = Q_NULLPTR) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = Q_NULLPTR) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = Q_NULLPTR) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = Q_NULLPTR) const override;

    int pixelMetric(PixelMetric m, const QStyleOption *opt = Q_NULLPTR, const QWidget *widget = Q_NULLPTR) const override;

    int styleHint(StyleHint sh, const QStyleOption *opt = Q_NULLPTR, const QWidget *w = Q_NULLPTR,
                  QStyleHintReturn *shret = Q_NULLPTR) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = Q_NULLPTR,
                       const QWidget *widget = Q_NULLPTR) const override;
    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt = Q_NULLPTR,
                           const QWidget *widget = Q_NULLPTR) const override;

    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const override;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                      Qt::Orientation orientation, const QStyleOption *option = Q_NULLPTR,
                      const QWidget *widget = Q_NULLPTR) const override;

    void polish(QPalette &) override;
    void polish(QApplication *app) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *application) override;

protected:
    QCommonStyle(QCommonStylePrivate &dd);

private:
    Q_DECLARE_PRIVATE(QCommonStyle)
    Q_DISABLE_COPY(QCommonStyle)
#ifndef QT_NO_ANIMATION
    Q_PRIVATE_SLOT(d_func(), void _q_removeAnimation())
#endif
};

QT_END_NAMESPACE

#endif // QCOMMONSTYLE_H

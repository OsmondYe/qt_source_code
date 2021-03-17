#ifndef QPROXYSTYLE_H
#define QPROXYSTYLE_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/QCommonStyle>

QT_BEGIN_NAMESPACE

class QProxyStylePrivate;
class  QProxyStyle : public QCommonStyle
{
    //Q_OBJECT

public:
    QProxyStyle(QStyle *style = Q_NULLPTR);
    QProxyStyle(const QString &key);
    ~QProxyStyle();

    QStyle *baseStyle() const;
    void setBaseStyle(QStyle *style);

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
    void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
    virtual void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override;
    QRect itemTextRect(const QFontMetrics &fm, const QRect &r, int flags, bool enabled, const QString &text) const override;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const override;

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option, const QPoint &pos, const QWidget *widget = Q_NULLPTR) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = Q_NULLPTR, const QWidget *widget = Q_NULLPTR, QStyleHintReturn *returnData = Q_NULLPTR) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = Q_NULLPTR, const QWidget *widget = Q_NULLPTR) const override;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                      Qt::Orientation orientation, const QStyleOption *option = Q_NULLPTR, const QWidget *widget = Q_NULLPTR) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = Q_NULLPTR, const QWidget *widget = Q_NULLPTR) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget = Q_NULLPTR) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const override;
    QPalette standardPalette() const override;

    void polish(QWidget *widget) override;
    void polish(QPalette &pal) override;
    void polish(QApplication *app) override;

    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

protected:
    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QProxyStyle)
    Q_DECLARE_PRIVATE(QProxyStyle)
};

QT_END_NAMESPACE

#endif // QPROXYSTYLE_H

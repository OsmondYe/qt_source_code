#ifndef QWINDOWSSTYLE_P_P_H
#define QWINDOWSSTYLE_P_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qwindowsstyle_p.h"
#include "qcommonstyle_p.h"

class QTime;

class QWindowsStylePrivate : public QCommonStylePrivate
{
    //Q_DECLARE_PUBLIC(QWindowsStyle)
public:
    bool alt_down;
    QList<const QWidget *> seenAlt;
    int menuBarTimer;

    QColor inactiveCaptionText;
    QColor activeCaptionColor;
    QColor activeGradientCaptionColor;
    QColor inactiveCaptionColor;
    QColor inactiveGradientCaptionColor;



    QWindowsStylePrivate();
    static int pixelMetricFromSystemDp(QStyle::PixelMetric pm, 
		const QStyleOption *option = 0, const QWidget *widget = 0);
    static int fixedPixelMetric(QStyle::PixelMetric pm);
    static qreal devicePixelRatio(const QWidget *widget = 0)
        { return widget ? widget->devicePixelRatioF() : QWindowsStylePrivate::appDevicePixelRatio(); }
    static qreal nativeMetricScaleFactor(const QWidget *widget = Q_NULLPTR);

    bool hasSeenAlt(const QWidget *widget) const;
    bool altDown() const { return alt_down; }

    enum { InvalidMetric = -23576 };

    enum {
        windowsItemFrame        =  2, // menu item frame width
        windowsSepHeight        =  9, // separator item height
        windowsItemHMargin      =  3, // menu item hor text margin
        windowsItemVMargin      =  2, // menu item ver text margin
        windowsArrowHMargin     =  6, // arrow horizontal margin
        windowsRightBorder      = 15, // right border on windows
        windowsCheckMarkWidth   = 12  // checkmarks width on windows
    };

private:
    static qreal appDevicePixelRatio();
};

#endif //QWINDOWSSTYLE_P_P_H
;

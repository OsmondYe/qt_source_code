#ifndef QFRAME_P_H
#define QFRAME_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qwidget_p.h"
#include "qframe.h"

QT_BEGIN_NAMESPACE

// ### unexport this class when and if QAbstractScrollAreaPrivate is unexported
class Q_WIDGETS_EXPORT QFramePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QFrame)
public:
    QFramePrivate();
    ~QFramePrivate();

    void        updateFrameWidth();
    void        updateStyledFrameWidths();

    QRect       frect;
    int         frameStyle;
    short       lineWidth;
    short       midLineWidth;
    short       frameWidth;
    short       leftFrameWidth, rightFrameWidth;
    short       topFrameWidth, bottomFrameWidth;

    inline void init();

};

QT_END_NAMESPACE

#endif // QFRAME_P_H

#ifndef QCOMMONSTYLE_P_H
#define QCOMMONSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qcommonstyle.h"
#include "qstyle_p.h"
#include "qstyleanimation_p.h"

#include "qstyleoption.h"


class QStringList;

// oye ItemView + Animation
// 
class QCommonStylePrivate : public QStylePrivate
{
    //Q_DECLARE_PUBLIC(QCommonStyle)
public:
    inline QCommonStylePrivate() :
    cachedOption(0),
    animationFps(30)
    { }

    ~QCommonStylePrivate()
    {
        qDeleteAll(animations);
        delete cachedOption;
    }

#if QT_CONFIG(itemviews)
    void viewItemDrawText(QPainter *p, const QStyleOptionViewItem *option, const QRect &rect) const;
    void viewItemLayout(const QStyleOptionViewItem *opt,  QRect *checkRect,
                        QRect *pixmapRect, QRect *textRect, bool sizehint) const;
    QSize viewItemSize(const QStyleOptionViewItem *option, int role) const;

    mutable QRect decorationRect, displayRect, checkRect;
    mutable QStyleOptionViewItem *cachedOption;
    bool isViewItemCached(const QStyleOptionViewItem &option) const {
        return cachedOption && (option.widget == cachedOption->widget
               && option.index == cachedOption->index
               && option.state == cachedOption->state
               && option.rect == cachedOption->rect
               && option.text == cachedOption->text
               && option.direction == cachedOption->direction
               && option.displayAlignment == cachedOption->displayAlignment
               && option.decorationAlignment == cachedOption->decorationAlignment
               && option.decorationPosition == cachedOption->decorationPosition
               && option.decorationSize == cachedOption->decorationSize
               && option.features == cachedOption->features
               && option.icon.isNull() == cachedOption->icon.isNull()
               && option.font == cachedOption->font
               && option.viewItemPosition == cachedOption->viewItemPosition);
    }
#endif
    mutable QIcon tabBarcloseButtonIcon;

#if QT_CONFIG(tabbar)
    void tabLayout(const QStyleOptionTab *opt, const QWidget *widget, QRect *textRect, QRect *pixmapRect) const;
#endif

    int animationFps;
#ifndef QT_NO_ANIMATION
    void _q_removeAnimation();

    QList<const QObject*> animationTargets() const;
    QStyleAnimation* animation(const QObject *target) const;
    void startAnimation(QStyleAnimation *animation) const;
    void stopAnimation(const QObject *target) const;

private:
    mutable QHash<const QObject*, QStyleAnimation*> animations;
#endif // QT_NO_ANIMATION
};

#endif //QCOMMONSTYLE_P_H

#ifndef QLAYOUT_P_H
#define QLAYOUT_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qobject_p.h"
#include "qstyle.h"
#include "qsizepolicy.h"


class QWidgetItem;
class QSpacerItem;
class QLayoutItem;

class  QLayoutPrivate : public QObjectPrivate
{
	//Q_DECLARE_PUBLIC(QLayout)
	inline QLayout* q_func() { return static_cast<QLayout *>(q_ptr); }
public:
	int insideSpacing;
    int userLeftMargin;
    int userTopMargin;
    int userRightMargin;
    int userBottomMargin;
    uint topLevel : 1;						// oye toplevel layout ÒâÎ¶×Åthis->parent()ÊÇ widget
    uint enabled : 1;
    uint activated : 1;
    uint autoNewChild : 1;
    QLayout::SizeConstraint constraint;
    QRect rect;
    QWidget *menubar;
	
   
    QLayoutPrivate();

    void getMargin(int *result, int userMargin, QStyle::PixelMetric pm) const;
    void doResize(const QSize &);  // OnEvent_Resize
    void reparentChildWidgets(QWidget *mw);
    bool checkWidget(QWidget *widget) const;
    bool checkLayout(QLayout *otherLayout) const;

    static QWidgetItem *createWidgetItem(const QLayout *layout, QWidget *widget);
    static QSpacerItem *createSpacerItem(const QLayout *layout, int w, int h, QSizePolicy::Policy hPolicy = QSizePolicy::Minimum, QSizePolicy::Policy vPolicy = QSizePolicy::Minimum);
    virtual QLayoutItem* replaceAt(int index, QLayoutItem *newitem) { Q_UNUSED(index); Q_UNUSED(newitem); return 0; }




};


#endif // QLAYOUT_P_H

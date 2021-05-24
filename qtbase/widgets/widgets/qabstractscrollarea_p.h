#ifndef QABSTRACTSCROLLAREA_P_H
#define QABSTRACTSCROLLAREA_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qframe_p.h"
#include "qabstractscrollarea.h"

class QScrollBar;
class QAbstractScrollAreaScrollBarContainer;

class  QAbstractScrollAreaPrivate: public QFramePrivate
{
	//Q_DECLARE_PUBLIC(QAbstractScrollArea)
    inline QAbstractScrollArea* q_func() { return static_cast<QAbstractScrollArea *>(q_ptr); } \
	inline const QAbstractScrollArea* q_func() const { return static_cast<const QAbstractScrollArea *>(q_ptr); } \
	friend class QAbstractScrollArea;

public:
    QAbstractScrollAreaPrivate();
    ~QAbstractScrollAreaPrivate();

    void replaceScrollBar(QScrollBar *scrollBar, Qt::Orientation orientation);

    QAbstractScrollAreaScrollBarContainer *scrollBarContainers[Qt::Vertical + 1];
    QScrollBar *hbar, *vbar;
    Qt::ScrollBarPolicy vbarpolicy, hbarpolicy;

    bool shownOnce;
    bool inResize;
    mutable QSize sizeHint;
    QAbstractScrollArea::SizeAdjustPolicy sizeAdjustPolicy;

    QWidget *viewport;
    QWidget *cornerWidget;
    QRect cornerPaintingRect;
    int left, top, right, bottom; // viewport margin

    int xoffset, yoffset;
    QPoint overshoot;

    void init();
    void layoutChildren();
    void layoutChildren_helper(bool *needHorizontalScrollbar, bool *needVerticalScrollbar);
    // ### Fix for 4.4, talk to Bjoern E or Girish.
    virtual void scrollBarPolicyChanged(Qt::Orientation, Qt::ScrollBarPolicy) {}
    bool canStartScrollingAt( const QPoint &startPos );

    void flashScrollBars();
    void setScrollBarTransient(QScrollBar *scrollBar, bool transient);

    void _q_hslide(int);
    void _q_vslide(int);
    void _q_showOrHideScrollBars();

    virtual QPoint contentsOffset() const;

    inline bool viewportEvent(QEvent *event)
    { return q_func()->viewportEvent(event); }
    QScopedPointer<QObject> viewportFilter;


};

class QAbstractScrollAreaFilter : public QObject
{
    //Q_OBJECT
public:
    QAbstractScrollAreaFilter(QAbstractScrollAreaPrivate *p) : d(p)
    { setObjectName(QLatin1String("qt_abstractscrollarea_filter")); }
    bool eventFilter(QObject *o, QEvent *e) Q_DECL_OVERRIDE
    { return (o == d->viewport ? d->viewportEvent(e) : false); }
private:
    QAbstractScrollAreaPrivate *d;
};

// 像信封一样, 包了scrollBar, 同时自己搞了一个layout, 
// 把scrollBar 送入layout 
// 因为有了layout的存在, 其也可以继续容纳其它 widget
class QAbstractScrollAreaScrollBarContainer : public QWidget
{
public:

    QScrollBar *scrollBar;
    QBoxLayout *layout;
    Qt::Orientation orientation;
	

    enum LogicalPosition { LogicalLeft = 1, LogicalRight = 2 };

    QAbstractScrollAreaScrollBarContainer(Qt::Orientation orientation, QWidget *parent);
    void addWidget(QWidget *widget, LogicalPosition position);
    QWidgetList widgets(LogicalPosition position);
    void removeWidget(QWidget *widget);
    int scrollBarLayoutIndex() const;



};

#endif // QABSTRACTSCROLLAREA_P_H

#ifndef QDYNAMICMAINWINDOWLAYOUT_P_H
#define QDYNAMICMAINWINDOWLAYOUT_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qmainwindow.h"

#include "QtWidgets/qlayout.h"
#if QT_CONFIG(tabbar)
#include "QtWidgets/qtabbar.h"
#endif
#include "QtCore/qvector.h"
#include "QtCore/qset.h"
#include "QtCore/qbasictimer.h"
#include "private/qlayoutengine_p.h"
#include "private/qwidgetanimator_p.h"

#if QT_CONFIG(dockwidget)
#include "qdockarealayout_p.h"
#endif
#include "qtoolbararealayout_p.h"

//QT_REQUIRE_CONFIG(mainwindow);

class QToolBar;
class QRubberBand;

#if QT_CONFIG(dockwidget)
class QDockWidgetGroupWindow : public QWidget
{
    //Q_OBJECT
public:
    explicit QDockWidgetGroupWindow(QWidget* parent = 0, Qt::WindowFlags f = 0)
        : QWidget(parent, f) {}
    QDockAreaLayoutInfo *layoutInfo() const;
    QDockWidget *topDockWidget() const;
    void destroyOrHideIfEmpty();
    void adjustFlags();
    bool hasNativeDecos() const;

protected:
    bool event(QEvent *) override;
    void paintEvent(QPaintEvent*) override;

private:
    QSize m_removedFrameSize;
};

// This item will be used in the layout for the gap item. We cannot use QWidgetItem directly
// because QWidgetItem functions return an empty size for widgets that are are floating.
class QDockWidgetGroupWindowItem : public QWidgetItem
{
public:
    explicit QDockWidgetGroupWindowItem(QDockWidgetGroupWindow *parent) : QWidgetItem(parent) {}
    QSize minimumSize() const override { return lay()->minimumSize(); }
    QSize maximumSize() const override { return lay()->maximumSize(); }
    QSize sizeHint() const override { return lay()->sizeHint(); }

private:
    QLayout *lay() const { return const_cast<QDockWidgetGroupWindowItem *>(this)->widget()->layout(); }
};
#endif

/* This data structure represents the state of all the tool-bars and dock-widgets. It's value based
   so it can be easilly copied into a temporary variable. All operations are performed without moving
   any widgets. Only when we are sure we have the desired state, we call apply(), which moves the
   widgets.
*/

class QMainWindowLayoutState
{
public:
    QRect rect;
    QMainWindow *mainWindow;

    QMainWindowLayoutState(QMainWindow *win);

#ifndef QT_NO_TOOLBAR
    QToolBarAreaLayout toolBarAreaLayout;
#endif

#if QT_CONFIG(dockwidget)
    QDockAreaLayout dockAreaLayout;
#else
    QLayoutItem *centralWidgetItem;
    QRect centralWidgetRect;
#endif

    void apply(bool animated);
    void deleteAllLayoutItems();
    void deleteCentralWidgetItem();

    QSize sizeHint() const;
    QSize minimumSize() const;
    void fitLayout();

    QLayoutItem *itemAt(int index, int *x) const;
    QLayoutItem *takeAt(int index, int *x);
    QList<int> indexOf(QWidget *widget) const;
    QLayoutItem *item(const QList<int> &path);
    QRect itemRect(const QList<int> &path) const;
    QRect gapRect(const QList<int> &path) const; // ### get rid of this, use itemRect() instead

    bool contains(QWidget *widget) const;

    void setCentralWidget(QWidget *widget);
    QWidget *centralWidget() const;

    QList<int> gapIndex(QWidget *widget, const QPoint &pos) const;
    bool insertGap(const QList<int> &path, QLayoutItem *item);
    void remove(const QList<int> &path);
    void remove(QLayoutItem *item);
    void clear();
    bool isValid() const;

    QLayoutItem *plug(const QList<int> &path);
    QLayoutItem *unplug(const QList<int> &path, QMainWindowLayoutState *savedState = 0);

    void saveState(QDataStream &stream) const;
    bool checkFormat(QDataStream &stream);
    bool restoreState(QDataStream &stream, const QMainWindowLayoutState &oldState);
};

class  QMainWindowLayout : public QLayout
{
    //Q_OBJECT
public:
	QMainWindowLayoutState layoutState, savedState;
	QMainWindow::DockOptions dockOptions;
	// tab bar
	QSet<QTabBar*> usedTabBars;
	QList<QTabBar*> unusedTabBars;
	bool verticalTabsEnabled;

    QSet<QWidget*> usedSeparatorWidgets;
    QList<QWidget*> unusedSeparatorWidgets;
    int sep; // separator extent

    QTabWidget::TabPosition tabPositions[4];
    QTabWidget::TabShape _tabShape;	

	QList<int> movingSeparator;
	QPoint movingSeparatorOrigin, movingSeparatorPos;
	QBasicTimer separatorMoveTimer;

    mutable QSize szHint;
    mutable QSize minSize;

    QWidgetAnimator widgetAnimator;
    QList<int> currentGapPos;
    QRect currentGapRect;
    QWidget *pluggingWidget;
#if QT_CONFIG(rubberband)
    QPointer<QRubberBand> gapIndicator;
#endif

public:
	QWidget *centralWidget() const;
	void setCentralWidget(QWidget *cw);
public:
	// 绝佳的操作, parentLayout会作为QMainWindowLayout的parent
	// MainWindow的做法是吧一个GridLayout传进来,
    QMainWindowLayout(QMainWindow *mainwindow, QLayout *parentLayout);
    ~QMainWindowLayout();
    
    void setDockOptions(QMainWindow::DockOptions opts);
    bool usesHIToolBar(QToolBar *toolbar) const;

    void timerEvent(QTimerEvent *e) override;

    // status bar

    QLayoutItem *statusbar;

#if QT_CONFIG(statusbar)
    QStatusBar *statusBar() const;
    void setStatusBar(QStatusBar *sb);
#endif
#ifndef QT_NO_TOOLBAR
    void addToolBarBreak(Qt::ToolBarArea area);
    void insertToolBarBreak(QToolBar *before);
    void removeToolBarBreak(QToolBar *before);

    void addToolBar(Qt::ToolBarArea area, QToolBar *toolbar, bool needAddChildWidget = true);
    void insertToolBar(QToolBar *before, QToolBar *toolbar);
    Qt::ToolBarArea toolBarArea(QToolBar *toolbar) const;
    bool toolBarBreak(QToolBar *toolBar) const;
    void getStyleOptionInfo(QStyleOptionToolBar *option, QToolBar *toolBar) const;
    void removeToolBar(QToolBar *toolbar);
    void toggleToolBarsVisible();
    void moveToolBar(QToolBar *toolbar, int pos);
#endif
#if QT_CONFIG(dockwidget)
    void setCorner(Qt::Corner corner, Qt::DockWidgetArea area);
    Qt::DockWidgetArea corner(Qt::Corner corner) const;
    void addDockWidget(Qt::DockWidgetArea area,
                       QDockWidget *dockwidget,
                       Qt::Orientation orientation);
    void splitDockWidget(QDockWidget *after,
                         QDockWidget *dockwidget,
                         Qt::Orientation orientation);
    void tabifyDockWidget(QDockWidget *first, QDockWidget *second);
    Qt::DockWidgetArea dockWidgetArea(QWidget* widget) const;
    void raise(QDockWidget *widget);
    void setVerticalTabsEnabled(bool enabled);
    bool restoreDockWidget(QDockWidget *dockwidget);

#if QT_CONFIG(tabbar)
    QDockAreaLayoutInfo *dockInfo(QWidget *w);
    bool _documentMode;
    bool documentMode() const;
    void setDocumentMode(bool enabled);
	
    QTabBar *getTabBar();
    QWidget *getSeparatorWidget();

#if QT_CONFIG(tabwidget)
    QTabWidget::TabShape tabShape() const;
    void setTabShape(QTabWidget::TabShape tabShape);
    QTabWidget::TabPosition tabPosition(Qt::DockWidgetArea area) const;
    void setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition);

    QDockWidgetGroupWindow *createTabbedDockWindow();
#endif // QT_CONFIG(tabwidget)
#endif // QT_CONFIG(tabbar)

    // separators

    bool startSeparatorMove(const QPoint &pos);
    bool separatorMove(const QPoint &pos);
    bool endSeparatorMove(const QPoint &pos);
    void keepSize(QDockWidget *w);
#endif // QT_CONFIG(dockwidget)

    // save/restore

    enum VersionMarkers { // sentinel values used to validate state data
        VersionMarker = 0xff
    };
    void saveState(QDataStream &stream) const;
    bool restoreState(QDataStream &stream);

    // QLayout interface

    void addItem(QLayoutItem *item) override;
    void setGeometry(const QRect &r) override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;

    QSize sizeHint() const override;
    QSize minimumSize() const override;

    void invalidate() override;

    // animations


#if QT_CONFIG(dockwidget)
    QPointer<QWidget> currentHoveredFloat; // set when dragging over a floating dock widget
    void setCurrentHoveredFloat(QWidget *w);
#endif

    void hover(QLayoutItem *widgetItem, const QPoint &mousePos);
    bool plug(QLayoutItem *widgetItem);
    QLayoutItem *unplug(QWidget *widget, bool group = false);
    void revert(QLayoutItem *widgetItem);
    void paintDropIndicator(QPainter *p, QWidget *widget, const QRegion &clip);
    void applyState(QMainWindowLayoutState &newState, bool animate = true);
    void restore(bool keepSavedState = false);
    void updateHIToolBarStatus();
    void animationFinished(QWidget *widget);

private Q_SLOTS:
    void updateGapIndicator();
#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
    void tabChanged();
    void tabMoved(int from, int to);
#endif
#endif
private:
#if QT_CONFIG(tabbar)
    void updateTabBarShapes();
#endif

};



#endif // QDYNAMICMAINWINDOWLAYOUT_P_H

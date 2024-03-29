#include "qmainwindow.h"
#include "qmainwindowlayout_p.h"

#if QT_CONFIG(dockwidget)
#include "qdockwidget.h"
#endif
#include "qtoolbar.h"

#include <qapplication.h>
#include <qmenu.h>
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#if QT_CONFIG(statusbar)
#include <qstatusbar.h>
#endif
#include <qevent.h>
#include <qstyle.h>
#include <qdebug.h>
#include <qpainter.h>

#include <private/qwidget_p.h>
#include "qtoolbar_p.h"
#include "qwidgetanimator_p.h"

class QMainWindowLayout;

class QMainWindowPrivate : public QWidgetPrivate
{
    //Q_DECLARE_PUBLIC(QMainWindow)
public:
    QMainWindowPrivate()
        : layout(0), explicitIconSize(false), toolButtonStyle(Qt::ToolButtonIconOnly)
		, hasOldCursor(false) , cursorAdjusted(false)
    { }
    QMainWindowLayout *layout;
    QSize iconSize;
    bool explicitIconSize;
    Qt::ToolButtonStyle toolButtonStyle;

    void init();
    QList<int> hoverSeparator;
    QPoint hoverPos;

    QCursor separatorCursor(const QList<int> &path) const;
    void adjustCursor(const QPoint &pos);
    QCursor oldCursor;
    QCursor adjustedCursor;
    uint hasOldCursor : 1;
    uint cursorAdjusted : 1;

    static inline QMainWindowLayout *mainWindowLayout(const QMainWindow *mainWindow)
    {
        return mainWindow ? mainWindow->d_func()->layout : static_cast<QMainWindowLayout *>(0);
    }
};

QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *mainWindow)
{
    return QMainWindowPrivate::mainWindowLayout(mainWindow);
}

 void qt_setMainWindowTitleWidget(QMainWindow *mainWindow, 
 						Qt::DockWidgetArea area, QWidget *widget)
{
    QGridLayout *topLayout = qobject_cast<QGridLayout *>(mainWindow->layout());
    int row = 0;
    int column = 0;

    switch (area) {
    case Qt::LeftDockWidgetArea:
        row = 1;
        column = 0;
        break;
    case Qt::TopDockWidgetArea:
        row = 0;
        column = 1;
        break;
    case Qt::BottomDockWidgetArea:
        row = 2;
        column = 1;
        break;
    case Qt::RightDockWidgetArea:
        row = 1;
        column = 2;
        break;
    default:
        Q_ASSERT_X(false, "qt_setMainWindowTitleWidget", "Unknown area");
        return;
    }

    if (QLayoutItem *oldItem = topLayout->itemAtPosition(row, column))
        delete oldItem->widget();
	//
    topLayout->addWidget(widget, row, column);
}

void QMainWindowPrivate::init()
{
    QMainWindow * const q = q_func();

#ifdef QT_EXPERIMENTAL_CLIENT_DECORATIONS
    QGridLayout *topLayout = new QGridLayout(q);
    topLayout->setContentsMargins(0, 0, 0, 0);

	// 把一个GridLayout对象直接塞入QMainWindowLayout
    layout = new QMainWindowLayout(q, topLayout);

    topLayout->addItem(layout, 1, 1);
#else

    layout = new QMainWindowLayout(q, 0);
#endif

    const int metric = q->style()->pixelMetric(QStyle::PM_ToolBarIconSize, 0, q);
    iconSize = QSize(metric, metric);
    q->setAttribute(Qt::WA_Hover);
}

QMainWindow::QMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*(new QMainWindowPrivate()), parent, flags | Qt::Window)// 强行给Qt::Window
{
    d_func()->init();
}


/*!
    Destroys the main window.
 */
QMainWindow::~QMainWindow()
{ }

void QMainWindow::setDockOptions(DockOptions opt)
{
    QMainWindowPrivate * const d = d_func();
    d->layout->setDockOptions(opt);
}

QMainWindow::DockOptions QMainWindow::dockOptions() const
{
    QMainWindowPrivate * const d = d_func();
    return d->layout->dockOptions;
}

QSize QMainWindow::iconSize() const
{ 
	QMainWindowPrivate * const d = d_func();
	return d->iconSize;
}
void QMainWindow::setIconSize(const QSize &iconSize)
{
    QMainWindowPrivate * const d = d_func();
    QSize sz = iconSize;
    if (!sz.isValid()) {
        const int metric = style()->pixelMetric(QStyle::PM_ToolBarIconSize, 0, this);
        sz = QSize(metric, metric);
    }
    if (d->iconSize != sz) {
        d->iconSize = sz;
        emit iconSizeChanged(d->iconSize);
    }
    d->explicitIconSize = iconSize.isValid();
}


Qt::ToolButtonStyle QMainWindow::toolButtonStyle() const
{ return d_func()->toolButtonStyle; }

void QMainWindow::setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
    QMainWindowPrivate * const d = d_func();
    if (d->toolButtonStyle == toolButtonStyle)
        return;
    d->toolButtonStyle = toolButtonStyle;
    emit toolButtonStyleChanged(d->toolButtonStyle);
}

#if QT_CONFIG(menubar)
/*!
    Returns the menu bar for the main window. This function creates
    and returns an empty menu bar if the menu bar does not exist.

    If you want all windows in a Mac application to share one menu
    bar, don't use this function to create it, because the menu bar
    created here will have this QMainWindow as its parent.  Instead,
    you must create a menu bar that does not have a parent, which you
    can then share among all the Mac windows. Create a parent-less
    menu bar this way:

    \snippet code/src_gui_widgets_qmenubar.cpp 1

    \sa setMenuBar()
*/
QMenuBar *QMainWindow::menuBar() const
{
    QMenuBar *menuBar = qobject_cast<QMenuBar *>(layout()->menuBar());
    if (!menuBar) {
        QMainWindow *self = const_cast<QMainWindow *>(this);
        menuBar = new QMenuBar(self);
        self->setMenuBar(menuBar);
    }
    return menuBar;
}

/*!
    Sets the menu bar for the main window to \a menuBar.

    Note: QMainWindow takes ownership of the \a menuBar pointer and
    deletes it at the appropriate time.

    \sa menuBar()
*/
void QMainWindow::setMenuBar(QMenuBar *menuBar)
{
    QLayout *topLayout = layout();

    if (topLayout->menuBar() && topLayout->menuBar() != menuBar) {
        // Reparent corner widgets before we delete the old menu bar.
        QMenuBar *oldMenuBar = qobject_cast<QMenuBar *>(topLayout->menuBar());
        if (menuBar) {
            // TopLeftCorner widget.
            QWidget *cornerWidget = oldMenuBar->cornerWidget(Qt::TopLeftCorner);
            if (cornerWidget)
                menuBar->setCornerWidget(cornerWidget, Qt::TopLeftCorner);
            // TopRightCorner widget.
            cornerWidget = oldMenuBar->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget)
                menuBar->setCornerWidget(cornerWidget, Qt::TopRightCorner);
        }
        oldMenuBar->hide();
        oldMenuBar->setParent(nullptr);
        oldMenuBar->deleteLater();
    }
    topLayout->setMenuBar(menuBar);
}

/*!
    \since 4.2

    Returns the menu bar for the main window. This function returns
    null if a menu bar hasn't been constructed yet.
*/
QWidget *QMainWindow::menuWidget() const
{
    QWidget *menuBar = d_func()->layout->menuBar();
    return menuBar;
}

/*!
    \since 4.2

    Sets the menu bar for the main window to \a menuBar.

    QMainWindow takes ownership of the \a menuBar pointer and
    deletes it at the appropriate time.
*/
void QMainWindow::setMenuWidget(QWidget *menuBar)
{
    QMainWindowPrivate * const d = d_func();
    if (d->layout->menuBar() && d->layout->menuBar() != menuBar) {
        d->layout->menuBar()->hide();
        d->layout->menuBar()->deleteLater();
    }
    d->layout->setMenuBar(menuBar);
}
#endif // QT_CONFIG(menubar)

#if QT_CONFIG(statusbar)
/*!
    Returns the status bar for the main window. This function creates
    and returns an empty status bar if the status bar does not exist.

    \sa setStatusBar()
*/
QStatusBar *QMainWindow::statusBar() const
{
    QStatusBar *statusbar = d_func()->layout->statusBar();
    if (!statusbar) {
        QMainWindow *self = const_cast<QMainWindow *>(this);
        statusbar = new QStatusBar(self);
        statusbar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        self->setStatusBar(statusbar);
    }
    return statusbar;
}

/*!
    Sets the status bar for the main window to \a statusbar.

    Setting the status bar to 0 will remove it from the main window.
    Note that QMainWindow takes ownership of the \a statusbar pointer
    and deletes it at the appropriate time.

    \sa statusBar()
*/
void QMainWindow::setStatusBar(QStatusBar *statusbar)
{
    QMainWindowPrivate * const d = d_func();
    if (d->layout->statusBar() && d->layout->statusBar() != statusbar) {
        d->layout->statusBar()->hide();
        d->layout->statusBar()->deleteLater();
    }
    d->layout->setStatusBar(statusbar);
}
#endif // QT_CONFIG(statusbar)


QWidget *QMainWindow::centralWidget() const
{ return d_func()->layout->centralWidget(); }


void QMainWindow::setCentralWidget(QWidget *widget)
{
    QMainWindowPrivate * const d = d_func();
    if (d->layout->centralWidget() && d->layout->centralWidget() != widget) {
        d->layout->centralWidget()->hide();
        d->layout->centralWidget()->deleteLater();
    }
    d->layout->setCentralWidget(widget);
}


QWidget *QMainWindow::takeCentralWidget()
{
    QMainWindowPrivate * const d = d_func();
    QWidget *oldcentralwidget = d->layout->centralWidget();
    if (oldcentralwidget) {
        oldcentralwidget->setParent(0);
        d->layout->setCentralWidget(0);
    }
    return oldcentralwidget;
}

#if QT_CONFIG(dockwidget)
/*!
    Sets the given dock widget \a area to occupy the specified \a
    corner.

    \sa corner()
*/
void QMainWindow::setCorner(Qt::Corner corner, Qt::DockWidgetArea area)
{
    bool valid = false;
    switch (corner) {
    case Qt::TopLeftCorner:
        valid = (area == Qt::TopDockWidgetArea || area == Qt::LeftDockWidgetArea);
        break;
    case Qt::TopRightCorner:
        valid = (area == Qt::TopDockWidgetArea || area == Qt::RightDockWidgetArea);
        break;
    case Qt::BottomLeftCorner:
        valid = (area == Qt::BottomDockWidgetArea || area == Qt::LeftDockWidgetArea);
        break;
    case Qt::BottomRightCorner:
        valid = (area == Qt::BottomDockWidgetArea || area == Qt::RightDockWidgetArea);
        break;
    }
    if (Q_UNLIKELY(!valid))
        qWarning("QMainWindow::setCorner(): 'area' is not valid for 'corner'");
    else
        d_func()->layout->setCorner(corner, area);
}

/*!
    Returns the dock widget area that occupies the specified \a
    corner.

    \sa setCorner()
*/
Qt::DockWidgetArea QMainWindow::corner(Qt::Corner corner) const
{ return d_func()->layout->corner(corner); }
#endif

#ifndef QT_NO_TOOLBAR

static bool checkToolBarArea(Qt::ToolBarArea area, const char *where)
{
    switch (area) {
    case Qt::LeftToolBarArea:
    case Qt::RightToolBarArea:
    case Qt::TopToolBarArea:
    case Qt::BottomToolBarArea:
        return true;
    default:
        break;
    }
    qWarning("%s: invalid 'area' argument", where);
    return false;
}

/*!
    Adds a toolbar break to the given \a area after all the other
    objects that are present.
*/
void QMainWindow::addToolBarBreak(Qt::ToolBarArea area)
{
    if (!checkToolBarArea(area, "QMainWindow::addToolBarBreak"))
        return;
    d_func()->layout->addToolBarBreak(area);
}

/*!
    Inserts a toolbar break before the toolbar specified by \a before.
*/
void QMainWindow::insertToolBarBreak(QToolBar *before)
{ d_func()->layout->insertToolBarBreak(before); }

/*!
    Removes a toolbar break previously inserted before the toolbar specified by \a before.
*/

void QMainWindow::removeToolBarBreak(QToolBar *before)
{
    QMainWindowPrivate * const d = d_func();
    d->layout->removeToolBarBreak(before);
}

/*!
    Adds the \a toolbar into the specified \a area in this main
    window. The \a toolbar is placed at the end of the current tool
    bar block (i.e. line). If the main window already manages \a toolbar
    then it will only move the toolbar to \a area.

    \sa insertToolBar(), addToolBarBreak(), insertToolBarBreak()
*/
void QMainWindow::addToolBar(Qt::ToolBarArea area, QToolBar *toolbar)
{
    if (!checkToolBarArea(area, "QMainWindow::addToolBar"))
        return;

    QMainWindowPrivate * const d = d_func();

    disconnect(this, SIGNAL(iconSizeChanged(QSize)),
               toolbar, SLOT(_q_updateIconSize(QSize)));
    disconnect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
               toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    if(toolbar->d_func()->state && toolbar->d_func()->state->dragging) {
        //removing a toolbar which is dragging will cause crash
#if QT_CONFIG(dockwidget)
        bool animated = isAnimated();
        setAnimated(false);
#endif
        toolbar->d_func()->endDrag();
#if QT_CONFIG(dockwidget)
        setAnimated(animated);
#endif
    }

    if (!d->layout->usesHIToolBar(toolbar)) {
        d->layout->removeWidget(toolbar);
    } else {
        d->layout->removeToolBar(toolbar);
    }

    toolbar->d_func()->_q_updateIconSize(d->iconSize);
    toolbar->d_func()->_q_updateToolButtonStyle(d->toolButtonStyle);
    connect(this, SIGNAL(iconSizeChanged(QSize)),
            toolbar, SLOT(_q_updateIconSize(QSize)));
    connect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    d->layout->addToolBar(area, toolbar);
}

/*! \overload
    Equivalent of calling addToolBar(Qt::TopToolBarArea, \a toolbar)
*/
void QMainWindow::addToolBar(QToolBar *toolbar)
{ addToolBar(Qt::TopToolBarArea, toolbar); }

/*!
    \overload

    Creates a QToolBar object, setting its window title to \a title,
    and inserts it into the top toolbar area.

    \sa setWindowTitle()
*/
QToolBar *QMainWindow::addToolBar(const QString &title)
{
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setWindowTitle(title);
    addToolBar(toolBar);
    return toolBar;
}

/*!
    Inserts the \a toolbar into the area occupied by the \a before toolbar
    so that it appears before it. For example, in normal left-to-right
    layout operation, this means that \a toolbar will appear to the left
    of the toolbar specified by \a before in a horizontal toolbar area.

    \sa insertToolBarBreak(), addToolBar(), addToolBarBreak()
*/
void QMainWindow::insertToolBar(QToolBar *before, QToolBar *toolbar)
{
    QMainWindowPrivate * const d = d_func();

    d->layout->removeToolBar(toolbar);

    toolbar->d_func()->_q_updateIconSize(d->iconSize);
    toolbar->d_func()->_q_updateToolButtonStyle(d->toolButtonStyle);
    connect(this, SIGNAL(iconSizeChanged(QSize)),
            toolbar, SLOT(_q_updateIconSize(QSize)));
    connect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    d->layout->insertToolBar(before, toolbar);
}

/*!
    Removes the \a toolbar from the main window layout and hides
    it. Note that the \a toolbar is \e not deleted.
*/
void QMainWindow::removeToolBar(QToolBar *toolbar)
{
    if (toolbar) {
        d_func()->layout->removeToolBar(toolbar);
        toolbar->hide();
    }
}

/*!
    Returns the Qt::ToolBarArea for \a toolbar. If \a toolbar has not
    been added to the main window, this function returns \c
    Qt::NoToolBarArea.

    \sa addToolBar(), addToolBarBreak(), Qt::ToolBarArea
*/
Qt::ToolBarArea QMainWindow::toolBarArea(QToolBar *toolbar) const
{ return d_func()->layout->toolBarArea(toolbar); }

/*!

    Returns whether there is a toolbar
    break before the \a toolbar.

    \sa addToolBarBreak(), insertToolBarBreak()
*/
bool QMainWindow::toolBarBreak(QToolBar *toolbar) const
{
    return d_func()->layout->toolBarBreak(toolbar);
}

#endif // QT_NO_TOOLBAR

#if QT_CONFIG(dockwidget)

/*! \property QMainWindow::animated
    \brief whether manipulating dock widgets and tool bars is animated
    \since 4.2

    When a dock widget or tool bar is dragged over the
    main window, the main window adjusts its contents
    to indicate where the dock widget or tool bar will
    be docked if it is dropped. Setting this property
    causes QMainWindow to move its contents in a smooth
    animation. Clearing this property causes the contents
    to snap into their new positions.

    By default, this property is set. It may be cleared if
    the main window contains widgets which are slow at resizing
    or repainting themselves.

    Setting this property is identical to setting the AnimatedDocks
    option using setDockOptions().
*/

bool QMainWindow::isAnimated() const
{
    QMainWindowPrivate * const d = d_func();
    return d->layout->dockOptions & AnimatedDocks;
}

void QMainWindow::setAnimated(bool enabled)
{
    QMainWindowPrivate * const d = d_func();

    DockOptions opts = d->layout->dockOptions;
    opts.setFlag(AnimatedDocks, enabled);

    d->layout->setDockOptions(opts);
}

/*! \property QMainWindow::dockNestingEnabled
    \brief whether docks can be nested
    \since 4.2

    If this property is \c false, dock areas can only contain a single row
    (horizontal or vertical) of dock widgets. If this property is \c true,
    the area occupied by a dock widget can be split in either direction to contain
    more dock widgets.

    Dock nesting is only necessary in applications that contain a lot of
    dock widgets. It gives the user greater freedom in organizing their
    main window. However, dock nesting leads to more complex
    (and less intuitive) behavior when a dock widget is dragged over the
    main window, since there are more ways in which a dropped dock widget
    may be placed in the dock area.

    Setting this property is identical to setting the AllowNestedDocks option
    using setDockOptions().
*/

bool QMainWindow::isDockNestingEnabled() const
{
    QMainWindowPrivate * const d = d_func();
    return d->layout->dockOptions & AllowNestedDocks;
}

void QMainWindow::setDockNestingEnabled(bool enabled)
{
    QMainWindowPrivate * const d = d_func();

    DockOptions opts = d->layout->dockOptions;
    opts.setFlag(AllowNestedDocks, enabled);

    d->layout->setDockOptions(opts);
}

#if 0
/*! \property QMainWindow::verticalTabsEnabled
    \brief whether left and right dock areas use vertical tabs
    \since 4.2

    If this property is set to false, dock areas containing tabbed dock widgets
    display horizontal tabs, simmilar to Visual Studio.

    If this property is set to true, then the right and left dock areas display vertical
    tabs, simmilar to KDevelop.

    This property should be set before any dock widgets are added to the main window.
*/

bool QMainWindow::verticalTabsEnabled() const
{
    return d_func()->layout->verticalTabsEnabled();
}

void QMainWindow::setVerticalTabsEnabled(bool enabled)
{
    d_func()->layout->setVerticalTabsEnabled(enabled);
}
#endif

static bool checkDockWidgetArea(Qt::DockWidgetArea area, const char *where)
{
    switch (area) {
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        return true;
    default:
        break;
    }
    qWarning("%s: invalid 'area' argument", where);
    return false;
}

#if QT_CONFIG(tabbar)
/*!
    \property QMainWindow::documentMode
    \brief whether the tab bar for tabbed dockwidgets is set to document mode.
    \since 4.5

    The default is false.

    \sa QTabBar::documentMode
*/
bool QMainWindow::documentMode() const
{
    return d_func()->layout->documentMode();
}

void QMainWindow::setDocumentMode(bool enabled)
{
    d_func()->layout->setDocumentMode(enabled);
}
#endif // QT_CONFIG(tabbar)

#if QT_CONFIG(tabwidget)
/*!
    \property QMainWindow::tabShape
    \brief the tab shape used for tabbed dock widgets.
    \since 4.5

    The default is \l QTabWidget::Rounded.

    \sa setTabPosition()
*/
QTabWidget::TabShape QMainWindow::tabShape() const
{
    return d_func()->layout->tabShape();
}

void QMainWindow::setTabShape(QTabWidget::TabShape tabShape)
{
    d_func()->layout->setTabShape(tabShape);
}

/*!
    \since 4.5

    Returns the tab position for \a area.

    \note The \l VerticalTabs dock option overrides the tab positions returned
    by this function.

    \sa setTabPosition(), tabShape()
*/
QTabWidget::TabPosition QMainWindow::tabPosition(Qt::DockWidgetArea area) const
{
    if (!checkDockWidgetArea(area, "QMainWindow::tabPosition"))
        return QTabWidget::South;
    return d_func()->layout->tabPosition(area);
}

/*!
    \since 4.5

    Sets the tab position for the given dock widget \a areas to the specified
    \a tabPosition. By default, all dock areas show their tabs at the bottom.

    \note The \l VerticalTabs dock option overrides the tab positions set by
    this method.

    \sa tabPosition(), setTabShape()
*/
void QMainWindow::setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition)
{
    d_func()->layout->setTabPosition(areas, tabPosition);
}
#endif // QT_CONFIG(tabwidget)

/*!
    Adds the given \a dockwidget to the specified \a area.
*/
void QMainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget)
{
    if (!checkDockWidgetArea(area, "QMainWindow::addDockWidget"))
        return;

    Qt::Orientation orientation = Qt::Vertical;
    switch (area) {
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        orientation = Qt::Horizontal;
        break;
    default:
        break;
    }
    d_func()->layout->removeWidget(dockwidget); // in case it was already in here
    addDockWidget(area, dockwidget, orientation);

#if 0 // Used to be included in Qt4 for Q_WS_MAC     //drawer support
    QMacAutoReleasePool pool;
    extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
    if (qt_mac_is_macdrawer(dockwidget)) {
        extern bool qt_mac_set_drawer_preferred_edge(QWidget *, Qt::DockWidgetArea); //qwidget_mac.cpp
        window()->createWinId();
        dockwidget->window()->createWinId();
        qt_mac_set_drawer_preferred_edge(dockwidget, area);
        if (dockwidget->isVisible()) {
            dockwidget->hide();
            dockwidget->show();
        }
    }
#endif
}

/*!
    Restores the state of \a dockwidget if it is created after the call
    to restoreState(). Returns \c true if the state was restored; otherwise
    returns \c false.

    \sa restoreState(), saveState()
*/

bool QMainWindow::restoreDockWidget(QDockWidget *dockwidget)
{
    return d_func()->layout->restoreDockWidget(dockwidget);
}

/*!
    Adds \a dockwidget into the given \a area in the direction
    specified by the \a orientation.
*/
void QMainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget,
                                Qt::Orientation orientation)
{
    if (!checkDockWidgetArea(area, "QMainWindow::addDockWidget"))
        return;

    // add a window to an area, placing done relative to the previous
    d_func()->layout->addDockWidget(area, dockwidget, orientation);
}

/*!
    \fn void QMainWindow::splitDockWidget(QDockWidget *first, QDockWidget *second, Qt::Orientation orientation)

    Splits the space covered by the \a first dock widget into two parts,
    moves the \a first dock widget into the first part, and moves the
    \a second dock widget into the second part.

    The \a orientation specifies how the space is divided: A Qt::Horizontal
    split places the second dock widget to the right of the first; a
    Qt::Vertical split places the second dock widget below the first.

    \e Note: if \a first is currently in a tabbed docked area, \a second will
    be added as a new tab, not as a neighbor of \a first. This is because a
    single tab can contain only one dock widget.

    \e Note: The Qt::LayoutDirection influences the order of the dock widgets
    in the two parts of the divided area. When right-to-left layout direction
    is enabled, the placing of the dock widgets will be reversed.

    \sa tabifyDockWidget(), addDockWidget(), removeDockWidget()
*/
void QMainWindow::splitDockWidget(QDockWidget *after, QDockWidget *dockwidget,
                                  Qt::Orientation orientation)
{
    d_func()->layout->splitDockWidget(after, dockwidget, orientation);
}

/*!
    \fn void QMainWindow::tabifyDockWidget(QDockWidget *first, QDockWidget *second)

    Moves \a second dock widget on top of \a first dock widget, creating a tabbed
    docked area in the main window.

    \sa tabifiedDockWidgets()
*/
void QMainWindow::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    d_func()->layout->tabifyDockWidget(first, second);
}


/*!
    \fn QList<QDockWidget*> QMainWindow::tabifiedDockWidgets(QDockWidget *dockwidget) const

    Returns the dock widgets that are tabified together with \a dockwidget.

    \since 4.5
    \sa tabifyDockWidget()
*/

QList<QDockWidget*> QMainWindow::tabifiedDockWidgets(QDockWidget *dockwidget) const
{
    QList<QDockWidget*> ret;
#if !QT_CONFIG(tabbar)
    Q_UNUSED(dockwidget);
#else
    const QDockAreaLayoutInfo *info = d_func()->layout->layoutState.dockAreaLayout.info(dockwidget);
    if (info && info->tabbed && info->tabBar) {
        for(int i = 0; i < info->item_list.count(); ++i) {
            const QDockAreaLayoutItem &item = info->item_list.at(i);
            if (item.widgetItem) {
                if (QDockWidget *dock = qobject_cast<QDockWidget*>(item.widgetItem->widget())) {
                    if (dock != dockwidget) {
                        ret += dock;
                    }
                }
            }
        }
    }
#endif
    return ret;
}


/*!
    Removes the \a dockwidget from the main window layout and hides
    it. Note that the \a dockwidget is \e not deleted.
*/
void QMainWindow::removeDockWidget(QDockWidget *dockwidget)
{
    if (dockwidget) {
        d_func()->layout->removeWidget(dockwidget);
        dockwidget->hide();
    }
}

/*!
    Returns the Qt::DockWidgetArea for \a dockwidget. If \a dockwidget
    has not been added to the main window, this function returns \c
    Qt::NoDockWidgetArea.

    \sa addDockWidget(), splitDockWidget(), Qt::DockWidgetArea
*/
Qt::DockWidgetArea QMainWindow::dockWidgetArea(QDockWidget *dockwidget) const
{ return d_func()->layout->dockWidgetArea(dockwidget); }


/*!
    \since 5.6
    Resizes the dock widgets in the list \a docks to the corresponding size in
    pixels from the list \a sizes. If \a orientation is Qt::Horizontal, adjusts
    the width, otherwise adjusts the height of the dock widgets.
    The sizes will be adjusted such that the maximum and the minimum sizes are
    respected and the QMainWindow itself will not be resized.
    Any additional/missing space is distributed amongst the widgets according
    to the relative weight of the sizes.

    Example:
    \code
    resizeDocks({blueWidget, yellowWidget}, {20 , 40}, Qt::Horizontal);
    \endcode
    If the blue and the yellow widget are nested on the same level they will be
    resized such that the yellowWidget is twice as big as the blueWidget

    If some widgets are grouped in tabs, only one widget per group should be
    specified. Widgets not in the list might be changed to repect the constraints.
*/
void QMainWindow::resizeDocks(const QList<QDockWidget *> &docks,
                              const QList<int> &sizes, Qt::Orientation orientation)
{
    d_func()->layout->layoutState.dockAreaLayout.resizeDocks(docks, sizes, orientation);
    d_func()->layout->invalidate();
}


#endif // QT_CONFIG(dockwidget)

/*!
    Saves the current state of this mainwindow's toolbars and
    dockwidgets. This includes the corner settings which can
    be set with setCorner(). The \a version number is stored
    as part of the data.

    The \l{QObject::objectName}{objectName} property is used
    to identify each QToolBar and QDockWidget.  You should make sure
    that this property is unique for each QToolBar and QDockWidget you
    add to the QMainWindow

    To restore the saved state, pass the return value and \a version
    number to restoreState().

    To save the geometry when the window closes, you can
    implement a close event like this:

    \snippet code/src_gui_widgets_qmainwindow.cpp 0

    \sa restoreState(), QWidget::saveGeometry(), QWidget::restoreGeometry()
*/
QByteArray QMainWindow::saveState(int version) const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << QMainWindowLayout::VersionMarker;
    stream << version;
    d_func()->layout->saveState(stream);
    return data;
}

/*!
    Restores the \a state of this mainwindow's toolbars and
    dockwidgets. Also restores the corner settings too. The
    \a version number is compared with that stored in \a state.
    If they do not match, the mainwindow's state is left
    unchanged, and this function returns \c false; otherwise, the state
    is restored, and this function returns \c true.

    To restore geometry saved using QSettings, you can use code like
    this:

    \snippet code/src_gui_widgets_qmainwindow.cpp 1

    \sa saveState(), QWidget::saveGeometry(),
    QWidget::restoreGeometry(), restoreDockWidget()
*/
bool QMainWindow::restoreState(const QByteArray &state, int version)
{
    if (state.isEmpty())
        return false;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    int marker, v;
    stream >> marker;
    stream >> v;
    if (stream.status() != QDataStream::Ok || marker != QMainWindowLayout::VersionMarker || v != version)
        return false;
    bool restored = d_func()->layout->restoreState(stream);
    return restored;
}

#if QT_CONFIG(dockwidget) && !defined(QT_NO_CURSOR)
QCursor QMainWindowPrivate::separatorCursor(const QList<int> &path) const
{
    QDockAreaLayoutInfo *info = layout->layoutState.dockAreaLayout.info(path);
    Q_ASSERT(info != 0);
    if (path.size() == 1) { // is this the "top-level" separator which separates a dock area
                            // from the central widget?
        switch (path.first()) {
            case QInternal::LeftDock:
            case QInternal::RightDock:
                return Qt::SplitHCursor;
            case QInternal::TopDock:
            case QInternal::BottomDock:
                return Qt::SplitVCursor;
            default:
                break;
        }
    }

    // no, it's a splitter inside a dock area, separating two dock widgets

    return info->o == Qt::Horizontal
            ? Qt::SplitHCursor : Qt::SplitVCursor;
}

void QMainWindowPrivate::adjustCursor(const QPoint &pos)
{
    QMainWindow * const q = q_func();

    hoverPos = pos;

    if (pos == QPoint(0, 0)) {
        if (!hoverSeparator.isEmpty())
            q->update(layout->layoutState.dockAreaLayout.separatorRect(hoverSeparator));
        hoverSeparator.clear();

        if (cursorAdjusted) {
            cursorAdjusted = false;
            if (hasOldCursor)
                q->setCursor(oldCursor);
            else
                q->unsetCursor();
        }
    } else if (layout->movingSeparator.isEmpty()) { // Don't change cursor when moving separator
        QList<int> pathToSeparator
            = layout->layoutState.dockAreaLayout.findSeparator(pos);

        if (pathToSeparator != hoverSeparator) {
            if (!hoverSeparator.isEmpty())
                q->update(layout->layoutState.dockAreaLayout.separatorRect(hoverSeparator));

            hoverSeparator = pathToSeparator;

            if (hoverSeparator.isEmpty()) {
                if (cursorAdjusted) {
                    cursorAdjusted = false;
                    if (hasOldCursor)
                        q->setCursor(oldCursor);
                    else
                        q->unsetCursor();
                }
            } else {
                q->update(layout->layoutState.dockAreaLayout.separatorRect(hoverSeparator));
                if (!cursorAdjusted) {
                    oldCursor = q->cursor();
                    hasOldCursor = q->testAttribute(Qt::WA_SetCursor);
                }
                adjustedCursor = separatorCursor(hoverSeparator);
                q->setCursor(adjustedCursor);
                cursorAdjusted = true;
            }
        }
    }
}
#endif

/*! \reimp */
bool QMainWindow::event(QEvent *event)
{
    QMainWindowPrivate * const d = d_func();
    switch (event->type()) {

#if QT_CONFIG(dockwidget)
        case QEvent::Paint: {
            QPainter p(this);
            QRegion r = static_cast<QPaintEvent*>(event)->region();
            d->layout->layoutState.dockAreaLayout.paintSeparators(&p, this, r, d->hoverPos);
            break;
        }

#ifndef QT_NO_CURSOR
        case QEvent::HoverMove:  {
            d->adjustCursor(static_cast<QHoverEvent*>(event)->pos());
            break;
        }

        // We don't want QWidget to call update() on the entire QMainWindow
        // on HoverEnter and HoverLeave, hence accept the event (return true).
        case QEvent::HoverEnter:
            return true;
        case QEvent::HoverLeave:
            d->adjustCursor(QPoint(0, 0));
            return true;
        case QEvent::ShortcutOverride: // when a menu pops up
            d->adjustCursor(QPoint(0, 0));
            break;
#endif // QT_NO_CURSOR

        case QEvent::MouseButtonPress: {
            QMouseEvent *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton && d->layout->startSeparatorMove(e->pos())) {
                // The click was on a separator, eat this event
                e->accept();
                return true;
            }
            break;
        }

        case QEvent::MouseMove: {
            QMouseEvent *e = static_cast<QMouseEvent*>(event);

#ifndef QT_NO_CURSOR
            d->adjustCursor(e->pos());
#endif
            if (e->buttons() & Qt::LeftButton) {
                if (d->layout->separatorMove(e->pos())) {
                    // We're moving a separator, eat this event
                    e->accept();
                    return true;
                }
            }

            break;
        }

        case QEvent::MouseButtonRelease: {
            QMouseEvent *e = static_cast<QMouseEvent*>(event);
            if (d->layout->endSeparatorMove(e->pos())) {
                // We've released a separator, eat this event
                e->accept();
                return true;
            }
            break;
        }

#endif

#ifndef QT_NO_TOOLBAR
        case QEvent::ToolBarChange: {
            d->layout->toggleToolBarsVisible();
            return true;
        }
#endif

#if QT_CONFIG(statustip)
        case QEvent::StatusTip:
#if QT_CONFIG(statusbar)
            if (QStatusBar *sb = d->layout->statusBar())
                sb->showMessage(static_cast<QStatusTipEvent*>(event)->tip());
            else
#endif
                static_cast<QStatusTipEvent*>(event)->ignore();
            return true;
#endif // QT_CONFIG(statustip)

        case QEvent::StyleChange:
#if QT_CONFIG(dockwidget)
            d->layout->layoutState.dockAreaLayout.styleChangedEvent();
#endif
            if (!d->explicitIconSize)
                setIconSize(QSize());
            break;
#if QT_CONFIG(dockwidget) && !defined(QT_NO_CURSOR)
       case QEvent::CursorChange:
           // CursorChange events are triggered as mouse moves to new widgets even
           // if the cursor doesn't actually change, so do not change oldCursor if
           // the "changed" cursor has same shape as adjusted cursor.
           if (d->cursorAdjusted && d->adjustedCursor.shape() != cursor().shape()) {
               d->oldCursor = cursor();
               d->hasOldCursor = testAttribute(Qt::WA_SetCursor);

               // Ensure our adjusted cursor stays visible
               setCursor(d->adjustedCursor);
           }
           break;
#endif
        default:
            break;
    }

    return QWidget::event(event);
}

/*!
    \internal
*/
bool QMainWindow::isSeparator(const QPoint &pos) const
{
#if QT_CONFIG(dockwidget)
    QMainWindowPrivate * const d = d_func();
    return !d->layout->layoutState.dockAreaLayout.findSeparator(pos).isEmpty();
#else
    Q_UNUSED(pos);
    return false;
#endif
}

#ifndef QT_NO_CONTEXTMENU
/*!
    \reimp
*/
void QMainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
    // only show the context menu for direct QDockWidget and QToolBar
    // children and for the menu bar as well
    QWidget *child = childAt(event->pos());
    while (child && child != this) {
#if QT_CONFIG(menubar)
        if (QMenuBar *mb = qobject_cast<QMenuBar *>(child)) {
            if (mb->parentWidget() != this)
                return;
            break;
        }
#endif
#if QT_CONFIG(dockwidget)
        if (QDockWidget *dw = qobject_cast<QDockWidget *>(child)) {
            if (dw->parentWidget() != this)
                return;
            if (dw->widget()
                && dw->widget()->geometry().contains(child->mapFrom(this, event->pos()))) {
                // ignore the event if the mouse is over the QDockWidget contents
                return;
            }
            break;
        }
#endif // QT_CONFIG(dockwidget)
#ifndef QT_NO_TOOLBAR
        if (QToolBar *tb = qobject_cast<QToolBar *>(child)) {
            if (tb->parentWidget() != this)
                return;
            break;
        }
#endif
        child = child->parentWidget();
    }
    if (child == this)
        return;

#if QT_CONFIG(menu)
    QMenu *popup = createPopupMenu();
    if (popup) {
        if (!popup->isEmpty()) {
            popup->setAttribute(Qt::WA_DeleteOnClose);
            popup->popup(event->globalPos());
            event->accept();
        } else {
            delete popup;
        }
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

#if QT_CONFIG(menu)
/*!
    Returns a popup menu containing checkable entries for the toolbars and
    dock widgets present in the main window. If  there are no toolbars and
    dock widgets present, this function returns a null pointer.

    By default, this function is called by the main window when the user
    activates a context menu, typically by right-clicking on a toolbar or a dock
    widget.

    If you want to create a custom popup menu, reimplement this function and
    return a newly-created popup menu. Ownership of the popup menu is transferred
    to the caller.

    \sa addDockWidget(), addToolBar(), menuBar()
*/
QMenu *QMainWindow::createPopupMenu()
{
    QMainWindowPrivate * const d = d_func();
    QMenu *menu = 0;
#if QT_CONFIG(dockwidget)
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    if (dockwidgets.size()) {
        menu = new QMenu(this);
        for (int i = 0; i < dockwidgets.size(); ++i) {
            QDockWidget *dockWidget = dockwidgets.at(i);
            // filter to find out if we own this QDockWidget
            if (dockWidget->parentWidget() == this) {
                if (d->layout->layoutState.dockAreaLayout.indexOf(dockWidget).isEmpty())
                    continue;
            } else if (QDockWidgetGroupWindow *dwgw =
                           qobject_cast<QDockWidgetGroupWindow *>(dockWidget->parentWidget())) {
                if (dwgw->parentWidget() != this)
                    continue;
                if (dwgw->layoutInfo()->indexOf(dockWidget).isEmpty())
                    continue;
            } else {
                continue;
            }
            menu->addAction(dockwidgets.at(i)->toggleViewAction());
        }
        menu->addSeparator();
    }
#endif // QT_CONFIG(dockwidget)
#ifndef QT_NO_TOOLBAR
    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    if (toolbars.size()) {
        if (!menu)
            menu = new QMenu(this);
        for (int i = 0; i < toolbars.size(); ++i) {
            QToolBar *toolBar = toolbars.at(i);
            if (toolBar->parentWidget() == this
                && (!d->layout->layoutState.toolBarAreaLayout.indexOf(toolBar).isEmpty())) {
                menu->addAction(toolbars.at(i)->toggleViewAction());
            }
        }
    }
#endif
    Q_UNUSED(d);
    return menu;
}
#endif // QT_CONFIG(menu)

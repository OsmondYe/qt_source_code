#ifndef QDYNAMICMAINWINDOW_H
#define QDYNAMICMAINWINDOW_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>
#if QT_CONFIG(tabwidget)
#include <QtWidgets/qtabwidget.h>
#endif


class QDockWidget;
class QMainWindowPrivate;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QMenu;


/*
layout:
	----------------------------
	menu_bar
	----------------------------
	tool_bar
	----------------------------
	dock_bar
		|------------------|
		|				   |
		|				   |
		|   centralWidget  |
		|				   |
		|				   |
		|------------------|
	----------------------------
	status_bar
	----------------------------

我们需要给提供一个centralWidget,  至于布局之类的, MainWindow自己内部定义了

*/
class  QMainWindow : public QWidget
{
    //Q_OBJECT

//    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
//    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle)
//    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)
//    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
//    Q_PROPERTY(QTabWidget::TabShape tabShape READ tabShape WRITE setTabShape)
//    Q_PROPERTY(bool dockNestingEnabled READ isDockNestingEnabled WRITE setDockNestingEnabled)
//    Q_PROPERTY(DockOptions dockOptions READ dockOptions WRITE setDockOptions)
//	Q_PROPERTY(bool unifiedTitleAndToolBarOnMac READ unifiedTitleAndToolBarOnMac WRITE setUnifiedTitleAndToolBarOnMac)
public:
	QWidget *centralWidget() const;
	void setCentralWidget(QWidget *widget);
	QWidget *takeCentralWidget();

protected:
	bool event(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
public Q_SLOTS:
#if QT_CONFIG(dockwidget)
    void setAnimated(bool enabled);
    void setDockNestingEnabled(bool enabled);
#endif

Q_SIGNALS:

    void iconSizeChanged(const QSize &iconSize);
    void toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle);
#if QT_CONFIG(dockwidget)
    void tabifiedDockWidgetActivated(QDockWidget *dockWidget);
#endif


public:
    explicit QMainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
    ~QMainWindow();

    QSize iconSize() const;
    void setIconSize(const QSize &iconSize);

    Qt::ToolButtonStyle toolButtonStyle() const;
    void setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle);

#if QT_CONFIG(dockwidget)
    bool isAnimated() const;
    bool isDockNestingEnabled() const;
#endif

#if QT_CONFIG(tabbar)
    bool documentMode() const;
    void setDocumentMode(bool enabled);
#endif

#if QT_CONFIG(tabwidget)
    QTabWidget::TabShape tabShape() const;
    void setTabShape(QTabWidget::TabShape tabShape);
    QTabWidget::TabPosition tabPosition(Qt::DockWidgetArea area) const;
    void setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition);
#endif // QT_CONFIG(tabwidget)

    void setDockOptions(DockOptions options);
    DockOptions dockOptions() const;

    bool isSeparator(const QPoint &pos) const;

#if QT_CONFIG(menubar)
    QMenuBar *menuBar() const;
    void setMenuBar(QMenuBar *menubar);

    QWidget  *menuWidget() const;
    void setMenuWidget(QWidget *menubar);
#endif

#if QT_CONFIG(statusbar)
    QStatusBar *statusBar() const;
    void setStatusBar(QStatusBar *statusbar);
#endif



#if QT_CONFIG(dockwidget)
    void setCorner(Qt::Corner corner, Qt::DockWidgetArea area);
    Qt::DockWidgetArea corner(Qt::Corner corner) const;
#endif

#ifndef QT_NO_TOOLBAR
    void addToolBarBreak(Qt::ToolBarArea area = Qt::TopToolBarArea);
    void insertToolBarBreak(QToolBar *before);

    void addToolBar(Qt::ToolBarArea area, QToolBar *toolbar);
    void addToolBar(QToolBar *toolbar);
    QToolBar *addToolBar(const QString &title);
    void insertToolBar(QToolBar *before, QToolBar *toolbar);
    void removeToolBar(QToolBar *toolbar);
    void removeToolBarBreak(QToolBar *before);

    Qt::ToolBarArea toolBarArea(QToolBar *toolbar) const;
    bool toolBarBreak(QToolBar *toolbar) const;
#endif
#if QT_CONFIG(dockwidget)
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget);
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget,
                       Qt::Orientation orientation);
    void splitDockWidget(QDockWidget *after, QDockWidget *dockwidget,
                         Qt::Orientation orientation);
    void tabifyDockWidget(QDockWidget *first, QDockWidget *second);
    QList<QDockWidget*> tabifiedDockWidgets(QDockWidget *dockwidget) const;
    void removeDockWidget(QDockWidget *dockwidget);
    bool restoreDockWidget(QDockWidget *dockwidget);

    Qt::DockWidgetArea dockWidgetArea(QDockWidget *dockwidget) const;

    void resizeDocks(const QList<QDockWidget *> &docks,
                     const QList<int> &sizes, Qt::Orientation orientation);
#endif // QT_CONFIG(dockwidget)

    QByteArray saveState(int version = 0) const;
    bool restoreState(const QByteArray &state, int version = 0);

#if QT_CONFIG(menu)
    virtual QMenu *createPopupMenu();
#endif



private:
    //Q_DECLARE_PRIVATE(QMainWindow)
    //Q_DISABLE_COPY(QMainWindow)
public:
	enum DockOption {
		AnimatedDocks = 0x01,
		AllowNestedDocks = 0x02,
		AllowTabbedDocks = 0x04,
		ForceTabbedDocks = 0x08,  // implies AllowTabbedDocks, !AllowNestedDocks
		VerticalTabs = 0x10,	  // implies AllowTabbedDocks
		GroupedDragging = 0x20	  // implies AllowTabbedDocks
	};
	enum DockOptions {
		AnimatedDocks = 0x01,
		AllowNestedDocks = 0x02,
		AllowTabbedDocks = 0x04,
		ForceTabbedDocks = 0x08,  // implies AllowTabbedDocks, !AllowNestedDocks
		VerticalTabs = 0x10,	  // implies AllowTabbedDocks
		GroupedDragging = 0x20	  // implies AllowTabbedDocks
	};

};

//Q_DECLARE_OPERATORS_FOR_FLAGS(QMainWindow::DockOptions)

#endif // QDYNAMICMAINWINDOW_H

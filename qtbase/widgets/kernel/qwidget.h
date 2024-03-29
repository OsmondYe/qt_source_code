#ifndef QWIDGET_H
#define QWIDGET_H


//oye
#define WId int*;

// oye  相比WidgetPrivate 这个很直接放置了widget自己可以让client看到的数据
class QWidgetData
{
public:
    WId winid;
    uint widget_attributes;
    Qt::WindowFlags window_flags;
    uint window_state : 4;
    uint focus_policy : 4;				// 有焦点
    uint sizehint_forced :1;
    uint is_closing :1;
    uint in_show : 1;
    uint in_set_window_state : 1;
    mutable uint fstrut_dirty : 1;		// frame strut dirty
    uint context_menu_policy : 3;
    uint window_modality : 2;
    uint in_destructor : 1;
    uint unused : 13;
    QRect crect;						// content rect
    mutable QPalette pal;
    QFont fnt;
    QRect wrect;
};

class QWidgetPrivate;

class  QWidget : public QObject, public QPaintDevice
{
	// QWidgetPrivate
	inline QWidgetPrivate* d_func() { return reinterpret_cast<QWidgetPrivate *>(qGetPtrHelper(d_ptr)); }
private:	
    QWidgetData *data;	
public:
    QLayout *layout() const;
    void setLayout(QLayout *);
	
    explicit QWidget(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~QWidget();

public: // override QPaintDevice	
    int devType() const override {return QInternal::Widget;}
	int metric(PaintDeviceMetric) const override;
    void initPainter(QPainter *painter) const override;
    QPaintDevice *redirected(QPoint *offset) const override;
    QPainter *sharedPainter() const override;
public:
    void stackUnder(QWidget*);
    void move(int x, int y);
    void move(const QPoint &);
    void resize(int w, int h);
    void resize(const QSize &);
    inline void setGeometry(int x, int y, int w, int h);
    void setGeometry(const QRect &);
    QByteArray saveGeometry() const;
    bool restoreGeometry(const QByteArray &geometry);
    void adjustSize();
    bool isVisible() const;
    bool isVisibleTo(const QWidget *) const;
    inline bool isHidden() const;

    bool isMinimized() const;
    bool isMaximized() const;
    bool isFullScreen() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    QSizePolicy sizePolicy() const;
    void setSizePolicy(QSizePolicy); // size发生变化时, 横竖轴各自的策略
    inline void setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical);
    virtual int heightForWidth(int) const;
    virtual bool hasHeightForWidth() const;

public:  // GUI style setting    
    QStyle *style() const;
    void setStyle(QStyle *);
	//oye 使用场景, QLineEdit绘制字体时,颜色
    const QPalette &palette() const;
    void setPalette(const QPalette &);

    void setBackgroundRole(QPalette::ColorRole);
    QPalette::ColorRole backgroundRole() const;

	// oye 调色盘取色时参考的角色Base, Dark, Light,Button???
    void setForegroundRole(QPalette::ColorRole);
    QPalette::ColorRole foregroundRole() const;

    const QFont &font() const;
    void setFont(const QFont &);
	// oye, PushButton use it to caculate the size of text
    QFontMetrics fontMetrics() const{ return QFontMetrics(data->fnt); }
    QFontInfo fontInfo() const;

    QString styleSheet() const;
public Q_SLOTS:
    void setStyleSheet(const QString& styleSheet);

protected:      // Event handlers
	// 自然而言的模板思想, 在event的switch中,通过调用虚函数,比如paintEvent,给基类一个机会来处理	
    bool event(QEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void enterEvent(QEvent *event){}
    virtual void leaveEvent(QEvent *event){}
    virtual void paintEvent(QPaintEvent *event){}	// 子类才知道怎么画自己
    virtual void moveEvent(QMoveEvent *event){}
    virtual void resizeEvent(QResizeEvent *event){}
    virtual void actionEvent(QActionEvent *event){}
    virtual void closeEvent(QCloseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual void tabletEvent(QTabletEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *event);
    virtual void dropEvent(QDropEvent *event);

    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);

	// oye 各种状态改变的统一扩展点, 给子类机会参与
	// theme, style, keyboard, font,palette,
	// WindowStateChange, LanguageChange,LayoutDirectionChange
	// 很多子控件会重载此函数来监控StyleChange, 进而修改自己的style,并重绘
    // Misc. protected functions
    virtual void changeEvent(QEvent *); 

    virtual void inputMethodEvent(QInputMethodEvent *);



public:  // Qt window
    WId winId() const;
    WId effectiveWinId() const;
    void createWinId(); // internal, going away
    inline WId internalWinId() const { return data->winid; }

	bool isWindow() const;
	Qt::WindowModality windowModality() const;
    void setWindowModality(Qt::WindowModality windowModality);
    QWidget *window() const;
    inline QWidget *topLevelWidget() const { return window(); }
	QString windowTitle() const;
	void setWindowIcon(const QIcon &icon);
	QIcon windowIcon() const;
	void setWindowIconText(const QString &);
	QString windowIconText() const;
	void setWindowRole(const QString &);
	QString windowRole() const;
	void setWindowFilePath(const QString &filePath);
	QString windowFilePath() const;

	void setWindowOpacity(qreal level);
	qreal windowOpacity() const;

	bool isWindowModified() const;

    bool isActiveWindow() const;
    void activateWindow();
    Qt::WindowStates windowState() const;
    void setWindowState(Qt::WindowStates state);
    void overrideWindowState(Qt::WindowStates state);
	
	inline Qt::WindowType windowType() const
	{ return static_cast<Qt::WindowType>(int(data->window_flags & Qt::WindowType_Mask)); }
	void setWindowFlags(Qt::WindowFlags type);
	
    inline Qt::WindowFlags windowFlags() const;
    void setWindowFlag(Qt::WindowType, bool on = true);
	
    void overrideWindowFlags(Qt::WindowFlags type);
	
	QWindow *windowHandle() const;
 	static QWidget *createWindowContainer(
				QWindow *window, 
				QWidget *parent=Q_NULLPTR, 
				Qt::WindowFlags flags=Qt::WindowFlags());
protected:
	// 创建widget窗口
    void create(WId = 0, bool initializeWindow = true,   bool destroyOldWindow = true);
    void destroy(bool destroyWindow = true,    bool destroySubWindows = true);
public: // designed for UIC		
    void setupUi(QWidget *widget);
	

    // Widget types and states
public:  // Position
    bool isTopLevel() const;   
    bool isModal() const;

    bool isEnabled() const;
    bool isEnabledTo(const QWidget *) const;
    bool isEnabledToTLW() const;

public Q_SLOTS:
    void setEnabled(bool);
    void setDisabled(bool);
    void setWindowModified(bool);

public: // Widget coordinates
    QRect frameGeometry() const;  // Widget一开始就考虑了frame的问题?
    const QRect &geometry() const;
    QRect normalGeometry() const;

    int x() const;
    int y() const;
    QPoint pos() const;
    QSize frameSize() const;
    QSize size() const;
    inline int width() const;
    inline int height() const;
    inline QRect rect() const;
    QRect childrenRect() const;
    QRegion childrenRegion() const;

    QSize minimumSize() const;
    QSize maximumSize() const;
    int minimumWidth() const;
    int minimumHeight() const;
    int maximumWidth() const;
    int maximumHeight() const;
    void setMinimumSize(const QSize &);
    void setMinimumSize(int minw, int minh);
    void setMaximumSize(const QSize &);
    void setMaximumSize(int maxw, int maxh);
    void setMinimumWidth(int minw);
    void setMinimumHeight(int minh);
    void setMaximumWidth(int maxw);
    void setMaximumHeight(int maxh);

    QSize sizeIncrement() const;
    void setSizeIncrement(const QSize &);
    void setSizeIncrement(int w, int h);
    QSize baseSize() const;
    void setBaseSize(const QSize &);
    void setBaseSize(int basew, int baseh);

    void setFixedSize(const QSize &);
    void setFixedSize(int w, int h);
    void setFixedWidth(int w);
    void setFixedHeight(int h);

    // Widget coordinate mapping

    QPoint mapToGlobal(const QPoint &) const;
    QPoint mapFromGlobal(const QPoint &) const;
    QPoint mapToParent(const QPoint &) const;
    QPoint mapFromParent(const QPoint &) const;
    QPoint mapTo(const QWidget *, const QPoint &) const;
    QPoint mapFrom(const QWidget *, const QPoint &) const;
	
public:
    QWidget *nativeParentWidget() const;

    QCursor cursor() const;
    void setCursor(const QCursor &);
    void unsetCursor();

    void setMouseTracking(bool enable);
    bool hasMouseTracking() const;
    bool underMouse() const;

    void setTabletTracking(bool enable);
    bool hasTabletTracking() const;

    void setMask(const QBitmap &);
    void setMask(const QRegion &);
    QRegion mask() const;
    void clearMask();

    void render(QPaintDevice *target, const QPoint &targetOffset = QPoint(),
                const QRegion &sourceRegion = QRegion(),
                RenderFlags renderFlags = RenderFlags(DrawWindowBackground | DrawChildren));

    void render(QPainter *painter, const QPoint &targetOffset = QPoint(),
                const QRegion &sourceRegion = QRegion(),
                RenderFlags renderFlags = RenderFlags(DrawWindowBackground | DrawChildren));

    Q_INVOKABLE QPixmap grab(const QRect &rectangle = QRect(QPoint(0, 0), QSize(-1, -1)));

    QGraphicsEffect *graphicsEffect() const;
    void setGraphicsEffect(QGraphicsEffect *effect);


public Q_SLOTS:
    void setWindowTitle(const QString &);
public:
    void setToolTip(const QString &);
    QString toolTip() const;
    void setToolTipDuration(int msec);
    int toolTipDuration() const;
    void setStatusTip(const QString &);
    QString statusTip() const;
    void setWhatsThis(const QString &);
    QString whatsThis() const;

    void setLayoutDirection(Qt::LayoutDirection direction);
    Qt::LayoutDirection layoutDirection() const;
    void unsetLayoutDirection();

    void setLocale(const QLocale &locale);
    QLocale locale() const;
    void unsetLocale();

    inline bool isRightToLeft() const { return layoutDirection() == Qt::RightToLeft; }
    inline bool isLeftToRight() const { return layoutDirection() == Qt::LeftToRight; }

public Q_SLOTS:
    inline void setFocus() { setFocus(Qt::OtherFocusReason); }

public:

    void clearFocus();
    void setFocus(Qt::FocusReason reason);
	
    Qt::FocusPolicy focusPolicy() const;
    void setFocusPolicy(Qt::FocusPolicy policy);
    bool hasFocus() const;
    static void setTabOrder(QWidget *, QWidget *);
    void setFocusProxy(QWidget *);
    QWidget *focusProxy() const;
    Qt::ContextMenuPolicy contextMenuPolicy() const;
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    // Grab functions
    void grabMouse();
	
    void grabMouse(const QCursor &);
    void releaseMouse();
    void grabKeyboard();
    void releaseKeyboard();
    int grabShortcut(const QKeySequence &key, Qt::ShortcutContext context = Qt::WindowShortcut);
    void releaseShortcut(int id);
    void setShortcutEnabled(int id, bool enable = true);
    void setShortcutAutoRepeat(int id, bool enable = true);
    static QWidget *mouseGrabber();
    static QWidget *keyboardGrabber();

    // Update/refresh functions
    inline bool updatesEnabled() const;
    void setUpdatesEnabled(bool enable);

    QGraphicsProxyWidget *graphicsProxyWidget() const;

public Q_SLOTS:
    void update();  // post RePaint Event
    void repaint();

public:
    inline void update(int x, int y, int w, int h);
    void update(const QRect&);
    void update(const QRegion&);

    void repaint(int x, int y, int w, int h);
    void repaint(const QRect &);
    void repaint(const QRegion &);

public Q_SLOTS:
    // Widget management functions
    virtual void setVisible(bool visible);
    void setHidden(bool hidden);
    void show();
    void hide();

    void showMinimized();
    void showMaximized();
    void showFullScreen();
    void showNormal();

    bool close();
    void raise();
    void lower();



public:
    QRegion visibleRegion() const;

    void setContentsMargins(int left, int top, int right, int bottom);
    void setContentsMargins(const QMargins &margins);
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;
    QMargins contentsMargins() const;

    QRect contentsRect() const; // oye  rect() - contentsMargins();


    void updateGeometry();

    void setParent(QWidget *parent);
    void setParent(QWidget *parent, Qt::WindowFlags f);

    void scroll(int dx, int dy);
    void scroll(int dx, int dy, const QRect&);

    // Misc. functions

    QWidget *focusWidget() const;
    QWidget *nextInFocusChain() const;
    QWidget *previousInFocusChain() const;

    // drag and drop
    bool acceptDrops() const;
    void setAcceptDrops(bool on);

    //actions
    void addAction(QAction *action);
    void addActions(const QList<QAction*> &actions);
    void insertActions(const QAction *before, const QList<QAction*> &actions);
    void addActions(QList<QAction*> actions);
    void insertActions(QAction *before, QList<QAction*> actions);
    void insertAction(QAction *before, QAction *action);
    void removeAction(QAction *action);
    QList<QAction*> actions() const;

    QWidget *parentWidget() const;

    static QWidget *find(WId);
    inline QWidget *childAt(int x, int y) const;
    QWidget *childAt(const QPoint &p) const;

    void setAttribute(Qt::WidgetAttribute, bool on = true);
    inline bool testAttribute(Qt::WidgetAttribute) const;

	// oye widget作为基类,居然给的是空实现
    QPaintEngine *paintEngine() const Q_DECL_OVERRIDE;

    void ensurePolished() const;

    bool isAncestorOf(const QWidget *child) const;

    bool hasEditFocus() const;
    void setEditFocus(bool on);

    bool autoFillBackground() const;
    void setAutoFillBackground(bool enabled);

    QBackingStore *backingStore() const;
   
Q_SIGNALS:
    void windowTitleChanged(const QString &title);
    void windowIconChanged(const QIcon &icon);
    void windowIconTextChanged(const QString &iconText);
    void customContextMenuRequested(const QPoint &pos);


public:
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    Qt::InputMethodHints inputMethodHints() const;	
    void setInputMethodHints(Qt::InputMethodHints hints);

protected Q_SLOTS:
    void updateMicroFocus();



    friend class QDataWidgetMapperPrivate; // for access to focusNextPrevChild
protected:
    
    virtual bool focusNextPrevChild(bool next);
    inline bool focusNextChild() { return focusNextPrevChild(true); }
    inline bool focusPreviousChild() { return focusNextPrevChild(false); }


private:
    void setBackingStore(QBackingStore *store);

    bool testAttribute_helper(Qt::WidgetAttribute) const;

    QLayout *takeLayout();

    friend class QBackingStoreDevice;
    friend class QWidgetBackingStore;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QBaseApplication;
    friend class QPainter;
    friend class QPainterPrivate;
    friend class QPixmap; // for QPixmap::fill()
    friend class QFontMetrics;
    friend class QFontInfo;
    friend class QLayout;
    friend class QWidgetItem;
    friend class QWidgetItemV2;
    friend class QGLContext;
    friend class QGLWidget;
    friend class QGLWindowSurface;
    friend class QX11PaintEngine;
    friend class QWin32PaintEngine;
    friend class QShortcutPrivate;
    friend class QWindowSurface;
    friend class QGraphicsProxyWidget;
    friend class QGraphicsProxyWidgetPrivate;
    friend class QStyleSheetStyle;
    friend struct QWidgetExceptionCleaner;
    friend class QWidgetWindow;
    friend class QAccessibleWidget;
    friend class QAccessibleTable;
    friend class QGestureManager;
    friend class QWinNativePanGestureRecognizer;
    friend class QWidgetEffectSourcePrivate;
    friend class QDesktopScreenWidget;


    friend  QWidgetData *qt_qwidget_data(QWidget *widget);
    friend  QWidgetPrivate *qt_widget_private(QWidget *widget);

private:
    //Q_PRIVATE_SLOT(d_func(), void _q_showIfNotHidden())
protected:
    QWidget(QWidgetPrivate &d, QWidget* parent, Qt::WindowFlags f);
public:
	enum RenderFlag {
		DrawWindowBackground = 0x1,
		DrawChildren = 0x2,
		IgnoreMask = 0x4
	};
	enum RenderFlags {
		DrawWindowBackground = 0x1,
		DrawChildren = 0x2,
		IgnoreMask = 0x4
	};
};

// oye 把qobject_cast 看成函数重载
//    这里特化了模板函数qobject_cast
//    当要转型为QWidget*指针时, 
template <> inline QWidget *qobject_cast<QWidget*>(QObject *o)
{
	// 利用直接在Object里面写有的标记,做快速判断和转化
    if (!o || !o->isWidgetType()) return Q_NULLPTR;
    return static_cast<QWidget*>(o);
}
template <> inline const QWidget *qobject_cast<const QWidget*>(const QObject *o)
{
    if (!o || !o->isWidgetType()) return Q_NULLPTR;
    return static_cast<const QWidget*>(o);
}


inline QWidgetData *qt_qwidget_data(QWidget *widget)
{
    return widget->data;
}

inline QWidgetPrivate *qt_widget_private(QWidget *widget)
{
    return widget->d_func();
}



inline QWidget *QWidget::childAt(int ax, int ay) const
{ return childAt(QPoint(ax, ay)); }


inline Qt::WindowFlags QWidget::windowFlags() const
{ return data->window_flags; }

inline bool QWidget::isTopLevel() const
{ return (windowType() & Qt::Window); }

inline bool QWidget::isWindow() const
{ return (windowType() & Qt::Window); }

inline bool QWidget::isEnabled() const
{ return !testAttribute(Qt::WA_Disabled); }

inline bool QWidget::isModal() const
{ return data->window_modality != Qt::NonModal; }

inline bool QWidget::isEnabledToTLW() const
{ return isEnabled(); }

inline int QWidget::minimumWidth() const
{ return minimumSize().width(); }

inline int QWidget::minimumHeight() const
{ return minimumSize().height(); }

inline int QWidget::maximumWidth() const
{ return maximumSize().width(); }

inline int QWidget::maximumHeight() const
{ return maximumSize().height(); }

inline void QWidget::setMinimumSize(const QSize &s)
{ setMinimumSize(s.width(),s.height()); }

inline void QWidget::setMaximumSize(const QSize &s)
{ setMaximumSize(s.width(),s.height()); }

inline void QWidget::setSizeIncrement(const QSize &s)
{ setSizeIncrement(s.width(),s.height()); }

inline void QWidget::setBaseSize(const QSize &s)
{ setBaseSize(s.width(),s.height()); }

inline const QFont &QWidget::font() const
{ return data->fnt; }

inline QFontInfo QWidget::fontInfo() const
{ return QFontInfo(data->fnt); }

inline void QWidget::setMouseTracking(bool enable)
{ setAttribute(Qt::WA_MouseTracking, enable); }

inline bool QWidget::hasMouseTracking() const
{ return testAttribute(Qt::WA_MouseTracking); }

inline bool QWidget::underMouse() const
{ return testAttribute(Qt::WA_UnderMouse); }

inline void QWidget::setTabletTracking(bool enable)
{ setAttribute(Qt::WA_TabletTracking, enable); }

inline bool QWidget::hasTabletTracking() const
{ return testAttribute(Qt::WA_TabletTracking); }

inline bool QWidget::updatesEnabled() const
{ return !testAttribute(Qt::WA_UpdatesDisabled); }

inline void QWidget::update(int ax, int ay, int aw, int ah)
{ update(QRect(ax, ay, aw, ah)); }

inline bool QWidget::isVisible() const
{ return testAttribute(Qt::WA_WState_Visible); }

inline bool QWidget::isHidden() const
{ return testAttribute(Qt::WA_WState_Hidden); }

inline void QWidget::move(int ax, int ay)
{ move(QPoint(ax, ay)); }

inline void QWidget::resize(int w, int h)
{ resize(QSize(w, h)); }

inline void QWidget::setGeometry(int ax, int ay, int aw, int ah)
{ setGeometry(QRect(ax, ay, aw, ah)); }

inline QRect QWidget::rect() const
{ return QRect(0,0,data->crect.width(),data->crect.height()); }

inline const QRect &QWidget::geometry() const
{ return data->crect; }

inline QSize QWidget::size() const
{ return data->crect.size(); }

inline int QWidget::width() const
{ return data->crect.width(); }

inline int QWidget::height() const
{ return data->crect.height(); }

inline QWidget *QWidget::parentWidget() const
{ return static_cast<QWidget *>(QObject::parent()); }

inline void QWidget::setSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver)
{ setSizePolicy(QSizePolicy(hor, ver)); }

inline bool QWidget::testAttribute(Qt::WidgetAttribute attribute) const
{
    if (attribute < int(8*sizeof(uint)))
        return data->widget_attributes & (1<<attribute);
    return testAttribute_helper(attribute);
}



#endif // QWIDGET_H

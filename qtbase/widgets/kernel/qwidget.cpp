
static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right()) &&
            qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); // qapplication.cpp
extern QDesktopWidget *qt_desktopWidget; // qapplication.cpp


QWidgetBackingStoreTracker::QWidgetBackingStoreTracker()
    :   m_ptr(0)
{

}

QWidgetBackingStoreTracker::~QWidgetBackingStoreTracker()
{
    delete m_ptr;
}


void QWidgetBackingStoreTracker::create(QWidget *widget)
{
    destroy();
    m_ptr = new QWidgetBackingStore(widget);
}


void QWidgetBackingStoreTracker::destroy()
{
    delete m_ptr;
    m_ptr = 0;
    m_widgets.clear();
}


void QWidgetBackingStoreTracker::registerWidget(QWidget *w)
{
    Q_ASSERT(m_ptr);
    Q_ASSERT(w->internalWinId());
    Q_ASSERT(qt_widget_private(w)->maybeBackingStore() == m_ptr);
    m_widgets.insert(w);
}


void QWidgetBackingStoreTracker::unregisterWidget(QWidget *w)
{
    if (m_widgets.remove(w) && m_widgets.isEmpty()) {
        delete m_ptr;
        m_ptr = 0;
    }
}


void QWidgetBackingStoreTracker::unregisterWidgetSubtree(QWidget *widget)
{
    unregisterWidget(widget);
    foreach (QObject *child, widget->children())
        if (QWidget *childWidget = qobject_cast<QWidget *>(child))
            unregisterWidgetSubtree(childWidget);
}

QWidgetPrivate::QWidgetPrivate(int version)
    : QObjectPrivate(version)
      , extra(0)
      , focus_next(0)
      , focus_prev(0)
      , focus_child(0)
      , layout(0)
      , needsFlush(0)
      , redirectDev(0)
      , widgetItem(0)
      , extraPaintEngine(0)
      , polished(0)
      , graphicsEffect(0)
      , imHints(Qt::ImhNone)
      , toolTipDuration(-1)
      , inheritedFontResolveMask(0)
      , inheritedPaletteResolveMask(0)
      , leftmargin(0)
      , topmargin(0)
      , rightmargin(0)
      , bottommargin(0)
      , leftLayoutItemMargin(0)
      , topLayoutItemMargin(0)
      , rightLayoutItemMargin(0)
      , bottomLayoutItemMargin(0)
      , hd(0)
      , size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred)
      , fg_role(QPalette::NoRole)
      , bg_role(QPalette::NoRole)
      , dirtyOpaqueChildren(1)
      , isOpaque(0)
      , retainSizeWhenHiddenChanged(0)
      , inDirtyList(0)
      , isScrolled(0)
      , isMoved(0)
      , usesDoubleBufferedGLContext(0)
      , mustHaveWindowHandle(0)
      , renderToTexture(0)
      , textureChildSeen(0)
      , inheritsInputMethodHints(0)
      , renderToTextureReallyDirty(1)
      , renderToTextureComposeActive(0)
      , childrenHiddenByWState(0)
      , childrenShownByExpose(0)
      , noPaintOnScreen(0)
{
    if (!qApp) {
        qFatal("QWidget: Must construct a QApplication before a QWidget");
        return;
    }

    isWidget = true;
    memset(high_attributes, 0, sizeof(high_attributes));

    static int count = 0;
    qDebug() << "widgets" << ++count;
}


QWidgetPrivate::~QWidgetPrivate()
{
    if (widgetItem)
        widgetItem->wid = 0;

    if (extra)
        deleteExtra();
}


void QWidgetPrivate::scrollChildren(int dx, int dy)
{
    QWidget* const q = q_func();
    if (q->children().size() > 0) {        // scroll children
        QPoint pd(dx, dy);
        QObjectList childObjects = q->children();
        for (int i = 0; i < childObjects.size(); ++i) { // move all children
            QWidget *w = qobject_cast<QWidget*>(childObjects.at(i));
            if (w && !w->isWindow()) {
                QPoint oldp = w->pos();
                QRect  r(w->pos() + pd, w->size());
                w->data->crect = r;
                if (w->testAttribute(Qt::WA_WState_Created))
                    w->d_func()->setWSGeometry();
                w->d_func()->setDirtyOpaqueRegion();
                QMoveEvent e(r.topLeft(), oldp);
                QApplication::sendEvent(w, &e);
            }
        }
    }
}

void QWidgetPrivate::setWSGeometry()
{
    QWidget* const q = q_func();
    if (QWindow *window = q->windowHandle())
        window->setGeometry(data.crect);
}

void QWidgetPrivate::updateWidgetTransform(QEvent *event)
{
    QWidget* const q = q_func();
    if (q == QGuiApplication::focusObject() || event->type() == QEvent::FocusIn) {
        QTransform t;
        QPoint p = q->mapTo(q->topLevelWidget(), QPoint(0,0));
        t.translate(p.x(), p.y());
        QGuiApplication::inputMethod()->setInputItemTransform(t);
        QGuiApplication::inputMethod()->setInputItemRectangle(q->rect());
        QGuiApplication::inputMethod()->update(Qt::ImInputItemClipRectangle);
    }
}

#ifdef QT_KEYPAD_NAVIGATION
QPointer<QWidget> QWidgetPrivate::editingWidget;


bool QWidget::hasEditFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
    return QWidgetPrivate::editingWidget == w;
}


void QWidget::setEditFocus(bool on)
{
    QWidget *f = this;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if (QWidgetPrivate::editingWidget && QWidgetPrivate::editingWidget != f)
        QWidgetPrivate::editingWidget->setEditFocus(false);

    if (on && !f->hasFocus())
        f->setFocus();

    if ((!on && !QWidgetPrivate::editingWidget)
        || (on && QWidgetPrivate::editingWidget == f)) {
        return;
    }

    if (!on && QWidgetPrivate::editingWidget == f) {
        QWidgetPrivate::editingWidget = 0;
        QEvent event(QEvent::LeaveEditFocus);
        QApplication::sendEvent(f, &event);
        QApplication::sendEvent(f->style(), &event);
    } else if (on) {
        QWidgetPrivate::editingWidget = f;
        QEvent event(QEvent::EnterEditFocus);
        QApplication::sendEvent(f, &event);
        QApplication::sendEvent(f->style(), &event);
    }
}
#endif


bool QWidget::autoFillBackground() const
{
    QWidgetPrivate * const d = d_func();
    return d->extra && d->extra->autoFillBackground;
}

void QWidget::setAutoFillBackground(bool enabled)
{
    QWidgetPrivate * const d = d_func();
    if (!d->extra)
        d->createExtra();
    if (d->extra->autoFillBackground == enabled)
        return;

    d->extra->autoFillBackground = enabled;
    d->updateIsOpaque();
    update();
    d->updateIsOpaque();
}


QWidgetMapper *QWidgetPrivate::mapper = 0;          // widget with wid
QWidgetSet *QWidgetPrivate::allWidgets = 0;         // widgets with no wid

QRegion qt_dirtyRegion(QWidget *widget)
{
    if (!widget)
        return QRegion();

    QWidgetBackingStore *bs = qt_widget_private(widget)->maybeBackingStore();
    if (!bs)
        return QRegion();

    return bs->dirtyRegion(widget);
}



struct QWidgetExceptionCleaner
{
    /* this cleans up when the constructor throws an exception */
    static inline void cleanup(QWidget *that, QWidgetPrivate *d)
    {
#ifdef QT_NO_EXCEPTIONS
        Q_UNUSED(that);
        Q_UNUSED(d);
#else
        QWidgetPrivate::allWidgets->remove(that);
        if (d->focus_next != that) {
            if (d->focus_next)
                d->focus_next->d_func()->focus_prev = d->focus_prev;
            if (d->focus_prev)
                d->focus_prev->d_func()->focus_next = d->focus_next;
        }
#endif
    }
};

QWidget::QWidget(QWidget *parent, Qt::WindowFlags f)
    : QObject(*new QWidgetPrivate, 0), QPaintDevice()
{
    QT_TRY {
        d_func()->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}



QWidget::QWidget(QWidgetPrivate &dd, QWidget* parent, Qt::WindowFlags f)
    : QObject(dd, 0), QPaintDevice()
{
    QWidgetPrivate * const d = d_func();
    QT_TRY {
        d->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}


int QWidget::devType() const
{
    return QInternal::Widget;
}


//### w is a "this" ptr, passed as a param because QWorkspace needs special logic
void QWidgetPrivate::adjustFlags(Qt::WindowFlags &flags, QWidget *w)
{
    bool customize =  (flags & (Qt::CustomizeWindowHint
            | Qt::FramelessWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowSystemMenuHint
            | Qt::WindowMinimizeButtonHint
            | Qt::WindowMaximizeButtonHint
            | Qt::WindowCloseButtonHint
            | Qt::WindowContextHelpButtonHint));

    uint type = (flags & Qt::WindowType_Mask);

    if ((type == Qt::Widget || type == Qt::SubWindow) && w && !w->parent()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

    if (flags & Qt::CustomizeWindowHint) {
        // modify window flags to make them consistent.
        // Only enable this on non-Mac platforms. Since the old way of doing this would
        // interpret WindowSystemMenuHint as a close button and we can't change that behavior
        // we can't just add this in.
#if 1 // Used to be excluded in Qt4 for Q_WS_MAC
        if ((flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint))
#  ifdef Q_OS_WIN
            && type != Qt::Dialog // QTBUG-2027, allow for menu-less dialogs.
#  endif
           ) {
            flags |= Qt::WindowSystemMenuHint;
#else
        if (flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
                     | Qt::WindowSystemMenuHint)) {
#endif
            flags |= Qt::WindowTitleHint;
            flags &= ~Qt::FramelessWindowHint;
        }
    } else if (customize && !(flags & Qt::FramelessWindowHint)) {
        // if any of the window hints that affect the titlebar are set
        // and the window is supposed to have frame, we add a titlebar
        // and system menu by default.
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowTitleHint;
    }
    if (customize)
        ; // don't modify window flags if the user explicitly set them.
    else if (type == Qt::Dialog || type == Qt::Sheet)
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint;
    else if (type == Qt::Tool)
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    else
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint |
                Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint;
    if (w->testAttribute(Qt::WA_TransparentForMouseEvents))
        flags |= Qt::WindowTransparentForInput;
}

void QWidgetPrivate::init(QWidget *parentWidget, Qt::WindowFlags f)
{
    QWidget* const q = q_func();
    if (Q_UNLIKELY(!qobject_cast<QApplication *>(QCoreApplication::instance())))
        qFatal("QWidget: Cannot create a QWidget without QApplication");

    if (allWidgets)
        allWidgets->insert(q);

    int targetScreen = -1;
    if (parentWidget && parentWidget->windowType() == Qt::Desktop) {
        const QDesktopScreenWidget *sw = qobject_cast<const QDesktopScreenWidget *>(parentWidget);
        targetScreen = sw ? sw->screenNumber() : 0;
        parentWidget = 0;
    }

    q->data = &data;

    if (!parent) {
        Q_ASSERT_X(q->thread() == qApp->thread(), 
			"QWidget", "Widgets must be created in the GUI thread.");
    }

    if (targetScreen >= 0) {
        topData()->initialScreenIndex = targetScreen;
        if (QWindow *window = q->windowHandle())
            window->setScreen(QGuiApplication::screens().value(targetScreen, Q_NULLPTR));
    }

    data.fstrut_dirty = true;

    data.winid = 0;
    data.widget_attributes = 0;
    data.window_flags = f;
    data.window_state = 0;
    data.focus_policy = 0;
    data.context_menu_policy = Qt::DefaultContextMenu;
    data.window_modality = Qt::NonModal;

    data.sizehint_forced = 0;
    data.is_closing = 0;
    data.in_show = 0;
    data.in_set_window_state = 0;
    data.in_destructor = false;

    // Widgets with Qt::MSWindowsOwnDC (typically QGLWidget) must have a window handle.
    if (f & Qt::MSWindowsOwnDC) {
        mustHaveWindowHandle = 1;
        q->setAttribute(Qt::WA_NativeWindow);
    }
    q->setAttribute(Qt::WA_QuitOnClose); // might be cleared in adjustQuitOnCloseAttribute()
    adjustQuitOnCloseAttribute();

    q->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea);
    q->setAttribute(Qt::WA_WState_Hidden);

    //give potential windows a bigger "pre-initial" size; create_sys() will give them a new size later
    data.crect = parentWidget ? QRect(0,0,100,30) : QRect(0,0,640,480);
    focus_next = focus_prev = q;

    if ((f & Qt::WindowType_Mask) == Qt::Desktop)
        q->create();
    else if (parentWidget)
        q->setParent(parentWidget, data.window_flags);
    else {
        adjustFlags(data.window_flags, q);
        resolveLayoutDirection();
        // opaque system background?
        const QBrush &background = q->palette().brush(QPalette::Window);
        setOpaque(q->isWindow() && background.style() != Qt::NoBrush && background.isOpaque());
    }
    data.fnt = QFont(data.fnt, q);
#if 0 // Used to be included in Qt4 for Q_WS_X11
    data.fnt.x11SetScreen(xinfo.screen());
#endif

    q->setAttribute(Qt::WA_PendingMoveEvent);
    q->setAttribute(Qt::WA_PendingResizeEvent);

    if (++QWidgetPrivate::instanceCounter > QWidgetPrivate::maxInstances)
        QWidgetPrivate::maxInstances = QWidgetPrivate::instanceCounter;

    if (QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation)) // ### fixme: Qt 6: Remove AA_ImmediateWidgetCreation.
        q->create();

    QEvent e(QEvent::Create);
    QApplication::sendEvent(q, &e);
    QApplication::postEvent(q, new QEvent(QEvent::PolishRequest));

    extraPaintEngine = 0;
}


void QWidgetPrivate::createRecursively()
{
    QWidget* const q = q_func();
    q->create(0, true, true);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (child && !child->isHidden() && !child->isWindow() && !child->testAttribute(Qt::WA_WState_Created))
            child->d_func()->createRecursively();
    }
}



void QWidget::create(WId window, bool initializeWindow, bool destroyOldWindow)
{
    QWidgetPrivate * const d = d_func();
    if (Q_UNLIKELY(window))
        qWarning("QWidget::create(): Parameter 'window' does not have any effect.");
    if (testAttribute(Qt::WA_WState_Created) && window == 0 && internalWinId())
        return;

    if (d->data.in_destructor)
        return;

    Qt::WindowType type = windowType();
    Qt::WindowFlags &flags = data->window_flags;

    if ((type == Qt::Widget || type == Qt::SubWindow) && !parentWidget()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

    if (QWidget *parent = parentWidget()) {
        if (type & Qt::Window) {
            if (!parent->testAttribute(Qt::WA_WState_Created))
                parent->createWinId();
        } else if (testAttribute(Qt::WA_NativeWindow) && !parent->internalWinId()
                   && !testAttribute(Qt::WA_DontCreateNativeAncestors)) {
            // We're about to create a native child widget that doesn't have a native parent;
            // enforce a native handle for the parent unless the Qt::WA_DontCreateNativeAncestors
            // attribute is set.
            d->createWinId();
            // Nothing more to do.
            Q_ASSERT(testAttribute(Qt::WA_WState_Created));
            Q_ASSERT(internalWinId());
            return;
        }
    }


    static const bool paintOnScreenEnv = qEnvironmentVariableIntValue("QT_ONSCREEN_PAINT") > 0;
    if (paintOnScreenEnv)
        setAttribute(Qt::WA_PaintOnScreen);

    if (QApplicationPrivate::testAttribute(Qt::AA_NativeWindows))
        setAttribute(Qt::WA_NativeWindow);


    d->updateIsOpaque();

    setAttribute(Qt::WA_WState_Created);                        // set created flag
    d->create_sys(window, initializeWindow, destroyOldWindow);

    // a real toplevel window needs a backing store
    if (isWindow() && windowType() != Qt::Desktop) {
        d->topData()->backingStoreTracker.destroy();
        d->topData()->backingStoreTracker.create(this);
    }

    d->setModal_sys();

    if (!isWindow() && parentWidget() && parentWidget()->testAttribute(Qt::WA_DropSiteRegistered))
        setAttribute(Qt::WA_DropSiteRegistered, true);

#ifdef QT_EVAL
    extern void qt_eval_init_widget(QWidget *w);
    qt_eval_init_widget(this);
#endif

    // need to force the resting of the icon after changing parents
    if (testAttribute(Qt::WA_SetWindowIcon))
        d->setWindowIcon_sys();

    if (isWindow() && !d->topData()->iconText.isEmpty())
        d->setWindowIconText_helper(d->topData()->iconText);
    if (isWindow() && !d->topData()->caption.isEmpty())
        d->setWindowTitle_helper(d->topData()->caption);
    if (isWindow() && !d->topData()->filePath.isEmpty())
        d->setWindowFilePath_helper(d->topData()->filePath);
    if (windowType() != Qt::Desktop) {
        d->updateSystemBackground();

        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon))
            d->setWindowIcon_sys();
    }

    // Frame strut update needed in cases where there are native widgets such as QGLWidget,
    // as those force native window creation on their ancestors before they are shown.
    // If the strut is not updated, any subsequent move of the top level window before show
    // will cause window frame to be ignored when positioning the window.
    // Note that this only helps on platforms that handle window creation synchronously.
    d->updateFrameStrut();
}

void q_createNativeChildrenAndSetParent(const QWidget *parentWidget)
{
    QObjectList children = parentWidget->children();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isWidgetType()) {
            const QWidget *childWidget = qobject_cast<const QWidget *>(children.at(i));
            if (childWidget) { // should not be necessary
                if (childWidget->testAttribute(Qt::WA_NativeWindow)) {
                    if (!childWidget->internalWinId())
                        childWidget->winId();
                    if (childWidget->windowHandle()) {
                        if (childWidget->isWindow()) {
                            childWidget->windowHandle()->setTransientParent(parentWidget->window()->windowHandle());
                        } else {
                            childWidget->windowHandle()->setParent(childWidget->nativeParentWidget()->windowHandle());
                        }
                    }
                } else {
                    q_createNativeChildrenAndSetParent(childWidget);
                }
            }
        }
    }

}

void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    QWidget* const q = q_func();

    Q_UNUSED(window);
    Q_UNUSED(initializeWindow);
    Q_UNUSED(destroyOldWindow);

    if (!q->testAttribute(Qt::WA_NativeWindow) && !q->isWindow())
        return; // we only care about real toplevels

    QWindow *win = topData()->window;
    // topData() ensures the extra is created but does not ensure 'window' is non-null
    // in case the extra was already valid.
    if (!win) {
        createTLSysExtra();
        win = topData()->window;
    }

    const auto dynamicPropertyNames = q->dynamicPropertyNames();
    for (const QByteArray &propertyName : dynamicPropertyNames) {
        if (!qstrncmp(propertyName, "_q_platform_", 12))
            win->setProperty(propertyName, q->property(propertyName));
    }

    Qt::WindowFlags &flags = data.window_flags;

    if (q->testAttribute(Qt::WA_ShowWithoutActivating))
        win->setProperty("_q_showWithoutActivating", QVariant(true));
    win->setFlags(flags);
    fixPosIncludesFrame();
    if (q->testAttribute(Qt::WA_Moved)
        || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
        win->setGeometry(q->geometry());
    else
        win->resize(q->size());
    if (win->isTopLevel()) {
        int screenNumber = topData()->initialScreenIndex;
        topData()->initialScreenIndex = -1;
        if (screenNumber < 0) {
            screenNumber = q->windowType() != Qt::Desktop
                ? QApplication::desktop()->screenNumber(q) : 0;
        }
        win->setScreen(QGuiApplication::screens().value(screenNumber, Q_NULLPTR));
    }

    QSurfaceFormat format = win->requestedFormat();
    if ((flags & Qt::Window) && win->surfaceType() != QSurface::OpenGLSurface
            && q->testAttribute(Qt::WA_TranslucentBackground)) {
        format.setAlphaBufferSize(8);
    }
    win->setFormat(format);

    if (QWidget *nativeParent = q->nativeParentWidget()) {
        if (nativeParent->windowHandle()) {
            if (flags & Qt::Window) {
                win->setTransientParent(nativeParent->window()->windowHandle());
                win->setParent(0);
            } else {
                win->setTransientParent(0);
                win->setParent(nativeParent->windowHandle());
            }
        }
    }

    qt_window_private(win)->positionPolicy = topData()->posIncludesFrame ?
        QWindowPrivate::WindowFrameInclusive : QWindowPrivate::WindowFrameExclusive;

    if (q->windowType() != Qt::Desktop || q->testAttribute(Qt::WA_NativeWindow)) {
        win->create();
        // Enable nonclient-area events for QDockWidget and other NonClientArea-mouse event processing.
        if (QPlatformWindow *platformWindow = win->handle())
            platformWindow->setFrameStrutEventsEnabled(true);
    }

    data.window_flags = win->flags();
    if (!win->isTopLevel()) // In a Widget world foreign windows can only be top level
      data.window_flags &= ~Qt::ForeignWindow;

    if (!topData()->role.isNull())
        QXcbWindowFunctions::setWmWindowRole(win, topData()->role.toLatin1());

    QBackingStore *store = q->backingStore();

    if (!store) {
        if (win && q->windowType() != Qt::Desktop) {
            if (q->isTopLevel())
                q->setBackingStore(new QBackingStore(win));
        } else {
            q->setAttribute(Qt::WA_PaintOnScreen, true);
        }
    }

    setWindowModified_helper();

    if (win->handle()) {
        WId id = win->winId();
        // See the QPlatformWindow::winId() documentation
        Q_ASSERT(id != WId(0));
        setWinId(id);
    }

    // Check children and create windows for them if necessary
    q_createNativeChildrenAndSetParent(q);

    if (extra && !extra->mask.isEmpty())
        setMask_sys(extra->mask);

    if (data.crect.width() == 0 || data.crect.height() == 0) {
        q->setAttribute(Qt::WA_OutsideWSRange, true);
    } else if (q->isVisible()) {
        // If widget is already shown, set window visible, too
        win->setVisible(true);
    }
}

#ifdef Q_OS_WIN
static const char activeXNativeParentHandleProperty[] = "_q_embedded_native_parent_handle";
#endif

void QWidgetPrivate::createTLSysExtra()
{
    QWidget* const q = q_func();
    if (!extra->topextra->window && (q->testAttribute(Qt::WA_NativeWindow) || q->isWindow())) {
        extra->topextra->window = new QWidgetWindow(q);
        if (extra->minw || extra->minh)
            extra->topextra->window->setMinimumSize(QSize(extra->minw, extra->minh));
        if (extra->maxw != QWIDGETSIZE_MAX || extra->maxh != QWIDGETSIZE_MAX)
            extra->topextra->window->setMaximumSize(QSize(extra->maxw, extra->maxh));
        if (extra->topextra->opacity != 255 && q->isWindow())
            extra->topextra->window->setOpacity(qreal(extra->topextra->opacity) / qreal(255));
#ifdef Q_OS_WIN
        // Pass on native parent handle for Widget embedded into Active X.
        const QVariant activeXNativeParentHandle = q->property(activeXNativeParentHandleProperty);
        if (activeXNativeParentHandle.isValid())
            extra->topextra->window->setProperty(activeXNativeParentHandleProperty, activeXNativeParentHandle);
        if (q->inherits("QTipLabel") || q->inherits("QAlphaWidget"))
            extra->topextra->window->setProperty("_q_windowsDropShadow", QVariant(true));
#endif
    }

}


QWidget::~QWidget()
{
    QWidgetPrivate * const d = d_func();
    d->data.in_destructor = true;

#if defined (QT_CHECK_STATE)
    if (Q_UNLIKELY(paintingActive()))
        qWarning("QWidget: %s (%s) deleted while being painted", className(), name());
#endif

#ifndef QT_NO_GESTURES
    if (QGestureManager *manager = QGestureManager::instance()) {
        // \forall Qt::GestureType type : ungrabGesture(type) (inlined)
        for (auto it = d->gestureContext.keyBegin(), end = d->gestureContext.keyEnd(); it != end; ++it)
            manager->cleanupCachedGestures(this, *it);
    }
    d->gestureContext.clear();
#endif

    // force acceptDrops false before winId is destroyed.
    d->registerDropSite(false);

#ifndef QT_NO_ACTION
    // remove all actions from this widget
    for (int i = 0; i < d->actions.size(); ++i) {
        QActionPrivate *apriv = d->actions.at(i)->d_func();
        apriv->widgets.removeAll(this);
    }
    d->actions.clear();
#endif

#ifndef QT_NO_SHORTCUT
    // Remove all shortcuts grabbed by this
    // widget, unless application is closing
    if (!QApplicationPrivate::is_app_closing && testAttribute(Qt::WA_GrabbedShortcut))
        qApp->d_func()->shortcutMap.removeShortcut(0, this, QKeySequence());
#endif

    // delete layout while we still are a valid widget
    delete d->layout;
    d->layout = 0;
    // Remove myself from focus list

    Q_ASSERT(d->focus_next->d_func()->focus_prev == this);
    Q_ASSERT(d->focus_prev->d_func()->focus_next == this);

    if (d->focus_next != this) {
        d->focus_next->d_func()->focus_prev = d->focus_prev;
        d->focus_prev->d_func()->focus_next = d->focus_next;
        d->focus_next = d->focus_prev = 0;
    }


    QT_TRY {
#if QT_CONFIG(graphicsview)
        const QWidget* w = this;
        while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
            w = w->d_func()->extra->focus_proxy;
        QWidget *window = w->window();
        QWExtra *e = window ? window->d_func()->extra : 0;
        if (!e || !e->proxyWidget || (w->parentWidget() && w->parentWidget()->d_func()->focus_child == this))
#endif
        clearFocus();
    } QT_CATCH(...) {
        // swallow this problem because we are in a destructor
    }

    d->setDirtyOpaqueRegion();

    if (isWindow() && isVisible() && internalWinId()) {
        QT_TRY {
            d->close_helper(QWidgetPrivate::CloseNoEvent);
        } QT_CATCH(...) {
            // if we're out of memory, at least hide the window.
            QT_TRY {
                hide();
            } QT_CATCH(...) {
                // and if that also doesn't work, then give up
            }
        }
    }

    else if (isVisible()) {
        qApp->d_func()->sendSyntheticEnterLeave(this);
    }

    if (QWidgetBackingStore *bs = d->maybeBackingStore()) {
        bs->removeDirtyWidget(this);
        if (testAttribute(Qt::WA_StaticContents))
            bs->removeStaticWidget(this);
    }

    delete d->needsFlush;
    d->needsFlush = 0;

    // The next 20 lines are duplicated from QObject, but required here
    // since QWidget deletes is children itself
    bool blocked = d->blockSig;
    d->blockSig = 0; // unblock signals so we always emit destroyed()

    if (d->isSignalConnected(0)) {
        QT_TRY {
            emit destroyed(this);
        } QT_CATCH(...) {
            // all the signal/slots connections are still in place - if we don't
            // quit now, we will crash pretty soon.
            qWarning("Detected an unexpected exception in ~QWidget while emitting destroyed().");
            QT_RETHROW;
        }
    }

    if (d->declarativeData) {
        if (static_cast<QAbstractDeclarativeDataImpl*>(d->declarativeData)->ownedByQml1) {
            if (QAbstractDeclarativeData::destroyed_qml1)
                QAbstractDeclarativeData::destroyed_qml1(d->declarativeData, this);
        } else {
            if (QAbstractDeclarativeData::destroyed)
                QAbstractDeclarativeData::destroyed(d->declarativeData, this);
        }
        d->declarativeData = 0;                 // don't activate again in ~QObject
    }

    d->blockSig = blocked;

    if (!d->children.isEmpty())
        d->deleteChildren();

    QApplication::removePostedEvents(this);

    QT_TRY {
        destroy();                                        // platform-dependent cleanup
    } QT_CATCH(...) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }
    --QWidgetPrivate::instanceCounter;

    if (QWidgetPrivate::allWidgets) // might have been deleted by ~QApplication
        QWidgetPrivate::allWidgets->remove(this);

    QT_TRY {
        QEvent e(QEvent::Destroy);
        QCoreApplication::sendEvent(this, &e);
    } QT_CATCH(const std::exception&) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }

#if QT_CONFIG(graphicseffect)
    delete d->graphicsEffect;
#endif
}

int QWidgetPrivate::instanceCounter = 0;  // Current number of widget instances
int QWidgetPrivate::maxInstances = 0;     // Maximum number of widget instances

void QWidgetPrivate::setWinId(WId id)                // set widget identifier
{
    QWidget* const q = q_func();
    // the user might create a widget with Qt::Desktop window
    // attribute (or create another QDesktopWidget instance), which
    // will have the same windowid (the root window id) as the
    // qt_desktopWidget. We should not add the second desktop widget
    // to the mapper.
    bool userDesktopWidget = qt_desktopWidget != 0 && qt_desktopWidget != q && q->windowType() == Qt::Desktop;
    if (mapper && data.winid && !userDesktopWidget) {
        mapper->remove(data.winid);
    }

    const WId oldWinId = data.winid;

    data.winid = id;
#if 0 // Used to be included in Qt4 for Q_WS_X11
    hd = id; // X11: hd == ident
#endif
    if (mapper && id && !userDesktopWidget) {
        mapper->insert(data.winid, q);
    }

    if(oldWinId != id) {
        QEvent e(QEvent::WinIdChange);
        QCoreApplication::sendEvent(q, &e);
    }
}

void QWidgetPrivate::createTLExtra()
{
    if (!extra)
        createExtra();
    if (!extra->topextra) {
        QTLWExtra* x = extra->topextra = new QTLWExtra;
        x->icon = 0;
        x->backingStore = 0;
        x->sharedPainter = 0;
        x->incw = x->inch = 0;
        x->basew = x->baseh = 0;
        x->frameStrut.setCoords(0, 0, 0, 0);
        x->normalGeometry = QRect(0,0,-1,-1);
        x->savedFlags = 0;
        x->opacity = 255;
        x->posIncludesFrame = 0;
        x->sizeAdjusted = false;
        x->inTopLevelResize = false;
        x->inRepaint = false;
        x->embedded = 0;
        x->window = 0;
        x->shareContext = 0;
        x->initialScreenIndex = -1;
#if 0 // Used to be included in Qt4 for Q_WS_MAC
        x->wasMaximized = false;
#endif
#ifdef QWIDGET_EXTRA_DEBUG
        static int count = 0;
        qDebug() << "tlextra" << ++count;
#endif
    }
}



void QWidgetPrivate::createExtra()
{
    if (!extra) {                                // if not exists
        extra = new QWExtra;
        extra->glContext = 0;
        extra->topextra = 0;
        extra->proxyWidget = 0;
        extra->curs = 0;
        extra->minw = 0;
        extra->minh = 0;
        extra->maxw = QWIDGETSIZE_MAX;
        extra->maxh = QWIDGETSIZE_MAX;
        extra->customDpiX = 0;
        extra->customDpiY = 0;
        extra->explicitMinSize = 0;
        extra->explicitMaxSize = 0;
        extra->autoFillBackground = 0;
        extra->nativeChildrenForced = 0;
        extra->inRenderWithPainter = 0;
        extra->hasWindowContainer = false;
        extra->hasMask = 0;
        createSysExtra();
        static int count = 0;
        qDebug() << "extra" << ++count;
    }
}

void QWidgetPrivate::createSysExtra()
{
}

/*!
  \internal
  Deletes the widget extra data.
*/

void QWidgetPrivate::deleteExtra()
{
    if (extra) {                       
        delete extra->curs;
        deleteSysExtra();
        // dereference the stylesheet style
        if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(extra->style))
            proxy->deref();
        if (extra->topextra) {
            deleteTLSysExtra();
            // extra->topextra->backingStore destroyed in QWidgetPrivate::deleteTLSysExtra()
            delete extra->topextra->icon;
            delete extra->topextra;
        }
        delete extra;
        // extra->xic destroyed in QWidget::destroy()
        extra = 0;
    }
}

void QWidgetPrivate::deleteSysExtra()
{
}

static void deleteBackingStore(QWidgetPrivate *d)
{
    QTLWExtra *topData = d->topData();

    // The context must be current when destroying the backing store as it may attempt to
    // release resources like textures and shader programs. The window may not be suitable
    // anymore as there will often not be a platform window underneath at this stage. Fall
    // back to a QOffscreenSurface in this case.
    QScopedPointer<QOffscreenSurface> tempSurface;
#ifndef QT_NO_OPENGL
    if (d->textureChildSeen && topData->shareContext) {
        if (topData->window->handle()) {
            topData->shareContext->makeCurrent(topData->window);
        } else {
            tempSurface.reset(new QOffscreenSurface);
            tempSurface->setFormat(topData->shareContext->format());
            tempSurface->create();
            topData->shareContext->makeCurrent(tempSurface.data());
        }
    }
#endif

    delete topData->backingStore;
    topData->backingStore = 0;

#ifndef QT_NO_OPENGL
    if (d->textureChildSeen && topData->shareContext)
        topData->shareContext->doneCurrent();
#endif
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra && extra->topextra) {
        //the qplatformbackingstore may hold a reference to the window, so the backingstore
        //needs to be deleted first. If the backingstore holds GL resources, we need to
        // make the context current here. This is taken care of by deleteBackingStore().

        extra->topextra->backingStoreTracker.destroy();
        deleteBackingStore(this);
#ifndef QT_NO_OPENGL
        qDeleteAll(extra->topextra->widgetTextures);
        extra->topextra->widgetTextures.clear();
        delete extra->topextra->shareContext;
        extra->topextra->shareContext = 0;
#endif

        //the toplevel might have a context with a "qglcontext associated with it. We need to
        //delete the qglcontext before we delete the qplatformopenglcontext.
        //One unfortunate thing about this is that we potentially create a glContext just to
        //delete it straight afterwards.
        if (extra->topextra->window) {
            extra->topextra->window->destroy();
        }
        delete extra->topextra->window;
        extra->topextra->window = 0;

    }
}

/*
  Returns  true if there are widgets above this which overlap with
   rect, which is in parent's coordinate system (same as crect).
*/

bool QWidgetPrivate::isOverlapped(const QRect &rect) const
{
    QWidget* const q = q_func();

    const QWidget *w = q;
    QRect r = rect;
    while (w) {
        if (w->isWindow())
            return false;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        bool above = false;
        for (int i = 0; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;
            if (!above) {
                above = (sibling == w);
                continue;
            }

            if (qRectIntersects(sibling->d_func()->effectiveRectFor(sibling->data->crect), r)) {
                const QWExtra *siblingExtra = sibling->d_func()->extra;
                if (siblingExtra && siblingExtra->hasMask && !sibling->d_func()->graphicsEffect
                    && !siblingExtra->mask.translated(sibling->data->crect.topLeft()).intersects(r)) {
                    continue;
                }
                return true;
            }
        }
        w = w->parentWidget();
        r.translate(pd->data.crect.topLeft());
    }
    return false;
}

void QWidgetPrivate::syncBackingStore()
{
	// 直接绘图就没有各种画面特效了,一般没有这个选型
    if (paintOnScreen()) {
        repaint_sys(dirty);
        dirty = QRegion();
    } 
	else if (QWidgetBackingStore *bs = maybeBackingStore()) {
        bs->sync();
    }
}

// 后备存储
void QWidgetPrivate::syncBackingStore(const QRegion &region)
{
	// 直接绘图就没有各种画面特效了,一般没有这个选型
    if (paintOnScreen())
        repaint_sys(region);
    else if (QWidgetBackingStore *bs = maybeBackingStore()) {
        bs->sync(q_func(), region);
    }
}

void QWidgetPrivate::setUpdatesEnabled_helper(bool enable)
{
    QWidget* const q = q_func();

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->updatesEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_UpdatesDisabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_UpdatesDisabled, !enable);
    if (enable)
        q->update();

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceUpdatesDisabled : Qt::WA_UpdatesDisabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow() && !w->testAttribute(attribute))
            w->d_func()->setUpdatesEnabled_helper(enable);
    }
}

/*!
    Propagate [this widget's palette] to all children, 

    except style sheet widgets, 

    and windows that don't enable window propagation 

    (palettes don't normally propagate to windows).
*/
void QWidgetPrivate::propagatePaletteChange()
{
    QWidget* const q = q_func();
	
    // Propagate a new inherited mask to all children.
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedPaletteResolveMask = p->d_func()->inheritedPaletteResolveMask | p->palette().resolve();
    } else
        if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedPaletteResolveMask = 0;
    }
		
    int mask = data.pal.resolve() | inheritedPaletteResolveMask;

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QEvent pc(QEvent::PaletteChange);
	// sendEvent 是 QCoreApplication里面的static, 这里默认给继承下来了
    QApplication::sendEvent(q, &pc);  //  until q handled

	// children是QObjectData里面专门设计的
	// QWidgetPrivate <-  QObjectPrivate <- QObjectData
	// Qt的Private系列中,基本上都是这样的继承体系
	// Qt的明暗2条线
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w && (!w->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
            && (!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
            QWidgetPrivate *wd = w->d_func();
            wd->inheritedPaletteResolveMask = mask;
            wd->resolvePalette();
        }
    }
}

/*
  Returns the widget's clipping rectangle.
*/
QRect QWidgetPrivate::clipRect() const
{
    QWidget* const q = q_func();
    const QWidget * w = q;
    if (!w->isVisible())
        return QRect();
    QRect r = effectiveRectFor(q->rect());
    int ox = 0;
    int oy = 0;
    while (w
            && w->isVisible()
            && !w->isWindow()
            && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        w = w->parentWidget();
        r &= QRect(ox, oy, w->width(), w->height());
    }
    return r;
}

/*
  Returns the widget's clipping region (without siblings).
*/
QRegion QWidgetPrivate::clipRegion() const
{
    QWidget* const q = q_func();
    if (!q->isVisible())
        return QRegion();
    QRegion r(q->rect());
    const QWidget * w = q;
    const QWidget *ignoreUpTo;
    int ox = 0;
    int oy = 0;
    while (w
           && w->isVisible()
           && !w->isWindow()
           && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        ignoreUpTo = w;
        w = w->parentWidget();
        r &= QRegion(ox, oy, w->width(), w->height());

        int i = 0;
        while(w->d_func()->children.at(i++) != static_cast<const QObject *>(ignoreUpTo))
            ;
        for ( ; i < w->d_func()->children.size(); ++i) {
            if(QWidget *sibling = qobject_cast<QWidget *>(w->d_func()->children.at(i))) {
                if(sibling->isVisible() && !sibling->isWindow()) {
                    QRect siblingRect(ox+sibling->x(), oy+sibling->y(),
                                      sibling->width(), sibling->height());
                    if (qRectIntersects(siblingRect, q->rect()))
                        r -= QRegion(siblingRect);
                }
            }
        }
    }
    return r;
}

void QWidgetPrivate::setSystemClip(QPaintDevice *paintDevice, const QRegion &region)
{
// Transform the system clip region from device-independent pixels to device pixels
    QPaintEngine *paintEngine = paintDevice->paintEngine();
    QTransform scaleTransform;
    const qreal devicePixelRatio = paintDevice->devicePixelRatioF();
    scaleTransform.scale(devicePixelRatio, devicePixelRatio);
    paintEngine->d_func()->systemClip = scaleTransform.map(region);
}

#if QT_CONFIG(graphicseffect)
void QWidgetPrivate::invalidateGraphicsEffectsRecursively()
{
    QWidget* const q = q_func();
    QWidget *w = q;
    do {
        if (w->graphicsEffect()) {
            QWidgetEffectSourcePrivate *sourced =
                static_cast<QWidgetEffectSourcePrivate *>(w->graphicsEffect()->source()->d_func());
            if (!sourced->updateDueToGraphicsEffect)
                w->graphicsEffect()->source()->d_func()->invalidateCache();
        }
        w = w->parentWidget();
    } while (w);
}
#endif // QT_CONFIG(graphicseffect)

void QWidgetPrivate::setDirtyOpaqueRegion()
{
    QWidget* const q = q_func();

    dirtyOpaqueChildren = true;

#if QT_CONFIG(graphicseffect)
    invalidateGraphicsEffectsRecursively();
#endif // QT_CONFIG(graphicseffect)

    if (q->isWindow())
        return;

    QWidget *parent = q->parentWidget();
    if (!parent)
        return;

    // TODO: instead of setting dirtyflag, manipulate the dirtyregion directly?
    QWidgetPrivate *pd = parent->d_func();
    if (!pd->dirtyOpaqueChildren)
        pd->setDirtyOpaqueRegion();
}

const QRegion &QWidgetPrivate::getOpaqueChildren() const
{
    if (!dirtyOpaqueChildren)
        return opaqueChildren;

    QWidgetPrivate *that = const_cast<QWidgetPrivate*>(this);
    that->opaqueChildren = QRegion();

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || !child->isVisible() || child->isWindow())
            continue;

        const QPoint offset = child->geometry().topLeft();
        QWidgetPrivate *childd = child->d_func();
        QRegion r = childd->isOpaque ? child->rect() : childd->getOpaqueChildren();
        if (childd->extra && childd->extra->hasMask)
            r &= childd->extra->mask;
        if (r.isEmpty())
            continue;
        r.translate(offset);
        that->opaqueChildren += r;
    }

    that->opaqueChildren &= q_func()->rect();
    that->dirtyOpaqueChildren = false;

    return that->opaqueChildren;
}

void QWidgetPrivate::subtractOpaqueChildren(QRegion &source, const QRect &clipRect) const
{
    if (children.isEmpty() || clipRect.isEmpty())
        return;

    const QRegion &r = getOpaqueChildren();
    if (!r.isEmpty())
        source -= (r & clipRect);
}

//subtract any relatives that are higher up than me --- this is too expensive !!!
void QWidgetPrivate::subtractOpaqueSiblings(QRegion &sourceRegion, bool *hasDirtySiblingsAbove,
                                            bool alsoNonOpaque) const
{
    QWidget* const q = q_func();
    static int disableSubtractOpaqueSiblings = qEnvironmentVariableIntValue("QT_NO_SUBTRACTOPAQUESIBLINGS");
    if (disableSubtractOpaqueSiblings || q->isWindow())
        return;

#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (q->d_func()->isInUnifiedToolbar)
        return;
#endif

    QRect clipBoundingRect;
    bool dirtyClipBoundingRect = true;

    QRegion parentClip;
    bool dirtyParentClip = true;

    QPoint parentOffset = data.crect.topLeft();

    const QWidget *w = q;

    while (w) {
        if (w->isWindow())
            break;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        const int myIndex = pd->children.indexOf(const_cast<QWidget *>(w));
        const QRect widgetGeometry = w->d_func()->effectiveRectFor(w->data->crect);
        for (int i = myIndex + 1; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;

            const QRect siblingGeometry = sibling->d_func()->effectiveRectFor(sibling->data->crect);
            if (!qRectIntersects(siblingGeometry, widgetGeometry))
                continue;

            if (dirtyClipBoundingRect) {
                clipBoundingRect = sourceRegion.boundingRect();
                dirtyClipBoundingRect = false;
            }

            if (!qRectIntersects(siblingGeometry, clipBoundingRect.translated(parentOffset)))
                continue;

            if (dirtyParentClip) {
                parentClip = sourceRegion.translated(parentOffset);
                dirtyParentClip = false;
            }

            const QPoint siblingPos(sibling->data->crect.topLeft());
            const QRect siblingClipRect(sibling->d_func()->clipRect());
            QRegion siblingDirty(parentClip);
            siblingDirty &= (siblingClipRect.translated(siblingPos));
            const bool hasMask = sibling->d_func()->extra && sibling->d_func()->extra->hasMask
                                 && !sibling->d_func()->graphicsEffect;
            if (hasMask)
                siblingDirty &= sibling->d_func()->extra->mask.translated(siblingPos);
            if (siblingDirty.isEmpty())
                continue;

            if (sibling->d_func()->isOpaque || alsoNonOpaque) {
                if (hasMask) {
                    siblingDirty.translate(-parentOffset);
                    sourceRegion -= siblingDirty;
                } else {
                    sourceRegion -= siblingGeometry.translated(-parentOffset);
                }
            } else {
                if (hasDirtySiblingsAbove)
                    *hasDirtySiblingsAbove = true;
                if (sibling->d_func()->children.isEmpty())
                    continue;
                QRegion opaqueSiblingChildren(sibling->d_func()->getOpaqueChildren());
                opaqueSiblingChildren.translate(-parentOffset + siblingPos);
                sourceRegion -= opaqueSiblingChildren;
            }
            if (sourceRegion.isEmpty())
                return;

            dirtyClipBoundingRect = true;
            dirtyParentClip = true;
        }

        w = w->parentWidget();
        parentOffset += pd->data.crect.topLeft();
        dirtyParentClip = true;
    }
}

void QWidgetPrivate::clipToEffectiveMask(QRegion &region) const
{
    QWidget* const q = q_func();

    const QWidget *w = q;
    QPoint offset;

#if QT_CONFIG(graphicseffect)
    if (graphicsEffect) {
        w = q->parentWidget();
        offset -= data.crect.topLeft();
    }
#endif // QT_CONFIG(graphicseffect)

    while (w) {
        const QWidgetPrivate *wd = w->d_func();
        if (wd->extra && wd->extra->hasMask)
            region &= (w != q) ? wd->extra->mask.translated(offset) : wd->extra->mask;
        if (w->isWindow())
            return;
        offset -= wd->data.crect.topLeft();
        w = w->parentWidget();
    }
}

bool QWidgetPrivate::paintOnScreen() const
{
    QWidget* const q = q_func();
    if (q->testAttribute(Qt::WA_PaintOnScreen)
            || (!q->isWindow() && q->window()->testAttribute(Qt::WA_PaintOnScreen))) {
        return true;
    }

    return false;
}

void QWidgetPrivate::updateIsOpaque()
{
    // hw: todo: only needed if opacity actually changed
    setDirtyOpaqueRegion();

#if QT_CONFIG(graphicseffect)
    if (graphicsEffect) {
        // ### We should probably add QGraphicsEffect::isOpaque at some point.
        setOpaque(false);
        return;
    }
#endif // QT_CONFIG(graphicseffect)

    QWidget* const q = q_func();
#if 0 // Used to be included in Qt4 for Q_WS_X11
    if (q->testAttribute(Qt::WA_X11OpenGLOverlay)) {
        setOpaque(false);
        return;
    }
#endif

    if (q->testAttribute(Qt::WA_OpaquePaintEvent) || q->testAttribute(Qt::WA_PaintOnScreen)) {
        setOpaque(true);
        return;
    }

    const QPalette &pal = q->palette();

    if (q->autoFillBackground()) {
        const QBrush &autoFillBrush = pal.brush(q->backgroundRole());
        if (autoFillBrush.style() != Qt::NoBrush && autoFillBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
    }

    if (q->isWindow() && !q->testAttribute(Qt::WA_NoSystemBackground)) {
        const QBrush &windowBrush = q->palette().brush(QPalette::Window);
        if (windowBrush.style() != Qt::NoBrush && windowBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
    }
    setOpaque(false);
}

void QWidgetPrivate::setOpaque(bool opaque)
{
    if (isOpaque != opaque) {
        isOpaque = opaque;
        updateIsTranslucent();
    }
}

void QWidgetPrivate::updateIsTranslucent()
{
    QWidget* const q = q_func();
    if (QWindow *window = q->windowHandle()) {
        QSurfaceFormat format = window->format();
        const int oldAlpha = format.alphaBufferSize();
        const int newAlpha = q->testAttribute(Qt::WA_TranslucentBackground)? 8 : 0;
        if (oldAlpha != newAlpha) {
            format.setAlphaBufferSize(newAlpha);
            window->setFormat(format);
        }
    }
}

static inline void fillRegion(QPainter *painter, const QRegion &rgn, const QBrush &brush)
{
    Q_ASSERT(painter);

    if (brush.style() == Qt::TexturePattern) {
#if 0 // Used to be included in Qt4 for Q_WS_MAC
        // Optimize pattern filling on mac by using HITheme directly
        // when filling with the standard widget background.
        // Defined in qmacstyle_mac.cpp
        extern void qt_mac_fill_background(QPainter *painter, const QRegion &rgn, const QBrush &brush);
        qt_mac_fill_background(painter, rgn, brush);
#else
        {
            const QRect rect(rgn.boundingRect());
            painter->setClipRegion(rgn);
            painter->drawTiledPixmap(rect, brush.texture(), rect.topLeft());
        }
#endif

    } else if (brush.gradient()
               && brush.gradient()->coordinateMode() == QGradient::ObjectBoundingMode) {
        painter->save();
        painter->setClipRegion(rgn);
        painter->fillRect(0, 0, painter->device()->width(), painter->device()->height(), brush);
        painter->restore();
    } else {
        for (const QRect &rect : rgn)
            painter->fillRect(rect, brush);
    }
}

void QWidgetPrivate::paintBackground(QPainter *painter, const QRegion &rgn, int flags) const
{
    QWidget* const q = q_func();

#if QT_CONFIG(scrollarea)
    bool resetBrushOrigin = false;
    QPointF oldBrushOrigin;
    //If we are painting the viewport of a scrollarea, we must apply an offset to the brush in case we are drawing a texture
    QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(parent);
    if (scrollArea && scrollArea->viewport() == q) {
        QObjectData *scrollPrivate = static_cast<QWidget *>(scrollArea)->d_ptr.data();
        QAbstractScrollAreaPrivate *priv = static_cast<QAbstractScrollAreaPrivate *>(scrollPrivate);
        oldBrushOrigin = painter->brushOrigin();
        resetBrushOrigin = true;
        painter->setBrushOrigin(-priv->contentsOffset());

    }
#endif // QT_CONFIG(scrollarea)

    const QBrush autoFillBrush = q->palette().brush(q->backgroundRole());

    if ((flags & DrawAsRoot) && !(q->autoFillBackground() && autoFillBrush.isOpaque())) {
        const QBrush bg = q->palette().brush(QPalette::Window);
        if (!(flags & DontSetCompositionMode)) {
            //copy alpha straight in
            QPainter::CompositionMode oldMode = painter->compositionMode();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            fillRegion(painter, rgn, bg);
            painter->setCompositionMode(oldMode);
        } else {
            fillRegion(painter, rgn, bg);
        }
    }

    if (q->autoFillBackground())
        fillRegion(painter, rgn, autoFillBrush);

    if (q->testAttribute(Qt::WA_StyledBackground)) {
        painter->setClipRegion(rgn);
        QStyleOption opt;
        opt.initFrom(q);
        q->style()->drawPrimitive(QStyle::PE_Widget, &opt, painter, q);
    }

#if QT_CONFIG(scrollarea)
    if (resetBrushOrigin)
        painter->setBrushOrigin(oldBrushOrigin);
#endif // QT_CONFIG(scrollarea)
}

/*
  \internal
  This function is called when a widget is hidden or destroyed.
  It resets some application global pointers that should only refer active,
  visible widgets.
*/
extern QWidget *qt_button_down;


void QWidgetPrivate::deactivateWidgetCleanup()
{
    QWidget* const q = q_func();
    // If this was the active application window, reset it
    if (QApplication::activeWindow() == q)
        QApplication::setActiveWindow(0);
    // If the is the active mouse press widget, reset it
    if (q == qt_button_down)
        qt_button_down = 0;
}


/*!
    Returns a pointer to the widget with window identifer/handle 
    id.

    The window identifier type depends on the underlying window
    system, see  qwindowdefs.h for the actual definition. If there
    is no widget with this identifier, 0 is returned.
*/

QWidget *QWidget::find(WId id)
{
    return QWidgetPrivate::mapper ? QWidgetPrivate::mapper->value(id, 0) : 0;
}



/*!
    \fn WId QWidget::internalWinId() const
    \internal
    Returns the window system identifier of the widget, or 0 if the widget is not created yet.

*/

/*!
    \fn WId QWidget::winId() const

    Returns the window system identifier of the widget.

    Portable in principle, but if you use it you are probably about to
    do something non-portable. Be careful.

    If a widget is non-native (alien) and winId() is invoked on it, that widget
    will be provided a native handle.

    This value may change at run-time. An event with type QEvent::WinIdChange
    will be sent to the widget following a change in window system identifier.

    \sa find()
*/
WId QWidget::winId() const
{
    if (!testAttribute(Qt::WA_WState_Created) || !internalWinId()) {
#ifdef ALIEN_DEBUG
        qDebug() << "QWidget::winId: creating native window for" << this;
#endif
        QWidget *that = const_cast<QWidget*>(this);
        that->setAttribute(Qt::WA_NativeWindow);
        that->d_func()->createWinId();
        return that->data->winid;
    }
    return data->winid;
}

void QWidgetPrivate::createWinId()
{
    QWidget* const q = q_func();

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::createWinId for" << q;
#endif
    const bool forceNativeWindow = q->testAttribute(Qt::WA_NativeWindow);
    if (!q->testAttribute(Qt::WA_WState_Created) || (forceNativeWindow && !q->internalWinId())) {
        if (!q->isWindow()) {
            QWidget *parent = q->parentWidget();
            QWidgetPrivate *pd = parent->d_func();
            if (forceNativeWindow && !q->testAttribute(Qt::WA_DontCreateNativeAncestors))
                parent->setAttribute(Qt::WA_NativeWindow);
            if (!parent->internalWinId()) {
                pd->createWinId();
            }

            for (int i = 0; i < pd->children.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(pd->children.at(i));
                if (w && !w->isWindow() && (!w->testAttribute(Qt::WA_WState_Created)
                                            || (!w->internalWinId() && w->testAttribute(Qt::WA_NativeWindow)))) {
                    w->create();
                }
            }
        } else {
            q->create();
        }
    }
}


/*!
\internal
Ensures that the widget has a window system identifier, i.e. that it is known to the windowing system.

*/

void QWidget::createWinId()
{
    QWidgetPrivate * const d = d_func();
#ifdef ALIEN_DEBUG
    qDebug()  << "QWidget::createWinId" << this;
#endif
//    qWarning("QWidget::createWinId is obsolete, please fix your code.");
    d->createWinId();
}

/*!
    \since 4.4

    Returns the effective window system identifier of the widget, i.e. the
    native parent's window system identifier.

    If the widget is native, this function returns the native widget ID.
    Otherwise, the window ID of the first native parent widget, i.e., the
    top-level widget that contains this widget, is returned.

    \note We recommend that you do not store this value as it is likely to
    change at run-time.

    \sa nativeParentWidget()
*/
WId QWidget::effectiveWinId() const
{
    const WId id = internalWinId();
    if (id || !testAttribute(Qt::WA_WState_Created))
        return id;
    if (const QWidget *realParent = nativeParentWidget())
        return realParent->internalWinId();
    return 0;
}

/*!
    If this is a native widget, return the associated QWindow.
    Otherwise return null.

    Native widgets include toplevel widgets, QGLWidget, and child widgets
    on which winId() was called.

    \since 5.0

    \sa winId()
*/
QWindow *QWidget::windowHandle() const
{
    QWidgetPrivate * const d = d_func();
    QTLWExtra *extra = d->maybeTopData();
    if (extra)
        return extra->window;

    return 0;
}

#ifndef QT_NO_STYLE_STYLESHEET


QString QWidget::styleSheet() const
{
    QWidgetPrivate * const d = d_func();
    if (!d->extra)
        return QString();
    return d->extra->styleSheet;
}

void QWidget::setStyleSheet(const QString& styleSheet)
{
	QWidgetPrivate * const d = d_func();

    if (data->in_destructor)
        return;
    d->createExtra();

	// 目前保存的style,是否可以转型为QStyleSheetStyle
    QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(d->extra->style);
    d->extra->styleSheet = styleSheet;
    if (styleSheet.isEmpty()) { // stylesheet removed
        if (!proxy)
            return;
		// 取消已经设置的style,回复默认
        d->inheritStyle();
        return;
    }

    if (proxy) { // style sheet update
        if (d->polished)
            proxy->repolish(this);
        return;
    }
	// oye stylesheetStyle的构造,是否需要含有原始style的一些特征
    if (testAttribute(Qt::WA_SetStyle)) {
		// 更新属性,并且要传播给孩子
        d->setStyle_helper(new QStyleSheetStyle(d->extra->style), true);
    } else {    
        d->setStyle_helper(new QStyleSheetStyle(0), true);
    }
}

#endif // QT_NO_STYLE_STYLESHEET

/*
	oye , PushButton 里面计算尺寸是碰到
			style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this)
			
	(this)PushButton 拿到style, 如果自己定制了,就用定制的, 否则用App的
*/
QStyle *QWidget::style() const
{
    QWidgetPrivate * const d = d_func();
    if (d->extra && d->extra->style)
        return d->extra->style;
    return QApplication::style();
}

/*!
    Sets the widget's GUI style to [style]. 
    The ownership of the style object is not transferred.

    If no style is set, the widget uses the application's style,
    QApplication::style() instead.

    Setting a widget's style has no effect on existing or future child
    widgets.

    \warning This function is particularly useful for demonstration
    purposes, where you want to show Qt's styling capabilities. Real
    applications should avoid it and use one consistent GUI style
    instead.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa style(), QStyle, QApplication::style(), QApplication::setStyle()
*/

void QWidget::setStyle(QStyle *style)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_SetStyle, style != 0);
    d->createExtra(); // oye  lazy-init

	
    if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(style)) {
        //if for some reason someone try to set a QStyleSheetStyle, ref it
        //(this may happen for exemple in QButtonDialogBox which propagates its style)
        proxy->ref();
        d->setStyle_helper(style, false);
    } else if (qobject_cast<QStyleSheetStyle *>(d->extra->style) || !qApp->styleSheet().isEmpty()) {
        // if we have an application stylesheet or have a proxy already, propagate
        d->setStyle_helper(new QStyleSheetStyle(style), true);
    } else
        d->setStyle_helper(style, false);
}

/*
	oye
	set style by send the event(QEvent::StyleChange) to current widget
*/
void QWidgetPrivate::setStyle_helper(QStyle *newStyle, bool propagate, bool)
{
	QWidget * const q = q_func();
    QStyle *oldStyle  = q->style();
    QPointer<QStyle> origStyle;

    {
        createExtra();
        origStyle = extra->style.data();
        extra->style = newStyle;
    }

    // repolish
    if (q->windowType() != Qt::Desktop) {
        if (polished) {
            oldStyle->unpolish(q);
        }
    }

    if (propagate) {
        // We copy the list because the order may be modified
        const QObjectList childrenList = children;
        for (int i = 0; i < childrenList.size(); ++i) {
            QWidget *c = qobject_cast<QWidget*>(childrenList.at(i));
            if (c)
                c->d_func()->inheritStyle();
        }
    }

    if (!qobject_cast<QStyleSheetStyle*>(newStyle)) {
        if (const QStyleSheetStyle* cssStyle = qobject_cast<QStyleSheetStyle*>(origStyle.data())) {
            cssStyle->clearWidgetFont(q);
        }
    }

    QEvent e(QEvent::StyleChange);
    QApplication::sendEvent(q, &e);

    // dereference the old stylesheet style
    if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(origStyle.data()))
        proxy->deref();
}

// Inherits style from the current parent and propagates it as necessary
void QWidgetPrivate::inheritStyle()
{
    //QWidget* const q = q_func();
	QWidget * const q = q_func();

    QStyleSheetStyle *proxy = extra ? qobject_cast<QStyleSheetStyle *>(extra->style) : 0;

    if (!q->styleSheet().isEmpty()) {
		
        proxy->repolish(q);
        return;
    }

    QStyle *origStyle = proxy ? proxy->base : (extra ? (QStyle*)extra->style : 0);
    QWidget *parent = q->parentWidget();
    QStyle *parentStyle = (parent && parent->d_func()->extra) ? (QStyle*)parent->d_func()->extra->style : 0;
    // If we have stylesheet on app or parent has stylesheet style, we need
    // to be running a proxy
    if (!qApp->styleSheet().isEmpty() || qobject_cast<QStyleSheetStyle *>(parentStyle)) {
        QStyle *newStyle = parentStyle;
        if (q->testAttribute(Qt::WA_SetStyle))
            newStyle = new QStyleSheetStyle(origStyle);
        else if (QStyleSheetStyle *newProxy = qobject_cast<QStyleSheetStyle *>(parentStyle))
            newProxy->ref();

        setStyle_helper(newStyle, true);
        return;
    }

    // So, we have no stylesheet on parent/app and we have an empty stylesheet
    // we just need our original style back
    if (origStyle == (extra ? (QStyle*)extra->style : 0)) // is it any different?
        return;

    // We could have inherited the proxy from our parent (which has a custom style)
    // In such a case we need to start following the application style (i.e revert
    // the propagation behavior of QStyleSheetStyle)
    if (!q->testAttribute(Qt::WA_SetStyle))
        origStyle = 0;

    setStyle_helper(origStyle, true);
}


/*!
    \fn bool QWidget::isWindow() const

    Returns  true if the widget is an independent window, otherwise
    returns  false.

    A window is a widget that isn't visually the child of any other
    widget and that usually has a frame and a
    {QWidget::setWindowTitle()}{window title}.

    A window can have a {QWidget::parentWidget()}{parent widget}.
    It will then be grouped with its parent and deleted when the
    parent is deleted, minimized when the parent is minimized etc. If
    supported by the window manager, it will also have a common
    taskbar entry with its parent.

    QDialog and QMainWindow widgets are by default windows, even if a
    parent widget is specified in the constructor. This behavior is
    specified by the Qt::Window flag.

    \sa window(), isModal(), parentWidget()
*/

/*!
    \property QWidget::modal
    \brief whether the widget is a modal widget

    This property only makes sense for windows. A modal widget
    prevents widgets in all other windows from getting any input.

    By default, this property is  false.

    \sa isWindow(), windowModality, QDialog
*/

/*!
    \property QWidget::windowModality
    \brief which windows are blocked by the modal widget
    \since 4.1

    This property only makes sense for windows. A modal widget
    prevents widgets in other windows from getting input. The value of
    this property controls which windows are blocked when the widget
    is visible. Changing this property while the window is visible has
    no effect; you must hide() the widget first, then show() it again.

    By default, this property is Qt::NonModal.

    \sa isWindow(), QWidget::modal, QDialog
*/

Qt::WindowModality QWidget::windowModality() const
{
    return static_cast<Qt::WindowModality>(data->window_modality);
}

void QWidget::setWindowModality(Qt::WindowModality windowModality)
{
    data->window_modality = windowModality;
    // setModal_sys() will be called by setAttribute()
    setAttribute(Qt::WA_ShowModal, (data->window_modality != Qt::NonModal));
    setAttribute(Qt::WA_SetWindowModality, true);
}

void QWidgetPrivate::setModal_sys()
{
    QWidget* const q = q_func();
    if (q->windowHandle())
        q->windowHandle()->setModality(q->windowModality());
}


bool QWidget::isMinimized() const
{ return data->window_state & Qt::WindowMinimized; }


void QWidget::showMinimized()
{
    bool isMin = isMinimized();
    if (isMin && isVisible())
        return;

    ensurePolished();

    if (!isMin)
        setWindowState((windowState() & ~Qt::WindowActive) | Qt::WindowMinimized);
    setVisible(true);
}

bool QWidget::isMaximized() const
{ return data->window_state & Qt::WindowMaximized; }

Qt::WindowStates QWidget::windowState() const
{
    return Qt::WindowStates(data->window_state);
}

/*!\internal

   The function sets the window state on child widgets similar to
   setWindowState(). The difference is that the window state changed
   event has the isOverride() flag set. It exists mainly to keep
   QWorkspace working.
 */
void QWidget::overrideWindowState(Qt::WindowStates newstate)
{
    QWindowStateChangeEvent e(Qt::WindowStates(data->window_state), true);
    data->window_state  = newstate;
    QApplication::sendEvent(this, &e);
}

Qt::WindowState effectiveState(Qt::WindowStates state)
{
    if (state & Qt::WindowMinimized)
        return Qt::WindowMinimized;
    else if (state & Qt::WindowFullScreen)
        return Qt::WindowFullScreen;
    else if (state & Qt::WindowMaximized)
        return Qt::WindowMaximized;
    return Qt::WindowNoState;
}

/*!
    \fn void QWidget::setWindowState(Qt::WindowStates windowState)

    Sets the window state to  windowState. The window state is a OR'ed
    combination of Qt::WindowState: Qt::WindowMinimized,
    Qt::WindowMaximized, Qt::WindowFullScreen, and Qt::WindowActive.

    If the window is not visible (i.e. isVisible() returns  false), the
    window state will take effect when show() is called. For visible
    windows, the change is immediate. For example, to toggle between
    full-screen and normal mode, use the following code:

    \snippet code/src_gui_kernel_qwidget.cpp 0

    In order to restore and activate a minimized window (while
    preserving its maximized and/or full-screen state), use the following:

    \snippet code/src_gui_kernel_qwidget.cpp 1

    Calling this function will hide the widget. You must call show() to make
    the widget visible again.

    \note On some window systems Qt::WindowActive is not immediate, and may be
    ignored in certain cases.

    When the window state changes, the widget receives a changeEvent()
    of type QEvent::WindowStateChange.

    \sa Qt::WindowState, windowState()
*/
void QWidget::setWindowState(Qt::WindowStates newstate)
{
    QWidgetPrivate * const d = d_func();
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;
    if (isWindow() && !testAttribute(Qt::WA_WState_Created))
        create();

    data->window_state = newstate;
    data->in_set_window_state = 1;
    Qt::WindowState newEffectiveState = effectiveState(newstate);
    Qt::WindowState oldEffectiveState = effectiveState(oldstate);
    if (isWindow() && newEffectiveState != oldEffectiveState) {
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        d->createTLExtra();
        if (oldEffectiveState == Qt::WindowNoState)
            d->topData()->normalGeometry = geometry();

        Q_ASSERT(windowHandle());
        windowHandle()->setWindowState(newEffectiveState);
    }
    data->in_set_window_state = 0;

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}


bool QWidget::isFullScreen() const
{ return data->window_state & Qt::WindowFullScreen; }

void QWidget::showFullScreen()
{
    ensurePolished();

    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowMaximized))
                   | Qt::WindowFullScreen);
    setVisible(true);
}

void QWidget::showMaximized()
{
    ensurePolished();
    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
    setVisible(true);
}

void QWidget::showNormal()
{
    ensurePolished();

    setWindowState(windowState() & ~(Qt::WindowMinimized
                                     | Qt::WindowMaximized
                                     | Qt::WindowFullScreen));

    setVisible(true);
}

/*!
    Returns  true if this widget would become enabled if  ancestor is
    enabled; otherwise returns  false.



    This is the case if neither the widget itself nor every parent up
    to but excluding  ancestor has been explicitly disabled.

    isEnabledTo(0) returns false if this widget or any if its ancestors
    was explicitly disabled.

    The word ancestor here means a parent widget within the same window.

    Therefore isEnabledTo(0) stops at this widget's window, unlike
    isEnabled() which also takes parent windows into considerations.

    \sa setEnabled(), enabled
*/

bool QWidget::isEnabledTo(const QWidget *ancestor) const
{
    const QWidget * w = this;
    while (!w->testAttribute(Qt::WA_ForceDisabled)
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->testAttribute(Qt::WA_ForceDisabled);
}

#ifndef QT_NO_ACTION
/*!
    Appends the action  action to this widget's list of actions.

    All QWidgets have a list of {QAction}s, however they can be
    represented graphically in many different ways. The default use of
    the QAction list (as returned by actions()) is to create a context
    QMenu.

    A QWidget should only have one of each action and adding an action
    it already has will not cause the same action to be in the widget twice.

    The ownership of  action is not transferred to this QWidget.

    \sa removeAction(), insertAction(), actions(), QMenu
*/
void QWidget::addAction(QAction *action)
{
    insertAction(0, action);
}

/*!
    Appends the actions  actions to this widget's list of actions.

    \sa removeAction(), QMenu, addAction()
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QWidget::addActions(const QList<QAction *> &actions)
#else
void QWidget::addActions(QList<QAction*> actions)
#endif
{
    for(int i = 0; i < actions.count(); i++)
        insertAction(0, actions.at(i));
}

/*!
    Inserts the action  action to this widget's list of actions,
    before the action  before. It appends the action if  before is 0 or
     before is not a valid action for this widget.

    A QWidget should only have one of each action.

    \sa removeAction(), addAction(), QMenu, contextMenuPolicy, actions()
*/
void QWidget::insertAction(QAction *before, QAction *action)
{
    if (Q_UNLIKELY(!action)) {
        qWarning("QWidget::insertAction: Attempt to insert null action");
        return;
    }

    QWidgetPrivate * const d = d_func();
    if(d->actions.contains(action))
        removeAction(action);

    int pos = d->actions.indexOf(before);
    if (pos < 0) {
        before = 0;
        pos = d->actions.size();
    }
    d->actions.insert(pos, action);

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.append(this);

    QActionEvent e(QEvent::ActionAdded, action, before);
    QApplication::sendEvent(this, &e);
}

/*!
    Inserts the actions  actions to this widget's list of actions,
    before the action  before. It appends the action if  before is 0 or
     before is not a valid action for this widget.

    A QWidget can have at most one of each action.

    \sa removeAction(), QMenu, insertAction(), contextMenuPolicy
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QWidget::insertActions(QAction *before, const QList<QAction*> &actions)
#else
void QWidget::insertActions(QAction *before, QList<QAction*> actions)
#endif
{
    for(int i = 0; i < actions.count(); ++i)
        insertAction(before, actions.at(i));
}

/*!
    Removes the action  action from this widget's list of actions.
    \sa insertAction(), actions(), insertAction()
*/
void QWidget::removeAction(QAction *action)
{
    if (!action)
        return;

    QWidgetPrivate * const d = d_func();

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.removeAll(this);

    if (d->actions.removeAll(action)) {
        QActionEvent e(QEvent::ActionRemoved, action);
        QApplication::sendEvent(this, &e);
    }
}

/*!
    Returns the (possibly empty) list of this widget's actions.

    \sa contextMenuPolicy, insertAction(), removeAction()
*/
QList<QAction*> QWidget::actions() const
{
    QWidgetPrivate * const d = d_func();
    return d->actions;
}
#endif // QT_NO_ACTION

void QWidget::setEnabled(bool enable)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_ForceDisabled, !enable);
    d->setEnabled_helper(enable);// QEvent e(QEvent::EnabledChange);
}

void QWidget::setDisabled(bool disable){    setEnabled(!disable);}

void QWidgetPrivate::setEnabled_helper(bool enable)
{
    QWidget* const q = q_func();

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->isEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_Disabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_Disabled, !enable);
    updateSystemBackground();

    if (!enable && q->window()->focusWidget() == q) {
        bool parentIsEnabled = (!q->parentWidget() || q->parentWidget()->isEnabled());
        if (!parentIsEnabled || !q->focusNextChild())
            q->clearFocus();
    }

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceDisabled : Qt::WA_Disabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->testAttribute(attribute))
            w->d_func()->setEnabled_helper(enable);
    }
#endif
#ifndef QT_NO_CURSOR
    if (q->testAttribute(Qt::WA_SetCursor) || q->isWindow()) {
        // enforce the windows behavior of clearing the cursor on
        // disabled widgets
        qt_qpa_set_cursor(q, false);
    }
#endif

#ifndef QT_NO_IM
    if (q->testAttribute(Qt::WA_InputMethodEnabled) && q->hasFocus()) {
        QWidget *focusWidget = effectiveFocusWidget();

        if (enable) {
            if (focusWidget->testAttribute(Qt::WA_InputMethodEnabled))
                QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        } else {
            QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
    }
#endif //QT_NO_IM
    QEvent e(QEvent::EnabledChange);
    QApplication::sendEvent(q, &e);
}

bool QWidget::acceptDrops() const{    return testAttribute(Qt::WA_AcceptDrops);}

void QWidget::setAcceptDrops(bool on){    setAttribute(Qt::WA_AcceptDrops, on);}

void QWidgetPrivate::registerDropSite(bool on){    Q_UNUSED(on);}

QRect QWidget::frameGeometry() const
{
    QWidgetPrivate * const d = d_func();
    if (isWindow() && ! (windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QRect(data->crect.x() - fs.left(),
                     data->crect.y() - fs.top(),
                     data->crect.width() + fs.left() + fs.right(),
                     data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect;
}

int QWidget::x() const
{
    QWidgetPrivate * const d = d_func();
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.x() - d->frameStrut().left();
    return data->crect.x();
}

int QWidget::y() const
{
    QWidgetPrivate * const d = d_func();
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.y() - d->frameStrut().top();
    return data->crect.y();
}

QPoint QWidget::pos() const
{
    QWidgetPrivate * const d = d_func();
    QPoint result = data->crect.topLeft();
    if (isWindow() && ! (windowType() == Qt::Popup))
        if (!d->maybeTopData() || !d->maybeTopData()->posIncludesFrame)
            result -= d->frameStrut().topLeft();
    return result;
}

QRect QWidget::normalGeometry() const
{
    QWidgetPrivate * const d = d_func();
    if (!d->extra || !d->extra->topextra)
        return QRect();

    if (!isMaximized() && !isFullScreen())
        return geometry();

    return d->topData()->normalGeometry;
}

QRect QWidget::childrenRect() const
{
    QWidgetPrivate * const d = d_func();
    QRect r(0, 0, 0, 0);
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden())
            r |= w->geometry();
    }
    return r;
}

QRegion QWidget::childrenRegion() const
{
    QWidgetPrivate * const d = d_func();
    QRegion r;
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden()) {
            QRegion mask = w->mask();
            if (mask.isEmpty())
                r |= w->geometry();
            else
                r |= mask.translated(w->pos());
        }
    }
    return r;
}

QSize QWidget::minimumSize() const
{
    QWidgetPrivate * const d = d_func();
    return d->extra ? QSize(d->extra->minw, d->extra->minh) : QSize(0, 0);
}

QSize QWidget::maximumSize() const
{
    QWidgetPrivate * const d = d_func();
    return d->extra ? QSize(d->extra->maxw, d->extra->maxh)
                 : QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

QSize QWidget::sizeIncrement() const
{
    QWidgetPrivate * const d = d_func();
    return (d->extra && d->extra->topextra)
        ? QSize(d->extra->topextra->incw, d->extra->topextra->inch)
        : QSize(0, 0);
}

QSize QWidget::baseSize() const
{
    QWidgetPrivate * const d = d_func();
    return (d->extra != 0 && d->extra->topextra != 0)
        ? QSize(d->extra->topextra->basew, d->extra->topextra->baseh)
        : QSize(0, 0);
}

bool QWidgetPrivate::setMinimumSize_helper(int &minw, int &minh)
{
    QWidget* const q = q_func();

    int mw = minw, mh = minh;
    if (mw == QWIDGETSIZE_MAX)
        mw = 0;
    if (mh == QWIDGETSIZE_MAX)
        mh = 0;
    if (Q_UNLIKELY(minw > QWIDGETSIZE_MAX || minh > QWIDGETSIZE_MAX)) {
        qWarning("QWidget::setMinimumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        minw = mw = qMin<int>(minw, QWIDGETSIZE_MAX);
        minh = mh = qMin<int>(minh, QWIDGETSIZE_MAX);
    }
    if (Q_UNLIKELY(minw < 0 || minh < 0)) {
        qWarning("QWidget::setMinimumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), minw, minh);
        minw = mw = qMax(minw, 0);
        minh = mh = qMax(minh, 0);
    }
    createExtra();
    if (extra->minw == mw && extra->minh == mh)
        return false;
    extra->minw = mw;
    extra->minh = mh;
    extra->explicitMinSize = (mw ? Qt::Horizontal : 0) | (mh ? Qt::Vertical : 0);
    return true;
}

void QWidgetPrivate::setConstraints_sys()
{
    QWidget* const q = q_func();
    if (extra && q->windowHandle()) {
        QWindow *win = q->windowHandle();
        QWindowPrivate *winp = qt_window_private(win);

        winp->minimumSize = QSize(extra->minw, extra->minh);
        winp->maximumSize = QSize(extra->maxw, extra->maxh);

        if (extra->topextra) {
            winp->baseSize = QSize(extra->topextra->basew, extra->topextra->baseh);
            winp->sizeIncrement = QSize(extra->topextra->incw, extra->topextra->inch);
        }

        if (winp->platformWindow) {
            fixPosIncludesFrame();
            winp->platformWindow->propagateSizeHints();
        }
    }
}

/*!
    \overload

    This function corresponds to setMinimumSize(QSize(minw, minh)).
    Sets the minimum width to  minw and the minimum height to 
    minh.
*/

void QWidget::setMinimumSize(int minw, int minh)
{
    QWidgetPrivate * const d = d_func();
    if (!d->setMinimumSize_helper(minw, minh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (minw > width() || minh > height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        bool maximized = isMaximized();
        resize(qMax(minw,width()), qMax(minh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
        if (maximized)
            data->window_state = data->window_state | Qt::WindowMaximized;
    }
#if QT_CONFIG(graphicsview)
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMinimumSize(minw, minh);
    }
#endif
    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

bool QWidgetPrivate::setMaximumSize_helper(int &maxw, int &maxh)
{
    QWidget* const q = q_func();
    if (Q_UNLIKELY(maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX)) {
        qWarning("QWidget::setMaximumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        maxw = qMin<int>(maxw, QWIDGETSIZE_MAX);
        maxh = qMin<int>(maxh, QWIDGETSIZE_MAX);
    }
    if (Q_UNLIKELY(maxw < 0 || maxh < 0)) {
        qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), maxw, maxh);
        maxw = qMax(maxw, 0);
        maxh = qMax(maxh, 0);
    }
    createExtra();
    if (extra->maxw == maxw && extra->maxh == maxh)
        return false;
    extra->maxw = maxw;
    extra->maxh = maxh;
    extra->explicitMaxSize = (maxw != QWIDGETSIZE_MAX ? Qt::Horizontal : 0) |
                             (maxh != QWIDGETSIZE_MAX ? Qt::Vertical : 0);
    return true;
}

/*!
    \overload

    This function corresponds to setMaximumSize(QSize( maxw, 
    maxh)). Sets the maximum width to  maxw and the maximum height
    to  maxh.
*/
void QWidget::setMaximumSize(int maxw, int maxh)
{
    QWidgetPrivate * const d = d_func();
    if (!d->setMaximumSize_helper(maxw, maxh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (maxw < width() || maxh < height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        resize(qMin(maxw,width()), qMin(maxh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
    }

#if QT_CONFIG(graphicsview)
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMaximumSize(maxw, maxh);
    }
#endif

    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

/*!
    \overload

    Sets the x (width) size increment to  w and the y (height) size
    increment to  h.
*/
void QWidget::setSizeIncrement(int w, int h)
{
    QWidgetPrivate * const d = d_func();
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->incw == w && x->inch == h)
        return;
    x->incw = w;
    x->inch = h;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    \overload

    This corresponds to setBaseSize(QSize( basew,  baseh)). Sets
    the widgets base size to width  basew and height  baseh.
*/
void QWidget::setBaseSize(int basew, int baseh)
{
    QWidgetPrivate * const d = d_func();
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->basew == basew && x->baseh == baseh)
        return;
    x->basew = basew;
    x->baseh = baseh;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    Sets both the minimum and maximum sizes of the widget to  s,
    thereby preventing it from ever growing or shrinking.

    This will override the default size constraints set by QLayout.

    To remove constraints, set the size to QWIDGETSIZE_MAX.

    Alternatively, if you want the widget to have a
    fixed size based on its contents, you can call
    QLayout::setSizeConstraint(QLayout::SetFixedSize);

    \sa maximumSize, minimumSize
*/

void QWidget::setFixedSize(const QSize & s)
{
    setFixedSize(s.width(), s.height());
}


/*!
    \fn void QWidget::setFixedSize(int w, int h)
    \overload

    Sets the width of the widget to  w and the height to  h.
*/

void QWidget::setFixedSize(int w, int h)
{
    QWidgetPrivate * const d = d_func();
    bool minSizeSet = d->setMinimumSize_helper(w, h);
    bool maxSizeSet = d->setMaximumSize_helper(w, h);
    if (!minSizeSet && !maxSizeSet)
        return;

    if (isWindow())
        d->setConstraints_sys();
    else
        d->updateGeometry_helper(true);

    if (w != QWIDGETSIZE_MAX || h != QWIDGETSIZE_MAX)
        resize(w, h);
}

void QWidget::setMinimumWidth(int w)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (w ? Qt::Horizontal : 0);
    setMinimumSize(w, minimumSize().height());
    d->extra->explicitMinSize = expl;
}

void QWidget::setMinimumHeight(int h)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (h ? Qt::Vertical : 0);
    setMinimumSize(minimumSize().width(), h);
    d->extra->explicitMinSize = expl;
}

void QWidget::setMaximumWidth(int w)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (w == QWIDGETSIZE_MAX ? 0 : Qt::Horizontal);
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMaxSize = expl;
}

void QWidget::setMaximumHeight(int h)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (h == QWIDGETSIZE_MAX ? 0 : Qt::Vertical);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMaxSize = expl;
}

/*!
    Sets both the minimum and maximum width of the widget to  w
    without changing the heights. Provided for convenience.

    \sa sizeHint(), minimumSize(), maximumSize(), setFixedSize()
*/

void QWidget::setFixedWidth(int w)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Horizontal;
    uint explMax = d->extra->explicitMaxSize | Qt::Horizontal;
    setMinimumSize(w, minimumSize().height());
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Sets both the minimum and maximum heights of the widget to  h
    without changing the widths. Provided for convenience.

    \sa sizeHint(), minimumSize(), maximumSize(), setFixedSize()
*/

void QWidget::setFixedHeight(int h)
{
    QWidgetPrivate * const d = d_func();
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Vertical;
    uint explMax = d->extra->explicitMaxSize | Qt::Vertical;
    setMinimumSize(minimumSize().width(), h);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Translates the widget coordinate  pos to the coordinate system
    of  parent. The  parent must not be 0 and must be a parent
    of the calling widget.

    \sa mapFrom(), mapToParent(), mapToGlobal(), underMouse()
*/

QPoint QWidget::mapTo(const QWidget * parent, const QPoint & pos) const
{
    QPoint p = pos;
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapTo(const QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");
            p = w->mapToParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate  pos from the coordinate system
    of  parent to this widget's coordinate system. The  parent
    must not be 0 and must be a parent of the calling widget.

    \sa mapTo(), mapFromParent(), mapFromGlobal(), underMouse()
*/

QPoint QWidget::mapFrom(const QWidget * parent, const QPoint & pos) const
{
    QPoint p(pos);
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapFrom(const QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");

            p = w->mapFromParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate  pos to a coordinate in the
    parent widget.

    Same as mapToGlobal() if the widget has no parent.

    \sa mapFromParent(), mapTo(), mapToGlobal(), underMouse()
*/

QPoint QWidget::mapToParent(const QPoint &pos) const
{
    return pos + data->crect.topLeft();
}

/*!
    Translates the parent widget coordinate  pos to widget
    coordinates.

    Same as mapFromGlobal() if the widget has no parent.

    \sa mapToParent(), mapFrom(), mapFromGlobal(), underMouse()
*/

QPoint QWidget::mapFromParent(const QPoint &pos) const
{
    return pos - data->crect.topLeft();
}


QWidget *QWidget::window() const
{
    QWidget *w = const_cast<QWidget *>(this);
    QWidget *p = w->parentWidget();
    while (!w->isWindow() && p) {
        w = p;
        p = p->parentWidget();
    }
    return w;
}

/*!
    \since 4.4

    Returns the native parent for this widget, i.e. the next ancestor widget
    that has a system identifier, or 0 if it does not have any native parent.

    \sa effectiveWinId()
*/
QWidget *QWidget::nativeParentWidget() const
{
    QWidget *parent = parentWidget();
    while (parent && !parent->internalWinId())
        parent = parent->parentWidget();
    return parent;
}

/*! \fn QWidget *QWidget::topLevelWidget() const
    \obsolete

    Use window() instead.
*/



/*!
  Returns the background role of the widget.

  The background role defines the brush from the widget's  palette that
  is used to render the background.

  If no explicit background role is set, the widget inherts its parent
  widget's background role.

  \sa setBackgroundRole(), foregroundRole()
 */
QPalette::ColorRole QWidget::backgroundRole() const
{

    const QWidget *w = this;
    do {
        QPalette::ColorRole role = w->d_func()->bg_role;
        if (role != QPalette::NoRole)
            return role;
        if (w->isWindow() || w->windowType() == Qt::SubWindow)
            break;
        w = w->parentWidget();
    } while (w);
    return QPalette::Window;
}

/*!
  Sets the background role of the widget to  role.

  The background role defines the brush from the widget's  palette that
  is used to render the background.

  If  role is QPalette::NoRole, then the widget inherits its
  parent's background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setBackgroundRole().

  \sa backgroundRole(), foregroundRole()
 */

void QWidget::setBackgroundRole(QPalette::ColorRole role)
{
    QWidgetPrivate * const d = d_func();
    d->bg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
    d->updateIsOpaque();
}

/*!
  Returns the foreground role.

  The foreground role defines the color from the widget's  palette that
  is used to draw the foreground.

  If no explicit foreground role is set, the function returns a role
  that contrasts with the background role.

  \sa setForegroundRole(), backgroundRole()
 */
QPalette::ColorRole QWidget::foregroundRole() const
{
    QWidgetPrivate * const d = d_func();
    QPalette::ColorRole rl = QPalette::ColorRole(d->fg_role);
    if (rl != QPalette::NoRole)
        return rl;
    QPalette::ColorRole role = QPalette::WindowText;
	// oye bgr decide fgr
    switch (backgroundRole()) {
    case QPalette::Button:
        role = QPalette::ButtonText;
        break;
    case QPalette::Base:
        role = QPalette::Text;
        break;
    case QPalette::Dark:
    case QPalette::Shadow:
        role = QPalette::Light;
        break;
    case QPalette::Highlight:
        role = QPalette::HighlightedText;
        break;
    case QPalette::ToolTipBase:
        role = QPalette::ToolTipText;
        break;
    default:
        ;
    }
    return role;
}

/*!
  Sets the foreground role of the widget to  role.

  The foreground role defines the color from the widget's  palette that
  is used to draw the foreground.

  If  role is QPalette::NoRole, the widget uses a foreground role
  that contrasts with the background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setForegroundRole().

  \sa foregroundRole(), backgroundRole()
 */
void QWidget::setForegroundRole(QPalette::ColorRole role)
{
    QWidgetPrivate * const d = d_func();
    d->fg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
}

const QPalette &QWidget::palette() const
{
    if (!isEnabled()) {
        data->pal.setCurrentColorGroup(QPalette::Disabled);
    } else if ((!isVisible() || isActiveWindow())&& !QApplicationPrivate::isBlockedByModal(const_cast<QWidget *>(this))        ) {
        data->pal.setCurrentColorGroup(QPalette::Active);
    } else {
        data->pal.setCurrentColorGroup(QPalette::Inactive);
    }
    return data->pal;
}

void QWidget::setPalette(const QPalette &palette)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_SetPalette, palette.resolve() != 0);

    // Determine which palette is inherited from this widget's ancestors and
    // QApplication::palette, resolve this against  palette (attributes from
    // the inherited palette are copied over this widget's palette). Then
    // propagate this palette to this widget's children.
    QPalette naturalPalette = d->naturalWidgetPalette(d->inheritedPaletteResolveMask);
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    d->setPalette_helper(resolvedPalette);
}

/*!
    \internal

    Returns the palette that the widget  w inherits from its ancestors and
    QApplication::palette.  inheritedMask is the combination of the widget's
    ancestors palette request masks (i.e., which attributes from the parent
    widget's palette are implicitly imposed on this widget by the user). Note
    that this font does not take into account the palette set on  w itself.
*/
QPalette QWidgetPrivate::naturalWidgetPalette(uint inheritedMask) const
{
    QWidget* const q = q_func();

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QPalette naturalPalette = QApplication::palette(q);
    if ((!q->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#if QT_CONFIG(graphicsview)
            || (extra && extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
            )) {
        if (QWidget *p = q->parentWidget()) {
            if (!p->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles) {
                if (!naturalPalette.isCopyOf(QApplication::palette())) {
                    QPalette inheritedPalette = p->palette();
                    inheritedPalette.resolve(inheritedMask);
                    naturalPalette = inheritedPalette.resolve(naturalPalette);
                } else {
                    naturalPalette = p->palette();
                }
            }
        }
#if QT_CONFIG(graphicsview)
        else if (extra && extra->proxyWidget) {
            QPalette inheritedPalette = extra->proxyWidget->palette();
            inheritedPalette.resolve(inheritedMask);
            naturalPalette = inheritedPalette.resolve(naturalPalette);
        }
#endif // QT_CONFIG(graphicsview)
    }
    naturalPalette.resolve(0);
    return naturalPalette;
}
/*!
    \internal

    Determine which palette is inherited from this widget's ancestors and
    QApplication::palette, resolve this against this widget's palette
    (attributes from the inherited palette are copied over this widget's
    palette). Then propagate this palette to this widget's children.
*/
void QWidgetPrivate::resolvePalette()
{
    QPalette naturalPalette = naturalWidgetPalette(inheritedPaletteResolveMask);
    QPalette resolvedPalette = data.pal.resolve(naturalPalette);
    setPalette_helper(resolvedPalette);
}

void QWidgetPrivate::setPalette_helper(const QPalette &palette)
{
    QWidget* const q = q_func();
    if (data.pal == palette && data.pal.resolve() == palette.resolve())
        return;
    data.pal = palette;
    updateSystemBackground();
    propagatePaletteChange();
    updateIsOpaque();
    q->update();
    updateIsOpaque();
}

void QWidget::setFont(const QFont &font)
{
    QWidgetPrivate * const d = d_func();

    const QStyleSheetStyle* style;
    if (d->extra && (style = qobject_cast<const QStyleSheetStyle*>(d->extra->style))) {
        style->saveWidgetFont(this, font);
    }

    setAttribute(Qt::WA_SetFont, font.resolve() != 0);

    // Determine which font is inherited from this widget's ancestors and
    // QApplication::font, resolve this against  font (attributes from the
    // inherited font are copied over). Then propagate this font to this
    // widget's children.
    QFont naturalFont = d->naturalWidgetFont(d->inheritedFontResolveMask);
    QFont resolvedFont = font.resolve(naturalFont);
    d->setFont_helper(resolvedFont);
}


QFont QWidgetPrivate::naturalWidgetFont(uint inheritedMask) const
{
    QWidget* const q = q_func();

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QFont naturalFont = QApplication::font(q);
    if ((!q->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#if QT_CONFIG(graphicsview)
            || (extra && extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
            )) {
        if (QWidget *p = q->parentWidget()) 
			{
            if (!p->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles) {
                if (!naturalFont.isCopyOf(QApplication::font())) {
                    if (inheritedMask != 0) {
                        QFont inheritedFont = p->font();
                        inheritedFont.resolve(inheritedMask);
                        naturalFont = inheritedFont.resolve(naturalFont);
                    } // else nothing to do (naturalFont = naturalFont)
                } else {
                    naturalFont = p->font();
                }
            }
        }
#if QT_CONFIG(graphicsview)
        else if (extra && extra->proxyWidget) {
            if (inheritedMask != 0) {
                QFont inheritedFont = extra->proxyWidget->font();
                inheritedFont.resolve(inheritedMask);
                naturalFont = inheritedFont.resolve(naturalFont);
            } // else nothing to do (naturalFont = naturalFont)
        }
#endif // QT_CONFIG(graphicsview)
    }
			
    naturalFont.resolve(0);
    return naturalFont;
}


void QWidgetPrivate::resolveFont()
{
    QFont naturalFont = naturalWidgetFont(inheritedFontResolveMask);	
    QFont resolvedFont = data.fnt.resolve(naturalFont);// naturalFont里面的属性构造data.fnt类型的font
    setFont_helper(resolvedFont); // will call updateFont();
}

void QWidgetPrivate::updateFont(const QFont &font)
{
    QWidget* const q = q_func();
		
    const QStyleSheetStyle* cssStyle;
    cssStyle = extra ? qobject_cast<const QStyleSheetStyle*>(extra->style) : 0;
    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    data.fnt = QFont(font, q); // new font updated

    // Combine new mask with natural mask and propagate to children.
#if QT_CONFIG(graphicsview)
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedFontResolveMask = p->d_func()->inheritedFontResolveMask | p->font().resolve();
    } else
#endif // QT_CONFIG(graphicsview)

    if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedFontResolveMask = 0;
    }
    uint newMask = data.fnt.resolve() | inheritedFontResolveMask;
	// 更新widget的子widget
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w) {
            if (0) {
            } else if (!useStyleSheetPropagationInWidgetStyles && w->testAttribute(Qt::WA_StyleSheet)) {
                // Style sheets follow a different font propagation scheme.
                if (cssStyle)
                    cssStyle->updateStyleSheetFont(w); 
            } else if ((!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
                // Propagate font changes.
                QWidgetPrivate *wd = w->d_func();
                wd->inheritedFontResolveMask = newMask;
                wd->resolveFont();
            }
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (!useStyleSheetPropagationInWidgetStyles && cssStyle) {
        cssStyle->updateStyleSheetFont(q);
    }
#endif

    QEvent e(QEvent::FontChange);
    QApplication::sendEvent(q, &e);
}

void QWidgetPrivate::setLayoutDirection_helper(Qt::LayoutDirection direction)
{
    QWidget* const q = q_func();

    if ( (direction == Qt::RightToLeft) == q->testAttribute(Qt::WA_RightToLeft))
        return;
    q->setAttribute(Qt::WA_RightToLeft, (direction == Qt::RightToLeft));
    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_SetLayoutDirection))
                w->d_func()->setLayoutDirection_helper(direction);
        }
    }
    QEvent e(QEvent::LayoutDirectionChange);
    QApplication::sendEvent(q, &e);
}

void QWidgetPrivate::resolveLayoutDirection()
{
    QWidget* const q = q_func();
    if (!q->testAttribute(Qt::WA_SetLayoutDirection))
        setLayoutDirection_helper(q->isWindow() ? QApplication::layoutDirection() : q->parentWidget()->layoutDirection());
}


void QWidget::setLayoutDirection(Qt::LayoutDirection direction)
{
    QWidgetPrivate * const d = d_func();

    if (direction == Qt::LayoutDirectionAuto) {
        unsetLayoutDirection();
        return;
    }

    setAttribute(Qt::WA_SetLayoutDirection);
    d->setLayoutDirection_helper(direction);
}

Qt::LayoutDirection QWidget::layoutDirection() const
{
    return testAttribute(Qt::WA_RightToLeft) ? Qt::RightToLeft : Qt::LeftToRight;
}

void QWidget::unsetLayoutDirection()
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_SetLayoutDirection, false);
    d->resolveLayoutDirection();
}


#ifndef QT_NO_CURSOR
QCursor QWidget::cursor() const
{
    QWidgetPrivate * const d = d_func();
    if (testAttribute(Qt::WA_SetCursor))
        return (d->extra && d->extra->curs)
            ? *d->extra->curs
            : QCursor(Qt::ArrowCursor);
    if (isWindow() || !parentWidget())
        return QCursor(Qt::ArrowCursor);
    return parentWidget()->cursor();
}

void QWidget::setCursor(const QCursor &cursor)
{
    QWidgetPrivate * const d = d_func();
// On Mac we must set the cursor even if it is the ArrowCursor.
#if 1 // Used to be excluded in Qt4 for Q_WS_MAC
    if (cursor.shape() != Qt::ArrowCursor
        || (d->extra && d->extra->curs))
#endif
    {
        d->createExtra();
        QCursor *newCursor = new QCursor(cursor);
        delete d->extra->curs;
        d->extra->curs = newCursor;
    }
    setAttribute(Qt::WA_SetCursor);
    d->setCursor_sys(cursor);

    QEvent event(QEvent::CursorChange);
    QApplication::sendEvent(this, &event);
}

void QWidgetPrivate::setCursor_sys(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    QWidget* const q = q_func();
    qt_qpa_set_cursor(q, false);
}

void QWidget::unsetCursor()
{
    QWidgetPrivate * const d = d_func();
    if (d->extra) {
        delete d->extra->curs;
        d->extra->curs = 0;
    }
    if (!isWindow())
        setAttribute(Qt::WA_SetCursor, false);
    d->unsetCursor_sys();

    QEvent event(QEvent::CursorChange);
    QApplication::sendEvent(this, &event);
}

void QWidgetPrivate::unsetCursor_sys()
{
    QWidget* const q = q_func();
    qt_qpa_set_cursor(q, false);
}

static inline void applyCursor(QWidget *w, const QCursor &c)
{
    if (QWindow *window = w->windowHandle())
        window->setCursor(c);
}

static inline void unsetCursor(QWidget *w)
{
    if (QWindow *window = w->windowHandle())
        window->unsetCursor();
}

void qt_qpa_set_cursor(QWidget *w, bool force)
{
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    static QPointer<QWidget> lastUnderMouse = 0;
    if (force) {
        lastUnderMouse = w;
    } else if (lastUnderMouse) {
        const WId lastWinId = lastUnderMouse->effectiveWinId();
        const WId winId = w->effectiveWinId();
        if (lastWinId && lastWinId == winId)
            w = lastUnderMouse;
    } else if (!w->internalWinId()) {
        return; // The mouse is not under this widget, and it's not native, so don't change it.
    }

    while (!w->internalWinId() && w->parentWidget() && !w->isWindow()
           && !w->testAttribute(Qt::WA_SetCursor))
        w = w->parentWidget();

    QWidget *nativeParent = w;
    if (!w->internalWinId())
        nativeParent = w->nativeParentWidget();
    if (!nativeParent || !nativeParent->internalWinId())
        return;

    if (w->isWindow() || w->testAttribute(Qt::WA_SetCursor)) {
        if (w->isEnabled())
            applyCursor(nativeParent, w->cursor());
        else
            // Enforce the windows behavior of clearing the cursor on
            // disabled widgets.
            unsetCursor(nativeParent);
    } else {
        unsetCursor(nativeParent);
    }
}
#endif

void QWidget::render(QPaintDevice *target, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    QPainter p(target);
    render(&p, targetOffset, sourceRegion, renderFlags);
}

void QWidget::render(QPainter *painter, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    const qreal opacity = painter->opacity();
    if (qFuzzyIsNull(opacity)) // opacity < 0.001f
        return; // Fully transparent.

    QWidgetPrivate * const d = d_func();
    const bool inRenderWithPainter = d->extra && d->extra->inRenderWithPainter;
    const QRegion toBePainted = !inRenderWithPainter ? d->prepareToRender(sourceRegion, renderFlags)
                                                     : sourceRegion;
    if (toBePainted.isEmpty())
        return;

    if (!d->extra)
        d->createExtra();
    d->extra->inRenderWithPainter = true;

    QPaintEngine *engine = painter->paintEngine();
    QPaintEnginePrivate *enginePriv = engine->d_func();
    QPaintDevice *target = engine->paintDevice();

    // Render via a pixmap when dealing with non-opaque painters or printers.
    if (!inRenderWithPainter && (opacity < 1.0 || (target->devType() == QInternal::Printer))) {
        d->render_helper(painter, targetOffset, toBePainted, renderFlags);
        d->extra->inRenderWithPainter = inRenderWithPainter;
        return;
    }

    // Set new shared painter.
    QPainter *oldPainter = d->sharedPainter();
    d->setSharedPainter(painter);

    // Save current system clip, viewport and transform,
    const QTransform oldTransform = enginePriv->systemTransform;
    const QRegion oldSystemClip = enginePriv->systemClip;
    const QRegion oldSystemViewport = enginePriv->systemViewport;

    // This ensures that all painting triggered by render() is clipped to the current engine clip.
    if (painter->hasClipping()) {
        const QRegion painterClip = painter->deviceTransform().map(painter->clipRegion());
        enginePriv->setSystemViewport(oldSystemClip.isEmpty() ? painterClip : oldSystemClip & painterClip);
    } else {
        enginePriv->setSystemViewport(oldSystemClip);
    }

    d->render(target, targetOffset, toBePainted, renderFlags);

    // Restore system clip, viewport and transform.
    enginePriv->setSystemViewport(oldSystemViewport);
    enginePriv->setSystemTransform(oldTransform);
    enginePriv->systemClip = oldSystemClip;
    enginePriv->systemStateChanged();

    // Restore shared painter.
    d->setSharedPainter(oldPainter);

    d->extra->inRenderWithPainter = inRenderWithPainter;
}

static void sendResizeEvents(QWidget *target)
{
    QResizeEvent e(target->size(), QSize());
    QApplication::sendEvent(target, &e);

    const QObjectList children = target->children();
    for (int i = 0; i < children.size(); ++i) {
        if (!children.at(i)->isWidgetType())
            continue;
        QWidget *child = static_cast<QWidget*>(children.at(i));
        if (!child->isWindow() && child->testAttribute(Qt::WA_PendingResizeEvent))
            sendResizeEvents(child);
    }
}

/*!
	把当前widget直接画到参数rectangle指定的size, 画成的pixmap
*/
QPixmap QWidget::grab(const QRect &rectangle)
{
    QWidgetPrivate * const d = d_func();
    if (testAttribute(Qt::WA_PendingResizeEvent) || !testAttribute(Qt::WA_WState_Created))
        sendResizeEvents(this);

    const QWidget::RenderFlags renderFlags = QWidget::DrawWindowBackground | QWidget::DrawChildren | QWidget::IgnoreMask;

    const bool oldDirtyOpaqueChildren =  d->dirtyOpaqueChildren;
    QRect r(rectangle);
    if (r.width() < 0 || r.height() < 0) {
        // For grabbing widgets that haven't been shown yet,
        // we trigger the layouting mechanism to determine the widget's size.
        r = d->prepareToRender(QRegion(), renderFlags).boundingRect();
        r.setTopLeft(rectangle.topLeft());
    }

    if (!r.intersects(rect()))
        return QPixmap();

    const qreal dpr = devicePixelRatioF();
    QPixmap res((QSizeF(r.size()) * dpr).toSize());
    res.setDevicePixelRatio(dpr);
    if (!d->isOpaque)
        res.fill(Qt::transparent);
    d->render(&res, QPoint(), QRegion(r), renderFlags);

    d->dirtyOpaqueChildren = oldDirtyOpaqueChildren;
    return res;
}


QGraphicsEffect *QWidget::graphicsEffect() const
{
    QWidgetPrivate * const d = d_func();
    return d->graphicsEffect;
}

void QWidget::setGraphicsEffect(QGraphicsEffect *effect)
{
    QWidgetPrivate * const d = d_func();
    if (d->graphicsEffect == effect)
        return;

    if (d->graphicsEffect) {
        d->invalidateBuffer(rect());
        delete d->graphicsEffect;
        d->graphicsEffect = 0;
    }

    if (effect) {
        // Set new effect.
        QGraphicsEffectSourcePrivate *sourced = new QWidgetEffectSourcePrivate(this);
        QGraphicsEffectSource *source = new QGraphicsEffectSource(*sourced);
        d->graphicsEffect = effect;
        effect->d_func()->setGraphicsEffectSource(source);
        update();
    }

    d->updateIsOpaque();
}

bool QWidgetPrivate::isAboutToShow() const
{
    if (data.in_show)
        return true;

    QWidget* const q = q_func();
    if (q->isHidden())
        return false;

    // The widget will be shown if any of its ancestors are about to show.
    QWidget *parent = q->parentWidget();
    return parent ? parent->d_func()->isAboutToShow() : false;
}

QRegion QWidgetPrivate::prepareToRender(const QRegion &region, QWidget::RenderFlags renderFlags)
{
    QWidget* const q = q_func();
    const bool isVisible = q->isVisible();

    // Make sure the widget is laid out correctly.
    if (!isVisible && !isAboutToShow()) {
        QWidget *topLevel = q->window();
        (void)topLevel->d_func()->topData(); // Make sure we at least have top-data.
        topLevel->ensurePolished();

        // Invalidate the layout of hidden ancestors (incl. myself) and pretend
        // they're not explicitly hidden.
        QWidget *widget = q;
        QWidgetList hiddenWidgets;
        while (widget) {
            if (widget->isHidden()) {
                widget->setAttribute(Qt::WA_WState_Hidden, false);
                hiddenWidgets.append(widget);
                if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                    widget->d_func()->updateGeometry_helper(true);
            }
            widget = widget->parentWidget();
        }

        // Activate top-level layout.
        if (topLevel->d_func()->layout)
            topLevel->d_func()->layout->activate();

        // Adjust size if necessary.
        QTLWExtra *topLevelExtra = topLevel->d_func()->maybeTopData();
        if (topLevelExtra && !topLevelExtra->sizeAdjusted
            && !topLevel->testAttribute(Qt::WA_Resized)) {
            topLevel->adjustSize();
            topLevel->setAttribute(Qt::WA_Resized, false);
        }

        // Activate child layouts.
        topLevel->d_func()->activateChildLayoutsRecursively();

        // We're not cheating with WA_WState_Hidden anymore.
        for (int i = 0; i < hiddenWidgets.size(); ++i) {
            QWidget *widget = hiddenWidgets.at(i);
            widget->setAttribute(Qt::WA_WState_Hidden);
            if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                widget->parentWidget()->d_func()->layout->invalidate();
        }
    } else if (isVisible) {
        q->window()->d_func()->sendPendingMoveAndResizeEvents(true, true);
    }

    // Calculate the region to be painted.
    QRegion toBePainted = !region.isEmpty() ? region : QRegion(q->rect());
    if (!(renderFlags & QWidget::IgnoreMask) && extra && extra->hasMask)
        toBePainted &= extra->mask;
    return toBePainted;
}

void QWidgetPrivate::render_helper(QPainter *painter, const QPoint &targetOffset, const QRegion &toBePainted,
                                   QWidget::RenderFlags renderFlags)
{
    Q_ASSERT(painter);
    Q_ASSERT(!toBePainted.isEmpty());

    QWidget* const q = q_func();
    const QTransform originalTransform = painter->worldTransform();
    const bool useDeviceCoordinates = originalTransform.isScaling();
    if (!useDeviceCoordinates) {
        // Render via a pixmap.
        const QRect rect = toBePainted.boundingRect();
        const QSize size = rect.size();
        if (size.isNull())
            return;

        const qreal pixmapDevicePixelRatio = painter->device()->devicePixelRatioF();
        QPixmap pixmap(size * pixmapDevicePixelRatio);
        pixmap.setDevicePixelRatio(pixmapDevicePixelRatio);

        if (!(renderFlags & QWidget::DrawWindowBackground) || !isOpaque)
            pixmap.fill(Qt::transparent);
        q->render(&pixmap, QPoint(), toBePainted, renderFlags);

        const bool restore = !(painter->renderHints() & QPainter::SmoothPixmapTransform);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

        painter->drawPixmap(targetOffset, pixmap);

        if (restore)
            painter->setRenderHints(QPainter::SmoothPixmapTransform, false);

    } else {
        // Render via a pixmap in device coordinates (to avoid pixmap scaling).
        QTransform transform = originalTransform;
        transform.translate(targetOffset.x(), targetOffset.y());

        QPaintDevice *device = painter->device();
        Q_ASSERT(device);

        // Calculate device rect.
        const QRectF rect(toBePainted.boundingRect());
        QRect deviceRect = transform.mapRect(QRectF(0, 0, rect.width(), rect.height())).toAlignedRect();
        deviceRect &= QRect(0, 0, device->width(), device->height());

        QPixmap pixmap(deviceRect.size());
        pixmap.fill(Qt::transparent);

        // Create a pixmap device coordinate painter.
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(painter->renderHints());
        transform *= QTransform::fromTranslate(-deviceRect.x(), -deviceRect.y());
        pixmapPainter.setTransform(transform);

        q->render(&pixmapPainter, QPoint(), toBePainted, renderFlags);
        pixmapPainter.end();

        // And then draw the pixmap.
        painter->setTransform(QTransform());
        painter->drawPixmap(deviceRect.topLeft(), pixmap);
        painter->setTransform(originalTransform);
    }
}

void QWidgetPrivate::drawWidget(QPaintDevice *pdev, const QRegion &rgn, const QPoint &offset, int flags,
                                QPainter *sharedPainter, QWidgetBackingStore *backingStore)
{
    if (rgn.isEmpty())
        return;

    const bool asRoot = flags & DrawAsRoot;
    bool onScreen = paintOnScreen();

    QWidget* const q = q_func();
#if QT_CONFIG(graphicseffect)
    if (graphicsEffect && graphicsEffect->isEnabled()) {
        QGraphicsEffectSource *source = graphicsEffect->d_func()->source;
        QWidgetEffectSourcePrivate *sourced = static_cast<QWidgetEffectSourcePrivate *>
                                                         (source->d_func());
        if (!sourced->context) {
            QWidgetPaintContext context(pdev, rgn, offset, flags, sharedPainter, backingStore);
            sourced->context = &context;
            if (!sharedPainter) {
                setSystemClip(pdev, rgn.translated(offset));
                QPainter p(pdev);
                p.translate(offset);
                context.painter = &p;
                graphicsEffect->draw(&p);
                setSystemClip(pdev, QRegion());
            } else {
                context.painter = sharedPainter;
                if (sharedPainter->worldTransform() != sourced->lastEffectTransform) {
                    sourced->invalidateCache();
                    sourced->lastEffectTransform = sharedPainter->worldTransform();
                }
                sharedPainter->save();
                sharedPainter->translate(offset);
                graphicsEffect->draw(sharedPainter);
                sharedPainter->restore();
            }
            sourced->context = 0;

            // Native widgets need to be marked dirty on screen so painting will be done in correct context
            // Same check as in the no effects case below.
            if (backingStore && !onScreen && !asRoot && (q->internalWinId() || !q->nativeParentWidget()->isWindow()))
                backingStore->markDirtyOnScreen(rgn, q, offset);

            return;
        }
    }
#endif // QT_CONFIG(graphicseffect)

    const bool alsoOnScreen = flags & DrawPaintOnScreen;
    const bool recursive = flags & DrawRecursive;
    const bool alsoInvisible = flags & DrawInvisible;

    QRegion toBePainted(rgn);
    if (asRoot && !alsoInvisible)
        toBePainted &= clipRect(); //(rgn & visibleRegion());
    if (!(flags & DontSubtractOpaqueChildren))
        subtractOpaqueChildren(toBePainted, q->rect());

    if (!toBePainted.isEmpty()) {
        if (!onScreen || alsoOnScreen) {
            //update the "in paint event" flag
            if (Q_UNLIKELY(q->testAttribute(Qt::WA_WState_InPaintEvent)))
                qWarning("QWidget::repaint: Recursive repaint detected");
            q->setAttribute(Qt::WA_WState_InPaintEvent);

            //clip away the new area
#ifndef QT_NO_PAINT_DEBUG
            bool flushed = QWidgetBackingStore::flushPaint(q, toBePainted);
#endif
            QPaintEngine *paintEngine = pdev->paintEngine();
            if (paintEngine) {
                setRedirected(pdev, -offset);
                if (sharedPainter)
                    setSystemClip(pdev, toBePainted);
                else
                    paintEngine->d_func()->systemRect = q->data->crect;

                //paint the background
                if ((asRoot || q->autoFillBackground() || onScreen || q->testAttribute(Qt::WA_StyledBackground))
                    && !q->testAttribute(Qt::WA_OpaquePaintEvent) && !q->testAttribute(Qt::WA_NoSystemBackground)) {
#ifndef QT_NO_OPENGL
                    beginBackingStorePainting();
#endif
                    QPainter p(q);
                    paintBackground(&p, toBePainted, (asRoot || onScreen) ? flags | DrawAsRoot : 0);
#ifndef QT_NO_OPENGL
                    endBackingStorePainting();
#endif
                }

                if (!sharedPainter)
                    setSystemClip(pdev, toBePainted.translated(offset));

                if (!onScreen && !asRoot && !isOpaque && q->testAttribute(Qt::WA_TintedBackground)) {
#ifndef QT_NO_OPENGL
                    beginBackingStorePainting();
#endif
                    QPainter p(q);
                    QColor tint = q->palette().window().color();
                    tint.setAlphaF(qreal(.6));
                    p.fillRect(toBePainted.boundingRect(), tint);
#ifndef QT_NO_OPENGL
                    endBackingStorePainting();
#endif
                }
            }

            bool skipPaintEvent = false;
#ifndef QT_NO_OPENGL
            if (renderToTexture) {
                // This widget renders into a texture which is composed later. We just need to
                // punch a hole in the backingstore, so the texture will be visible.
                if (!q->testAttribute(Qt::WA_AlwaysStackOnTop)) {
                    beginBackingStorePainting();
                    if (backingStore) {
                        QPainter p(q);
                        p.setCompositionMode(QPainter::CompositionMode_Source);
                        p.fillRect(q->rect(), Qt::transparent);
                    } else {
                        QImage img = grabFramebuffer();
                        QPainter p(q);
                        // We are not drawing to a backingstore: fall back to QImage
                        p.drawImage(q->rect(), img);
                        skipPaintEvent = true;
                    }
                    endBackingStorePainting();
                }
                if (renderToTextureReallyDirty)
                    renderToTextureReallyDirty = 0;
                else
                    skipPaintEvent = true;
            }
#endif // QT_NO_OPENGL

            if (!skipPaintEvent) {
                //actually send the paint event
                sendPaintEvent(toBePainted);
            }

            // Native widgets need to be marked dirty on screen so painting will be done in correct context
            if (backingStore && !onScreen && !asRoot && (q->internalWinId() || (q->nativeParentWidget() && !q->nativeParentWidget()->isWindow())))
                backingStore->markDirtyOnScreen(toBePainted, q, offset);

            //restore
            if (paintEngine) {
                restoreRedirected();
                if (!sharedPainter)
                    paintEngine->d_func()->systemRect = QRect();
                else
                    paintEngine->d_func()->currentClipDevice = 0;

                setSystemClip(pdev, QRegion());
            }
            q->setAttribute(Qt::WA_WState_InPaintEvent, false);
            if (Q_UNLIKELY(q->paintingActive()))
                qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");

            if (paintEngine && paintEngine->autoDestruct()) {  delete paintEngine;   }

#ifndef QT_NO_PAINT_DEBUG
            if (flushed)   QWidgetBackingStore::unflushPaint(q, toBePainted);
#endif
        } 
		else if (q->isWindow()) 
		{
            QPaintEngine *engine = pdev->paintEngine();
            if (engine) {
                QPainter p(pdev);
                p.setClipRegion(toBePainted);
                const QBrush bg = q->palette().brush(QPalette::Window);
                if (bg.style() == Qt::TexturePattern)
                    p.drawTiledPixmap(q->rect(), bg.texture());
                else
                    p.fillRect(q->rect(), bg);

                if (engine->autoDestruct())
                    delete engine;
            }
        }
    }

    if (recursive && !children.isEmpty()) {
        paintSiblingsRecursive(pdev, children, children.size() - 1, rgn, offset, flags & ~DrawAsRoot
                                , sharedPainter, backingStore);
    }
}

void QWidgetPrivate::sendPaintEvent(const QRegion &toBePainted)
{
    QWidget* const q = q_func();
    QPaintEvent e(toBePainted);
    QCoreApplication::sendSpontaneousEvent(q, &e);

#ifndef QT_NO_OPENGL
    if (renderToTexture)
        resolveSamples();
#endif // QT_NO_OPENGL
}

void QWidgetPrivate::render(QPaintDevice *target, const QPoint &targetOffset,
                            const QRegion &sourceRegion, QWidget::RenderFlags renderFlags)
{
    if (Q_UNLIKELY(!target)) {
        qWarning("QWidget::render: null pointer to paint device");
        return;
    }

    const bool inRenderWithPainter = extra && extra->inRenderWithPainter;
    QRegion paintRegion = !inRenderWithPainter
                          ? prepareToRender(sourceRegion, renderFlags)
                          : sourceRegion;
    if (paintRegion.isEmpty())
        return;

    QPainter *oldSharedPainter = inRenderWithPainter ? sharedPainter() : 0;

    // Use the target's shared painter if set (typically set when doing
    // "other->render(widget);" in the widget's paintEvent.
    if (target->devType() == QInternal::Widget) {
        QWidgetPrivate *targetPrivate = static_cast<QWidget *>(target)->d_func();
        if (targetPrivate->extra && targetPrivate->extra->inRenderWithPainter) {
            QPainter *targetPainter = targetPrivate->sharedPainter();
            if (targetPainter && targetPainter->isActive())
                setSharedPainter(targetPainter);
        }
    }


    // Use the target's redirected device if set and adjust offset and paint
    // region accordingly. This is typically the case when people call render
    // from the paintEvent.
    QPoint offset = targetOffset;
    offset -= paintRegion.boundingRect().topLeft();
    QPoint redirectionOffset;
    QPaintDevice *redirected = 0;

    if (target->devType() == QInternal::Widget)
        redirected = static_cast<QWidget *>(target)->d_func()->redirected(&redirectionOffset);
    if (!redirected)
        redirected = QPainter::redirected(target, &redirectionOffset);

    if (redirected) {
        target = redirected;
        offset -= redirectionOffset;
    }

    if (!inRenderWithPainter) { // Clip handled by shared painter (in qpainter.cpp).
        if (QPaintEngine *targetEngine = target->paintEngine()) {
            const QRegion targetSystemClip = targetEngine->systemClip();
            if (!targetSystemClip.isEmpty())
                paintRegion &= targetSystemClip.translated(-offset);
        }
    }

    // Set backingstore flags.
    int flags = DrawPaintOnScreen | DrawInvisible;
    if (renderFlags & QWidget::DrawWindowBackground)
        flags |= DrawAsRoot;

    if (renderFlags & QWidget::DrawChildren)
        flags |= DrawRecursive;
    else
        flags |= DontSubtractOpaqueChildren;

    flags |= DontSetCompositionMode;

    // Render via backingstore.
    drawWidget(target, paintRegion, offset, flags, sharedPainter());

    // Restore shared painter.
    if (oldSharedPainter)
        setSharedPainter(oldSharedPainter);
}

void QWidgetPrivate::paintSiblingsRecursive(QPaintDevice *pdev, const QObjectList& siblings, int index, const QRegion &rgn,
                                            const QPoint &offset, int flags
                                            , QPainter *sharedPainter, QWidgetBackingStore *backingStore)
{
    QWidget *w = 0;
    QRect boundingRect;
    bool dirtyBoundingRect = true;
    const bool exludeOpaqueChildren = (flags & DontDrawOpaqueChildren);
    const bool excludeNativeChildren = (flags & DontDrawNativeChildren);

    do {
        QWidget *x =  qobject_cast<QWidget*>(siblings.at(index));
        if (x && !(exludeOpaqueChildren && x->d_func()->isOpaque) && !x->isHidden() && !x->isWindow()
            && !(excludeNativeChildren && x->internalWinId())) {
            if (dirtyBoundingRect) {
                boundingRect = rgn.boundingRect();
                dirtyBoundingRect = false;
            }

            if (qRectIntersects(boundingRect, x->d_func()->effectiveRectFor(x->data->crect))) {
                w = x;
                break;
            }
        }
        --index;
    } while (index >= 0);

    if (!w)
        return;

    QWidgetPrivate *wd = w->d_func();
    const QPoint widgetPos(w->data->crect.topLeft());
    const bool hasMask = wd->extra && wd->extra->hasMask && !wd->graphicsEffect;
    if (index > 0) {
        QRegion wr(rgn);
        if (wd->isOpaque)
            wr -= hasMask ? wd->extra->mask.translated(widgetPos) : w->data->crect;
        paintSiblingsRecursive(pdev, siblings, --index, wr, offset, flags
                               , sharedPainter, backingStore);
    }

    if (w->updatesEnabled()
#if QT_CONFIG(graphicsview)
            && (!w->d_func()->extra || !w->d_func()->extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
       ) {
        QRegion wRegion(rgn);
        wRegion &= wd->effectiveRectFor(w->data->crect);
        wRegion.translate(-widgetPos);
        if (hasMask)
            wRegion &= wd->extra->mask;
        wd->drawWidget(pdev, wRegion, offset + widgetPos, flags, sharedPainter, backingStore);
    }
}

#if QT_CONFIG(graphicseffect)
QRectF QWidgetEffectSourcePrivate::boundingRect(Qt::CoordinateSystem system) const
{
    if (system != Qt::DeviceCoordinates)
        return m_widget->rect();

    if (Q_UNLIKELY(!context)) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::boundingRect: Not yet implemented, lacking device context");
        return QRectF();
    }

    return context->painter->worldTransform().mapRect(m_widget->rect());
}

void QWidgetEffectSourcePrivate::draw(QPainter *painter)
{
    if (!context || context->painter != painter) {
        m_widget->render(painter);
        return;
    }

    // The region saved in the context is neither clipped to the rect
    // nor the mask, so we have to clip it here before calling drawWidget.
    QRegion toBePainted = context->rgn;
    toBePainted &= m_widget->rect();
    QWidgetPrivate *wd = qt_widget_private(m_widget);
    if (wd->extra && wd->extra->hasMask)
        toBePainted &= wd->extra->mask;

    wd->drawWidget(context->pdev, toBePainted, context->offset, context->flags,
                   context->sharedPainter, context->backingStore);
}

QPixmap QWidgetEffectSourcePrivate::pixmap(Qt::CoordinateSystem system, QPoint *offset,
                                           QGraphicsEffect::PixmapPadMode mode) const
{
    const bool deviceCoordinates = (system == Qt::DeviceCoordinates);
    if (Q_UNLIKELY(!context && deviceCoordinates)) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::pixmap: Not yet implemented, lacking device context");
        return QPixmap();
    }

    QPoint pixmapOffset;
    QRectF sourceRect = m_widget->rect();

    if (deviceCoordinates) {
        const QTransform &painterTransform = context->painter->worldTransform();
        sourceRect = painterTransform.mapRect(sourceRect);
        pixmapOffset = painterTransform.map(pixmapOffset);
    }

    QRect effectRect;

    if (mode == QGraphicsEffect::PadToEffectiveBoundingRect)
        effectRect = m_widget->graphicsEffect()->boundingRectFor(sourceRect).toAlignedRect();
    else if (mode == QGraphicsEffect::PadToTransparentBorder)
        effectRect = sourceRect.adjusted(-1, -1, 1, 1).toAlignedRect();
    else
        effectRect = sourceRect.toAlignedRect();

    if (offset)
        *offset = effectRect.topLeft();

    pixmapOffset -= effectRect.topLeft();

    const qreal dpr = context->painter->device()->devicePixelRatioF();
    QPixmap pixmap(effectRect.size() * dpr);
    pixmap.setDevicePixelRatio(dpr);

    pixmap.fill(Qt::transparent);
    m_widget->render(&pixmap, pixmapOffset, QRegion(), QWidget::DrawChildren);
    return pixmap;
}
#endif // QT_CONFIG(graphicseffect)

#if QT_CONFIG(graphicsview)
/*!
    \internal

    Finds the nearest widget embedded in a graphics proxy widget along the chain formed by this
    widget and its ancestors. The search starts at  origin (inclusive).
    If successful, the function returns the proxy that embeds the widget, or 0 if no embedded
    widget was found.
*/
QGraphicsProxyWidget * QWidgetPrivate::nearestGraphicsProxyWidget(const QWidget *origin)
{
    if (origin) {
        QWExtra *extra = origin->d_func()->extra;
        if (extra && extra->proxyWidget)
            return extra->proxyWidget;
        return nearestGraphicsProxyWidget(origin->parentWidget());
    }
    return 0;
}
#endif

void QWidgetPrivate::setLocale_helper(const QLocale &loc, bool forceUpdate)
{
    QWidget* const q = q_func();
    if (locale == loc && !forceUpdate)
        return;

    locale = loc;

    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (!w)
                continue;
            if (w->testAttribute(Qt::WA_SetLocale))
                continue;
            if (w->isWindow() && !w->testAttribute(Qt::WA_WindowPropagation))
                continue;
            w->d_func()->setLocale_helper(loc, forceUpdate);
        }
    }
    QEvent e(QEvent::LocaleChange);
    QApplication::sendEvent(q, &e);
}

void QWidget::setLocale(const QLocale &locale)
{
    QWidgetPrivate * const d = d_func();

    setAttribute(Qt::WA_SetLocale);
    d->setLocale_helper(locale);
}

QLocale QWidget::locale() const
{
    QWidgetPrivate * const d = d_func();

    return d->locale;
}

void QWidgetPrivate::resolveLocale()
{
    QWidget* const q = q_func();

    if (!q->testAttribute(Qt::WA_SetLocale)) {
        QWidget *parent = q->parentWidget();
        setLocale_helper(!parent || (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation))
                         ? QLocale() : parent->locale());
    }
}

void QWidget::unsetLocale()
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_SetLocale, false);
    d->resolveLocale();
}


QString QWidget::windowTitle() const
{
    QWidgetPrivate * const d = d_func();
    if (d->extra && d->extra->topextra) {
        if (!d->extra->topextra->caption.isEmpty())
            return d->extra->topextra->caption;
        if (!d->extra->topextra->filePath.isEmpty())
            return QFileInfo(d->extra->topextra->filePath).fileName() + QLatin1String("[*]");
    }
    return QString();
}


QString qt_setWindowTitle_helperHelper(const QString &title, const QWidget *widget)
{
    QString cap = title;
    if (cap.isEmpty())
        return cap;

    QLatin1String placeHolder("[*]");
    int index = cap.indexOf(placeHolder);

    // here the magic begins
    while (index != -1) {
        index += placeHolder.size();
        int count = 1;
        while (cap.indexOf(placeHolder, index) == index) {
            ++count;
            index += placeHolder.size();
        }

        if (count%2) { // odd number of [*] -> replace last one
            int lastIndex = cap.lastIndexOf(placeHolder, index - 1);
            if (widget->isWindowModified()
             && widget->style()->styleHint(QStyle::SH_TitleBar_ModifyNotification, 0, widget))
                cap.replace(lastIndex, 3, QWidget::tr("*"));
            else
                cap.remove(lastIndex, 3);
        }

        index = cap.indexOf(placeHolder, index);
    }

    cap.replace(QLatin1String("[*][*]"), placeHolder);

    return cap;
}

void QWidgetPrivate::setWindowTitle_helper(const QString &title)
{
    QWidget* const q = q_func();
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowTitle_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    QWidget* const q = q_func();
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setTitle(caption);

}

void QWidgetPrivate::setWindowIconText_helper(const QString &title)
{
    QWidget* const q = q_func();
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowIconText_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    QWidget* const q = q_func();
    // ### The QWidget property is deprecated, but the XCB window function is not.
    // It should remain available for the rare application that needs it.
    if (QWindow *window = q->windowHandle())
        QXcbWindowFunctions::setWmWindowIconText(window, iconText);
}

void QWidget::setWindowIconText(const QString &iconText)
{
    if (QWidget::windowIconText() == iconText)
        return;

    QWidgetPrivate * const d = d_func();
    d->topData()->iconText = iconText;
    d->setWindowIconText_helper(iconText);

    QEvent e(QEvent::IconTextChange);
    QApplication::sendEvent(this, &e);

    emit windowIconTextChanged(iconText);
}

void QWidget::setWindowTitle(const QString &title)
{
    if (QWidget::windowTitle() == title && !title.isEmpty() && !title.isNull())
        return;

    QWidgetPrivate * const d = d_func();
    d->topData()->caption = title;
    d->setWindowTitle_helper(title);

    QEvent e(QEvent::WindowTitleChange);
    QApplication::sendEvent(this, &e);

    emit windowTitleChanged(title);
}


QIcon QWidget::windowIcon() const
{
    const QWidget *w = this;
    while (w) {
        const QWidgetPrivate *d = w->d_func();
        if (d->extra && d->extra->topextra && d->extra->topextra->icon)
            return *d->extra->topextra->icon;
        w = w->parentWidget();
    }
    return QApplication::windowIcon();
}

void QWidgetPrivate::setWindowIcon_helper()
{
    QWidget* const q = q_func();
    QEvent e(QEvent::WindowIconChange);

    // Do not send the event if the widget is a top level.
    // In that case, setWindowIcon_sys does it, and event propagation from
    // QWidgetWindow to the top level QWidget ensures that the event reaches
    // the top level anyhow
    if (!q->windowHandle())
        QApplication::sendEvent(q, &e);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow())
            QApplication::sendEvent(w, &e);
    }
	// 告诉所有子Widget windowIcon改变了, 他的意义是什么?
}

void QWidget::setWindowIcon(const QIcon &icon)
{
    QWidgetPrivate * const d = d_func();

    setAttribute(Qt::WA_SetWindowIcon, !icon.isNull());
    d->createTLExtra();

    if (!d->extra->topextra->icon)
        d->extra->topextra->icon = new QIcon();
    *d->extra->topextra->icon = icon;

    d->setWindowIcon_sys();
    d->setWindowIcon_helper();

    emit windowIconChanged(icon);
}

void QWidgetPrivate::setWindowIcon_sys()
{
    QWidget* const q = q_func();
    if (QWindow *window = q->windowHandle())
        window->setIcon(q->windowIcon());
}

QString QWidget::windowIconText() const
{
    QWidgetPrivate * const d = d_func();
    return (d->extra && d->extra->topextra) ? d->extra->topextra->iconText : QString();
}

QString QWidget::windowFilePath() const
{
    QWidgetPrivate * const d = d_func();
    return (d->extra && d->extra->topextra) ? d->extra->topextra->filePath : QString();
}

void QWidget::setWindowFilePath(const QString &filePath)
{
    if (filePath == windowFilePath())
        return;

    QWidgetPrivate * const d = d_func();

    d->createTLExtra();
    d->extra->topextra->filePath = filePath;
    d->setWindowFilePath_helper(filePath);
}

void QWidgetPrivate::setWindowFilePath_helper(const QString &filePath)
{
    if (extra->topextra && extra->topextra->caption.isEmpty()) {
        QWidget* const q = q_func();
        Q_UNUSED(filePath);
        setWindowTitle_helper(q->windowTitle());
    }
}

void QWidgetPrivate::setWindowFilePath_sys(const QString &filePath)
{
    QWidget* const q = q_func();
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setFilePath(filePath);
}

QString QWidget::windowRole() const
{
    QWidgetPrivate * const d = d_func();
    return (d->extra && d->extra->topextra) ? d->extra->topextra->role : QString();
}

void QWidget::setWindowRole(const QString &role)
{
    QWidgetPrivate * const d = d_func();
    d->createTLExtra();
    d->topData()->role = role;
    if (windowHandle())
        QXcbWindowFunctions::setWmWindowRole(windowHandle(), role.toLatin1());
}

/*!
	oye
	比如QEditLine 父w有focus,但实际其作用的应该是其内的输入框
*/

void QWidget::setFocusProxy(QWidget * w)
{
    QWidgetPrivate * const d = d_func();
    if (!w && !d->extra)
        return;

    for (QWidget* fp  = w; fp; fp = fp->focusProxy()) {
        if (Q_UNLIKELY(fp == this)) {
            qWarning("QWidget: %s (%s) already in focus proxy chain", metaObject()->className(), objectName().toLocal8Bit().constData());
            return;
        }
    }

    d->createExtra();
    d->extra->focus_proxy = w;
}

QWidget * QWidget::focusProxy() const
{
    QWidgetPrivate * const d = d_func();
    return d->extra ? (QWidget *)d->extra->focus_proxy : 0;
}

// oye whether QApplication::focusWidget() refers to the widget.
bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
	
    if (QWidget *window = w->window()) {
        QWExtra *e = window->d_func()->extra;
        if (e && e->proxyWidget && e->proxyWidget->hasFocus() && window->focusWidget() == w)
            return true;
    }
    return (QApplication::focusWidget() == w);
}

void QWidget::setFocus(Qt::FocusReason reason)
{
    if (!isEnabled())
        return;

    QWidget *f = this;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if (QApplication::focusWidget() == f  )
        return;

#if QT_CONFIG(graphicsview)
    QWidget *previousProxyFocus = 0;
    if (QWExtra *topData = window()->d_func()->extra) {
        if (topData->proxyWidget && topData->proxyWidget->hasFocus()) {
            previousProxyFocus = topData->proxyWidget->widget()->focusWidget();
            if (previousProxyFocus && previousProxyFocus->focusProxy())
                previousProxyFocus = previousProxyFocus->focusProxy();
            if (previousProxyFocus == this && !topData->proxyWidget->d_func()->proxyIsGivingFocus)
                return;
        }
    }
#endif

#if QT_CONFIG(graphicsview)
    // Update proxy state
    if (QWExtra *topData = window()->d_func()->extra) {
        if (topData->proxyWidget && !topData->proxyWidget->hasFocus()) {
            f->d_func()->updateFocusChild();
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 1;
            topData->proxyWidget->setFocus(reason);
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 0;
        }
    }
#endif

    if (f->isActiveWindow()) {
        QWidget *prev = QApplicationPrivate::focus_widget;
        if (prev) {
            if (reason != Qt::PopupFocusReason && reason != Qt::MenuBarFocusReason
                && prev->testAttribute(Qt::WA_InputMethodEnabled)) {
                QGuiApplication::inputMethod()->commit();
            }

            if (reason != Qt::NoFocusReason) {
                QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange, reason);
                QApplication::sendEvent(prev, &focusAboutToChange);
            }
        }

        f->d_func()->updateFocusChild();

        QApplicationPrivate::setFocusWidget(f, reason);

#if QT_CONFIG(graphicsview)
        if (QWExtra *topData = window()->d_func()->extra) {
            if (topData->proxyWidget) {
                if (previousProxyFocus && previousProxyFocus != f) {
                    // Send event to self
                    QFocusEvent event(QEvent::FocusOut, reason);
                    QPointer<QWidget> that = previousProxyFocus;
                    QApplication::sendEvent(previousProxyFocus, &event);
                    if (that)
                        QApplication::sendEvent(that->style(), &event);
                }
                if (!isHidden()) {
#if QT_CONFIG(graphicsview)
                    // Update proxy state
                    if (QWExtra *topData = window()->d_func()->extra)
                        if (topData->proxyWidget && topData->proxyWidget->hasFocus())
                            topData->proxyWidget->d_func()->updateProxyInputMethodAcceptanceFromWidget();
#endif
                    // Send event to self
                    QFocusEvent event(QEvent::FocusIn, reason);
                    QPointer<QWidget> that = f;
                    QApplication::sendEvent(f, &event);
                    if (that)
                        QApplication::sendEvent(that->style(), &event);
                }
            }
        }
#endif
    } else {
        f->d_func()->updateFocusChild();
    }
}

void QWidgetPrivate::setFocus_sys()
{
    QWidget* const q = q_func();
    // Embedded native widget may have taken the focus; get it back to toplevel if that is the case
    const QWidget *topLevel = q->window();
    if (topLevel->windowType() != Qt::Popup) {
        if (QWindow *nativeWindow = q->window()->windowHandle()) {
            if (nativeWindow != QGuiApplication::focusWindow()
                && q->testAttribute(Qt::WA_WState_Created)) {
                nativeWindow->requestActivate();
            }
        }
    }
}

// updates focus_child on parent widgets to point into this widget
void QWidgetPrivate::updateFocusChild()
{
    QWidget* const q = q_func();

    QWidget *w = q;
    if (q->isHidden()) {
        while (w && w->isHidden()) {
            w->d_func()->focus_child = q;
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    } else {
        while (w) {
            w->d_func()->focus_child = q;
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    }

    if (QTLWExtra *extra = q->window()->d_func()->maybeTopData()) {
        if (extra->window)
            emit extra->window->focusObjectChanged(q);
    }
}

void QWidget::clearFocus()
{
    if (hasFocus()) {
        if (testAttribute(Qt::WA_InputMethodEnabled))
            QGuiApplication::inputMethod()->commit();

        QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange);
        QApplication::sendEvent(this, &focusAboutToChange);
    }

    QWidget *w = this;
    while (w) {
        // Just like setFocus(), we update (clear) the focus_child of our parents
        if (w->d_func()->focus_child == this)
            w->d_func()->focus_child = 0;
        w = w->parentWidget();
    }

    // Since we've unconditionally cleared the focus_child of our parents, we need
    // to report this to the rest of Qt. Note that the focus_child is not the same
    // thing as the application's focusWidget, which is why this piece of code is
    // not inside the hasFocus() block below.
    if (QTLWExtra *extra = window()->d_func()->maybeTopData()) {
        if (extra->window)
            emit extra->window->focusObjectChanged(extra->window->focusObject());
    }

#if QT_CONFIG(graphicsview)
    QWExtra *topData = d_func()->extra;
    if (topData && topData->proxyWidget)
        topData->proxyWidget->clearFocus();
#endif

    if (hasFocus()) {
        // Update proxy state
        QApplicationPrivate::setFocusWidget(0, Qt::OtherFocusReason);
    }
}


bool QWidget::focusNextPrevChild(bool next)
{
    QWidget* p = parentWidget();
    bool isSubWindow = (windowType() == Qt::SubWindow);
    if (!isWindow() && !isSubWindow && p)
        return p->focusNextPrevChild(next);
#if QT_CONFIG(graphicsview)
    QWidgetPrivate * const d = d_func();
    if (d->extra && d->extra->proxyWidget)
        return d->extra->proxyWidget->focusNextPrevChild(next);
#endif

    bool wrappingOccurred = false;
    QWidget *w = QApplicationPrivate::focusNextPrevChild_helper(this, next,
                                                                &wrappingOccurred);
    if (!w) return false;

    Qt::FocusReason reason = next ? Qt::TabFocusReason : Qt::BacktabFocusReason;

    /* If we are about to wrap the focus chain, give the platform
     * implementation a chance to alter the wrapping behavior.  This is
     * especially needed when the window is embedded in a window created by
     * another process.
     */
    if (wrappingOccurred) {
        QWindow *window = windowHandle();
        if (window != 0) {
            QWindowPrivate *winp = qt_window_private(window);

            if (winp->platformWindow != 0) {
                QFocusEvent event(QEvent::FocusIn, reason);
                event.ignore();
                winp->platformWindow->windowEvent(&event);
                if (event.isAccepted()) return true;
            }
        }
    }

    w->setFocus(reason);
    return true;
}

/*!
    Returns the last child of this widget that setFocus had been
    called on.  For top level widgets this is the widget that will get
    focus in case this window gets activated

    This is not the same as QApplication::focusWidget(), which returns
    the focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    return const_cast<QWidget *>(d_func()->focus_child);
}

/*!
    Returns the next widget in this widget's focus chain.

    \sa previousInFocusChain()
*/
QWidget *QWidget::nextInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_next);
}

/*!
    \brief The previousInFocusChain function returns the previous
    widget in this widget's focus chain.

    \sa nextInFocusChain()

    \since 4.6
*/
QWidget *QWidget::previousInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_prev);
}


bool QWidget::isActiveWindow() const
{
    QWidget *tlw = window();
    if(tlw == QApplication::activeWindow() || (isVisible() && (tlw->windowType() == Qt::Popup)))
        return true;

#if QT_CONFIG(graphicsview)
    if (QWExtra *tlwExtra = tlw->d_func()->extra) {
        if (isVisible() && tlwExtra->proxyWidget)
            return tlwExtra->proxyWidget->isActiveWindow();
    }
#endif

    if(style()->styleHint(QStyle::SH_Widget_ShareActivation, 0, this)) {
        if(tlw->windowType() == Qt::Tool &&
           !tlw->isModal() &&
           (!tlw->parentWidget() || tlw->parentWidget()->isActiveWindow()))
           return true;
        QWidget *w = QApplication::activeWindow();
        while(w && tlw->windowType() == Qt::Tool &&
              !w->isModal() && w->parentWidget()) {
            w = w->parentWidget()->window();
            if(w == tlw)
                return true;
        }
    }

    // Check for an active window container
    if (QWindow *ww = QGuiApplication::focusWindow()) {
        while (ww) {
            QWidgetWindow *qww = qobject_cast<QWidgetWindow *>(ww);
            QWindowContainer *qwc = qww ? qobject_cast<QWindowContainer *>(qww->widget()) : 0;
            if (qwc && qwc->topLevelWidget() == tlw)
                return true;
            ww = ww->parent();
        }
    }

    // Check if platform adaptation thinks the window is active. This is necessary for
    // example in case of ActiveQt servers that are embedded into another application.
    // Those are separate processes that are not part of the parent application Qt window/widget
    // hierarchy, so they need to rely on native methods to determine if they are part of the
    // active window.
    if (const QWindow *w = tlw->windowHandle()) {
        if (w->handle())
            return w->handle()->isActive();
    }

    return false;
}


void QWidget::setTabOrder(QWidget* first, QWidget *second)
{
    if (!first || !second || first->focusPolicy() == Qt::NoFocus || second->focusPolicy() == Qt::NoFocus)
        return;

    if (Q_UNLIKELY(first->window() != second->window())) {
        qWarning("QWidget::setTabOrder: 'first' and 'second' must be in the same window");
        return;
    }

    QWidget *fp = first->focusProxy();
    if (fp) {
        // If first is redirected, set first to the last child of first
        // that can take keyboard focus so that second is inserted after
        // that last child, and the focus order within first is (more
        // likely to be) preserved.
        QList<QWidget *> l = first->findChildren<QWidget *>();
        for (int i = l.size()-1; i >= 0; --i) {
            QWidget * next = l.at(i);
            if (next->window() == fp->window()) {
                fp = next;
                if (fp->focusPolicy() != Qt::NoFocus)
                    break;
            }
        }
        first = fp;
    }

    if (fp == second)
        return;

    if (QWidget *sp = second->focusProxy())
        second = sp;

//    QWidget *fp = first->d_func()->focus_prev;
    QWidget *fn = first->d_func()->focus_next;

    if (fn == second || first == second)
        return;

    QWidget *sp = second->d_func()->focus_prev;
    QWidget *sn = second->d_func()->focus_next;

    fn->d_func()->focus_prev = second;
    first->d_func()->focus_next = second;

    second->d_func()->focus_next = fn;
    second->d_func()->focus_prev = first;

    sp->d_func()->focus_next = sn;
    sn->d_func()->focus_prev = sp;


    Q_ASSERT(first->d_func()->focus_next->d_func()->focus_prev == first);
    Q_ASSERT(first->d_func()->focus_prev->d_func()->focus_next == first);

    Q_ASSERT(second->d_func()->focus_next->d_func()->focus_prev == second);
    Q_ASSERT(second->d_func()->focus_prev->d_func()->focus_next == second);
}

/*!\internal

  Moves the relevant subwidgets of this widget from the  oldtlw's
  tab chain to that of the new parent, if there's anything to move and
  we're really moving

  This function is called from QWidget::reparent() *after* the widget
  has been reparented.

  \sa reparent()
*/
void QWidgetPrivate::reparentFocusWidgets(QWidget * oldtlw)
{
    QWidget* const q = q_func();
    if (oldtlw == q->window())
        return; // nothing to do

    if(focus_child)
        focus_child->clearFocus();

    // separate the focus chain into new (children of myself) and old (the rest)
    QWidget *firstOld = 0;
    //QWidget *firstNew = q; //invariant
    QWidget *o = 0; // last in the old list
    QWidget *n = q; // last in the new list

    bool prevWasNew = true;
    QWidget *w = focus_next;

    //Note: for efficiency, we do not maintain the list invariant inside the loop
    //we append items to the relevant list, and we optimize by not changing pointers
    //when subsequent items are going into the same list.
    while (w  != q) {
        bool currentIsNew =  q->isAncestorOf(w);
        if (currentIsNew) {
            if (!prevWasNew) {
                //prev was old -- append to new list
                n->d_func()->focus_next = w;
                w->d_func()->focus_prev = n;
            }
            n = w;
        } else {
            if (prevWasNew) {
                //prev was new -- append to old list, if there is one
                if (o) {
                    o->d_func()->focus_next = w;
                    w->d_func()->focus_prev = o;
                } else {
                    // "create" the old list
                    firstOld = w;
                }
            }
            o = w;
        }
        w = w->d_func()->focus_next;
        prevWasNew = currentIsNew;
    }

    //repair the old list:
    if (firstOld) {
        o->d_func()->focus_next = firstOld;
        firstOld->d_func()->focus_prev = o;
    }

    if (!q->isWindow()) {
        QWidget *topLevel = q->window();
        //insert new chain into toplevel's chain

        QWidget *prev = topLevel->d_func()->focus_prev;

        topLevel->d_func()->focus_prev = n;
        prev->d_func()->focus_next = q;

        focus_prev = prev;
        n->d_func()->focus_next = topLevel;
    } else {
        //repair the new list
            n->d_func()->focus_next = q;
            focus_prev = n;
    }

}

/*!\internal

  Measures the shortest distance from a point to a rect.

  This function is called from QDesktopwidget::screen(QPoint) to find the
  closest screen for a point.
  In directional KeypadNavigation, it is called to find the closest
  widget to the current focus widget center.
*/
int QWidgetPrivate::pointToRect(const QPoint &p, const QRect &r)
{
    int dx = 0;
    int dy = 0;
    if (p.x() < r.left())
        dx = r.left() - p.x();
    else if (p.x() > r.right())
        dx = p.x() - r.right();
    if (p.y() < r.top())
        dy = r.top() - p.y();
    else if (p.y() > r.bottom())
        dy = p.y() - r.bottom();
    return dx + dy;
}


QSize QWidget::frameSize() const
{
    QWidgetPrivate * const d = d_func();
    if (isWindow() && !(windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QSize(data->crect.width() + fs.left() + fs.right(),
                      data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect.size();
}

void QWidget::move(const QPoint &p)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_Moved);
    if (testAttribute(Qt::WA_WState_Created)) {
        if (isWindow())
            d->topData()->posIncludesFrame = false;
        d->setGeometry_sys(p.x() + geometry().x() - QWidget::x(),
                       p.y() + geometry().y() - QWidget::y(),
                       width(), height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        // no frame yet: see also QWidgetPrivate::fixPosIncludesFrame(), QWindowPrivate::PositionPolicy.
        if (isWindow())
            d->topData()->posIncludesFrame = true;
        data->crect.moveTopLeft(p); // no frame yet
        setAttribute(Qt::WA_PendingMoveEvent);
    }

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasMoved(this);
}

// move() was invoked with Qt::WA_WState_Created not set (frame geometry
// unknown), that is, crect has a position including the frame.
// If we can determine the frame strut, fix that and clear the flag.
void QWidgetPrivate::fixPosIncludesFrame()
{
    QWidget* const q = q_func();
    if (QTLWExtra *te = maybeTopData()) {
        if (te->posIncludesFrame) {
            // For Qt::WA_DontShowOnScreen, assume a frame of 0 (for
            // example, in QGraphicsProxyWidget).
            if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
                te->posIncludesFrame = 0;
            } else {
                if (q->windowHandle()) {
                    updateFrameStrut();
                    if (!q->data->fstrut_dirty) {
                        data.crect.translate(te->frameStrut.x(), te->frameStrut.y());
                        te->posIncludesFrame = 0;
                    }
                } // windowHandle()
            } // !WA_DontShowOnScreen
        } // posIncludesFrame
    } // QTLWExtra
}

/*! \fn void QWidget::resize(int w, int h)
    \overload

    This corresponds to resize(QSize( w,  h)).
*/

void QWidget::resize(const QSize &s)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_Resized);
    if (testAttribute(Qt::WA_WState_Created)) {
        d->fixPosIncludesFrame();
        d->setGeometry_sys(geometry().x(), geometry().y(), s.width(), s.height(), false);
        d->setDirtyOpaqueRegion();
    } else {
        data->crect.setSize(s.boundedTo(maximumSize()).expandedTo(minimumSize()));
        setAttribute(Qt::WA_PendingResizeEvent);
    }
}

void QWidget::setGeometry(const QRect &r)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_Resized);
    setAttribute(Qt::WA_Moved);
    if (isWindow())
        d->topData()->posIncludesFrame = 0;
    if (testAttribute(Qt::WA_WState_Created)) {
        d->setGeometry_sys(r.x(), r.y(), r.width(), r.height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        data->crect.setTopLeft(r.topLeft());
        data->crect.setSize(r.size().boundedTo(maximumSize()).expandedTo(minimumSize()));
        setAttribute(Qt::WA_PendingMoveEvent);
        setAttribute(Qt::WA_PendingResizeEvent);
    }

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasMoved(this);
}

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    QWidget* const q = q_func();
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }

    if (q->isWindow() && q->windowHandle()) {
        QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
        if (!integration->hasCapability(QPlatformIntegration::NonFullScreenWindows)) {
            x = 0;
            y = 0;
            w = q->windowHandle()->width();
            h = q->windowHandle()->height();
        }
    }

    QPoint oldp = q->geometry().topLeft();
    QSize olds = q->size();
    QRect r(x, y, w, h);

    bool isResize = olds != r.size();
    if (!isMove)
        isMove = oldp != r.topLeft();


    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if (r.size() == olds && oldp == r.topLeft())
        return;

    if (!data.in_set_window_state) {
        q->data->window_state &= ~Qt::WindowMaximized;
        q->data->window_state &= ~Qt::WindowFullScreen;
        if (q->isWindow())
            topData()->normalGeometry = QRect(0, 0, -1, -1);
    }

    QPoint oldPos = q->pos();
    data.crect = r;

    bool needsShow = false;

    if (q->isWindow() || q->windowHandle()) {
        if (!(data.window_state & Qt::WindowFullScreen) && (w == 0 || h == 0)) {
            q->setAttribute(Qt::WA_OutsideWSRange, true);
            if (q->isVisible())
                hide_sys();
            data.crect = QRect(x, y, w, h);
        } else if (q->testAttribute(Qt::WA_OutsideWSRange)) {
            q->setAttribute(Qt::WA_OutsideWSRange, false);
            needsShow = true;
        }
    }

    if (q->isVisible()) {
        if (!q->testAttribute(Qt::WA_DontShowOnScreen) && !q->testAttribute(Qt::WA_OutsideWSRange)) {
            if (QWindow *win = q->windowHandle()) {
                if (q->isWindow()) {
                    if (isResize && !isMove)
                        win->resize(w, h);
                    else if (isMove && !isResize)
                        win->setPosition(x, y);
                    else
                        win->setGeometry(q->geometry());
                } else {
                    QPoint posInNativeParent =  q->mapTo(q->nativeParentWidget(),QPoint());
                    win->setGeometry(QRect(posInNativeParent,r.size()));
                }

                if (needsShow)
                    show_sys();
            }

            if (!q->isWindow()) {
                if (renderToTexture) {
                    QRegion updateRegion(q->geometry());
                    updateRegion += QRect(oldPos, olds);
                    q->parentWidget()->d_func()->invalidateBuffer(updateRegion);
                } else if (isMove && !isResize) {
                    moveRect(QRect(oldPos, olds), x - oldPos.x(), y - oldPos.y());
                } else {
                    invalidateBuffer_resizeHelper(oldPos, olds);
                }
            }
        }

        if (isMove) {
            QMoveEvent e(q->pos(), oldPos);
            QApplication::sendEvent(q, &e);
        }
        if (isResize) {
            QResizeEvent e(r.size(), olds);
            QApplication::sendEvent(q, &e);
            if (q->windowHandle())
                q->update();
        }
    } else { // not visible
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

}


QByteArray QWidget::saveGeometry() const
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    const quint32 magicNumber = 0x1D9D0CB;
    // Version history:
    // - Qt 4.2 - 4.8.6, 5.0 - 5.3    : Version 1.0
    // - Qt 4.8.6 - today, 5.4 - today: Version 2.0, save screen width in addition to check for high DPI scaling.
    quint16 majorVersion = 2;
    quint16 minorVersion = 0;
    const int screenNumber = QApplication::desktop()->screenNumber(this);
    stream << magicNumber
           << majorVersion
           << minorVersion
           << frameGeometry()
           << normalGeometry()
#endif
           << qint32(screenNumber)
           << quint8(windowState() & Qt::WindowMaximized)
           << quint8(windowState() & Qt::WindowFullScreen)
           << qint32(QApplication::desktop()->screenGeometry(screenNumber).width()); // 1.1 onwards
    return array;
}


bool QWidget::restoreGeometry(const QByteArray &geometry)
{
    if (geometry.size() < 4)
        return false;
    QDataStream stream(geometry);
    stream.setVersion(QDataStream::Qt_4_0);

    const quint32 magicNumber = 0x1D9D0CB;
    quint32 storedMagicNumber;
    stream >> storedMagicNumber;
    if (storedMagicNumber != magicNumber)
        return false;

    const quint16 currentMajorVersion = 2;
    quint16 majorVersion = 0;
    quint16 minorVersion = 0;

    stream >> majorVersion >> minorVersion;

    if (majorVersion > currentMajorVersion)
        return false;
    // (Allow all minor versions.)

    QRect restoredFrameGeometry;
     QRect restoredNormalGeometry;
    qint32 restoredScreenNumber;
    quint8 maximized;
    quint8 fullScreen;
    qint32 restoredScreenWidth = 0;

    stream >> restoredFrameGeometry
           >> restoredNormalGeometry
           >> restoredScreenNumber
           >> maximized
           >> fullScreen;

    if (majorVersion > 1)
        stream >> restoredScreenWidth;

    const QDesktopWidget * const desktop = QApplication::desktop();
    if (restoredScreenNumber >= desktop->numScreens())
        restoredScreenNumber = desktop->primaryScreen();
    const qreal screenWidthF = qreal(desktop->screenGeometry(restoredScreenNumber).width());
    // Sanity check bailing out when large variations of screen sizes occur due to
    // high DPI scaling or different levels of DPI awareness.
    if (restoredScreenWidth) {
        const qreal factor = qreal(restoredScreenWidth) / screenWidthF;
        if (factor < 0.8 || factor > 1.25)
            return false;
    } else {
        // Saved by Qt 5.3 and earlier, try to prevent too large windows
        // unless the size will be adapted by maximized or fullscreen.
        if (!maximized && !fullScreen && qreal(restoredFrameGeometry.width()) / screenWidthF > 1.5)
            return false;
    }

    const int frameHeight = 20;
    if (!restoredFrameGeometry.isValid())
        restoredFrameGeometry = QRect(QPoint(0,0), sizeHint());

    if (!restoredNormalGeometry.isValid())
        restoredNormalGeometry = QRect(QPoint(0, frameHeight), sizeHint());
    if (!restoredNormalGeometry.isValid()) {
        // use the widget's adjustedSize if the sizeHint() doesn't help
        restoredNormalGeometry.setSize(restoredNormalGeometry
                                       .size()
                                       .expandedTo(d_func()->adjustedSize()));
    }

    const QRect availableGeometry = desktop->availableGeometry(restoredScreenNumber);

    // Modify the restored geometry if we are about to restore to coordinates
    // that would make the window "lost". This happens if:
    // - The restored geometry is completely oustside the available geometry
    // - The title bar is outside the available geometry.
    // - (Mac only) The window is higher than the available geometry. It must
    //   be possible to bring the size grip on screen by moving the window.
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    restoredFrameGeometry.setHeight(qMin(restoredFrameGeometry.height(), availableGeometry.height()));
    restoredNormalGeometry.setHeight(qMin(restoredNormalGeometry.height(), availableGeometry.height() - frameHeight));
#endif

    if (!restoredFrameGeometry.intersects(availableGeometry)) {
        restoredFrameGeometry.moveBottom(qMin(restoredFrameGeometry.bottom(), availableGeometry.bottom()));
        restoredFrameGeometry.moveLeft(qMax(restoredFrameGeometry.left(), availableGeometry.left()));
        restoredFrameGeometry.moveRight(qMin(restoredFrameGeometry.right(), availableGeometry.right()));
    }
    restoredFrameGeometry.moveTop(qMax(restoredFrameGeometry.top(), availableGeometry.top()));

    if (!restoredNormalGeometry.intersects(availableGeometry)) {
        restoredNormalGeometry.moveBottom(qMin(restoredNormalGeometry.bottom(), availableGeometry.bottom()));
        restoredNormalGeometry.moveLeft(qMax(restoredNormalGeometry.left(), availableGeometry.left()));
        restoredNormalGeometry.moveRight(qMin(restoredNormalGeometry.right(), availableGeometry.right()));
    }
    restoredNormalGeometry.moveTop(qMax(restoredNormalGeometry.top(), availableGeometry.top() + frameHeight));

    if (maximized || fullScreen) {
        // set geometry before setting the window state to make
        // sure the window is maximized to the right screen.
        Qt::WindowStates ws = windowState();
#ifndef Q_OS_WIN
        setGeometry(restoredNormalGeometry);
#else
        if (ws & Qt::WindowFullScreen) {
            // Full screen is not a real window state on Windows.
            move(availableGeometry.topLeft());
        } else if (ws & Qt::WindowMaximized) {
            // Setting a geometry on an already maximized window causes this to be
            // restored into a broken, half-maximized state, non-resizable state (QTBUG-4397).
            // Move the window in normal state if needed.
            if (restoredScreenNumber != desktop->screenNumber(this)) {
                setWindowState(Qt::WindowNoState);
                setGeometry(restoredNormalGeometry);
            }
        } else {
            setGeometry(restoredNormalGeometry);
        }
#endif // Q_OS_WIN
        if (maximized)
            ws |= Qt::WindowMaximized;
        if (fullScreen)
            ws |= Qt::WindowFullScreen;
       setWindowState(ws);
       d_func()->topData()->normalGeometry = restoredNormalGeometry;
    } else {
        QPoint offset;
#if 0 // Used to be included in Qt4 for Q_WS_X11
        if (isFullScreen())
            offset = d_func()->topData()->fullScreenOffset;
#endif
        setWindowState(windowState() & ~(Qt::WindowMaximized | Qt::WindowFullScreen));
        move(restoredFrameGeometry.topLeft() + offset);
        resize(restoredNormalGeometry.size());
    }
    return true;
}

void QWidget::setContentsMargins(int left, int top, int right, int bottom)
{
    QWidgetPrivate * const d = d_func();
    if (left == d->leftmargin && top == d->topmargin
         && right == d->rightmargin && bottom == d->bottommargin)
        return;
    d->leftmargin = left;
    d->topmargin = top;
    d->rightmargin = right;
    d->bottommargin = bottom;

    d->updateContentsRect();
}


void QWidget::setContentsMargins(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(),
                       margins.right(), margins.bottom());
}

void QWidgetPrivate::updateContentsRect()
{
    QWidget* const q = q_func();

    if (layout)
        layout->update(); //force activate; will do updateGeometry
    else
        q->updateGeometry();

    if (q->isVisible()) {
        q->update();
        QResizeEvent e(q->data->crect.size(), q->data->crect.size());
        QApplication::sendEvent(q, &e);
    } else {
        q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

    QEvent e(QEvent::ContentsRectChange);
    QApplication::sendEvent(q, &e);
}


void QWidget::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    QMargins m = contentsMargins();
    if (left)
        *left = m.left();
    if (top)
        *top = m.top();
    if (right)
        *right = m.right();
    if (bottom)
        *bottom = m.bottom();
}

// FIXME: Move to qmargins.h for next minor Qt release
QMargins operator|(const QMargins &m1, const QMargins &m2)
{
    return QMargins(qMax(m1.left(), m2.left()), qMax(m1.top(), m2.top()),
        qMax(m1.right(), m2.right()), qMax(m1.bottom(), m2.bottom()));
}


QMargins QWidget::contentsMargins() const
{
    QWidgetPrivate * const d = d_func();
    QMargins userMargins(d->leftmargin, d->topmargin, d->rightmargin, d->bottommargin);
    return testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea) ?
        userMargins | d->safeAreaMargins() : userMargins;
}


QRect QWidget::contentsRect() const
{
    return rect() - contentsMargins();
}

QMargins QWidgetPrivate::safeAreaMargins() const
{
    QWidget* const q = q_func();
    QWidget *nativeWidget = q->window();
    if (!nativeWidget->windowHandle())
        return QMargins();

    QPlatformWindow *platformWindow = nativeWidget->windowHandle()->handle();
    if (!platformWindow)
        return QMargins();

    QMargins safeAreaMargins = platformWindow->safeAreaMargins();

    if (!q->isWindow()) {
        // In theory the native parent widget already has a contents rect reflecting
        // the safe area of that widget, but we can't be sure that the widget or child
        // widgets of that widget have respected the contents rect when setting their
        // geometry, so we need to manually compute the safe area.

        // Unless the native widget doesn't have any margins, in which case there's
        // nothing for us to compute.
        if (safeAreaMargins.isNull())
            return QMargins();

        // Or, if one of our ancestors are in a layout that does not have WA_LayoutOnEntireRect
        // set, then we know that the layout has already taken care of placing us inside the
        // safe area, by taking the contents rect of its parent widget into account.
        const QWidget *assumedSafeWidget = nullptr;
        for (const QWidget *w = q; w != nativeWidget; w = w->parentWidget()) {
            QWidget *parentWidget = w->parentWidget();
            if (parentWidget->testAttribute(Qt::WA_LayoutOnEntireRect))
                continue; // Layout not going to help us

            QLayout *layout = parentWidget->layout();
            if (!layout)
                continue;

            if (layout->geometry().isNull())
                continue; // Layout hasn't been activated yet

            if (layout->indexOf(const_cast<QWidget *>(w)) < 0)
                continue; // Widget is not in layout

            assumedSafeWidget = w;
            break;
        }

#if !defined(QT_DEBUG)
        if (assumedSafeWidget) {
            // We found a layout that we assume will take care of keeping us within the safe area
            // For debug builds we still map the safe area using the fallback logic, so that we
            // can detect any misbehaving layouts.
            return QMargins();
        }
#endif

        // In all other cases we need to map the safe area of the native parent to the widget.
        // This depends on the widget being positioned and sized already, which means the initial
        // layout will be wrong, but the layout will then adjust itself.
        QPoint topLeftMargins = q->mapFrom(nativeWidget, QPoint(safeAreaMargins.left(), safeAreaMargins.top()));
        QRect widgetRect = q->isVisible() ? q->visibleRegion().boundingRect() : q->rect();
        QPoint bottomRightMargins = widgetRect.bottomRight() - q->mapFrom(nativeWidget,
            nativeWidget->rect().bottomRight() - QPoint(safeAreaMargins.right(), safeAreaMargins.bottom()));

        // Margins should never be negative
        safeAreaMargins = QMargins(qMax(0, topLeftMargins.x()), qMax(0, topLeftMargins.y()),
            qMax(0, bottomRightMargins.x()), qMax(0, bottomRightMargins.y()));

        if (!safeAreaMargins.isNull() && assumedSafeWidget) {
            QLayout *layout = assumedSafeWidget->parentWidget()->layout();
            qWarning() << layout << "is laying out" << assumedSafeWidget
                << "outside of the contents rect of" << layout->parentWidget();
            return QMargins(); // Return empty margin to visually highlight the error
        }
    }

    return safeAreaMargins;
}

Qt::ContextMenuPolicy QWidget::contextMenuPolicy() const
{
    return (Qt::ContextMenuPolicy)data->context_menu_policy;
}

void QWidget::setContextMenuPolicy(Qt::ContextMenuPolicy policy)
{
    data->context_menu_policy = (uint) policy;
}

Qt::FocusPolicy QWidget::focusPolicy() const
{
    return (Qt::FocusPolicy)data->focus_policy;
}

void QWidget::setFocusPolicy(Qt::FocusPolicy policy)
{
    data->focus_policy = (uint) policy;
    QWidgetPrivate * const d = d_func();
    if (d->extra && d->extra->focus_proxy)
        d->extra->focus_proxy->setFocusPolicy(policy);
}

void QWidget::setUpdatesEnabled(bool enable)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_ForceUpdatesDisabled, !enable);
    d->setUpdatesEnabled_helper(enable);
}


void QWidget::show()
{
    Qt::WindowState defaultState = QGuiApplicationPrivate::platformIntegration()->defaultWindowState(data->window_flags);
    if (defaultState == Qt::WindowFullScreen)
        showFullScreen();
    else if (defaultState == Qt::WindowMaximized)
        showMaximized();
    else
        setVisible(true); // FIXME: Why not showNormal(), like QWindow::show()?
}


void QWidgetPrivate::show_recursive()
{
    QWidget* const q = q_func();
    // polish if necessary

    if (!q->testAttribute(Qt::WA_WState_Created))
        createRecursively();
    q->ensurePolished();

    if (!q->isWindow() && q->parentWidget()->d_func()->layout && !q->parentWidget()->data->in_show)
        q->parentWidget()->d_func()->layout->activate();
    // activate our layout before we and our children become visible
    if (layout)
        layout->activate();

    show_helper();
}

void QWidgetPrivate::sendPendingMoveAndResizeEvents(bool recursive, bool disableUpdates)
{
    QWidget* const q = q_func();

    disableUpdates = disableUpdates && q->updatesEnabled();
    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled);

    if (q->testAttribute(Qt::WA_PendingMoveEvent)) {
        QMoveEvent e(data.crect.topLeft(), data.crect.topLeft());
        QApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingMoveEvent, false);
    }

    if (q->testAttribute(Qt::WA_PendingResizeEvent)) {
        QResizeEvent e(data.crect.size(), QSize());
        QApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingResizeEvent, false);
    }

    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled, false);

    if (!recursive)
        return;

    for (int i = 0; i < children.size(); ++i) {
        if (QWidget *child = qobject_cast<QWidget *>(children.at(i)))
            child->d_func()->sendPendingMoveAndResizeEvents(recursive, disableUpdates);
    }
}

void QWidgetPrivate::activateChildLayoutsRecursively()
{
    sendPendingMoveAndResizeEvents(false, true);

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isHidden() || child->isWindow())
            continue;

        child->ensurePolished();

        // Activate child's layout
        QWidgetPrivate *childPrivate = child->d_func();
        if (childPrivate->layout)
            childPrivate->layout->activate();

        // Pretend we're visible.
        const bool wasVisible = child->isVisible();
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible);

        // Do the same for all my children.
        childPrivate->activateChildLayoutsRecursively();

        // We're not cheating anymore.
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible, false);
    }
}

void QWidgetPrivate::show_helper()
{
    QWidget* const q = q_func();
    data.in_show = true; // qws optimization
    // make sure we receive pending move and resize events
    sendPendingMoveAndResizeEvents();

    // become visible before showing all children
    q->setAttribute(Qt::WA_WState_Visible);

    // finally show all children recursively
    showChildren(false);



    const bool isWindow = q->isWindow();
#if QT_CONFIG(graphicsview)
    bool isEmbedded = isWindow && q->graphicsProxyWidget() != Q_NULLPTR;
#else
    bool isEmbedded = false;
#endif

    // popup handling: new popups and tools need to be raised, and
    // existing popups must be closed. Also propagate the current
    // windows's KeyboardFocusChange status.
    if (isWindow && !isEmbedded) {
        if ((q->windowType() == Qt::Tool) || (q->windowType() == Qt::Popup) || q->windowType() == Qt::ToolTip) {
            q->raise();
            if (q->parentWidget() && q->parentWidget()->window()->testAttribute(Qt::WA_KeyboardFocusChange))
                q->setAttribute(Qt::WA_KeyboardFocusChange);
        } else {
            while (QApplication::activePopupWidget()) {
                if (!QApplication::activePopupWidget()->close())
                    break;
            }
        }
    }

    // Automatic embedding of child windows of widgets already embedded into
    // QGraphicsProxyWidget when they are shown the first time.
#if QT_CONFIG(graphicsview)
    if (isWindow) {
        if (!isEmbedded && !bypassGraphicsProxyWidget(q)) {
            QGraphicsProxyWidget *ancestorProxy = nearestGraphicsProxyWidget(q->parentWidget());
            if (ancestorProxy) {
                isEmbedded = true;
                ancestorProxy->d_func()->embedSubWindow(q);
            }
        }
    }
#else
    Q_UNUSED(isEmbedded);
#endif

    // On Windows, show the popup now so that our own focus handling
    // stores the correct old focus widget even if it's stolen in the
    // showevent
#if 0 /* Used to be included in Qt4 for Q_WS_WIN */ || 0 /* Used to be included in Qt4 for Q_WS_MAC */
    if (!isEmbedded && q->windowType() == Qt::Popup)
        qApp->d_func()->openPopup(q);
#endif

    // send the show event before showing the window
    QShowEvent showEvent;
    QApplication::sendEvent(q, &showEvent);

    show_sys();

    if (!isEmbedded && q->windowType() == Qt::Popup)
        qApp->d_func()->openPopup(q);

#ifndef QT_NO_ACCESSIBILITY
    if (q->windowType() != Qt::ToolTip) {    // Tooltips are read aloud twice in MS narrator.
        QAccessibleEvent event(q, QAccessible::ObjectShow);
        QAccessible::updateAccessibility(&event);
    }
#endif

    if (QApplicationPrivate::hidden_focus_widget == q) {
        QApplicationPrivate::hidden_focus_widget = 0;
        q->setFocus(Qt::OtherFocusReason);
    }

    // Process events when showing a Qt::SplashScreen widget before the event loop
    // is spinnning; otherwise it might not show up on particular platforms.
    // This makes QSplashScreen behave the same on all platforms.
    if (!qApp->d_func()->in_exec && q->windowType() == Qt::SplashScreen)
        QApplication::processEvents();

    data.in_show = false;  // reset qws optimization
}

void QWidgetPrivate::show_sys()
{
    QWidget* const q = q_func();

    QWindow *window = q->windowHandle();

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        q->setAttribute(Qt::WA_Mapped);
        // add our window the modal window list (native dialogs)
        if (window && q->isWindow()
#if QT_CONFIG(graphicsview)
            && (!extra || !extra->proxyWidget)
#endif
            && q->windowModality() != Qt::NonModal) {
            QGuiApplicationPrivate::showModalWindow(window);
        }
        return;
    }

    if (renderToTexture && !q->isWindow())
        QApplication::postEvent(q->parentWidget(), new QUpdateLaterEvent(q->geometry()));
    else
        QApplication::postEvent(q, new QUpdateLaterEvent(q->rect()));

    if ((!q->isWindow() && !q->testAttribute(Qt::WA_NativeWindow))
            || q->testAttribute(Qt::WA_OutsideWSRange)) {
        return;
    }

    if (window) {
        if (q->isWindow())
            fixPosIncludesFrame();
        QRect geomRect = q->geometry();
        if (!q->isWindow()) {
            QPoint topLeftOfWindow = q->mapTo(q->nativeParentWidget(),QPoint());
            geomRect.moveTopLeft(topLeftOfWindow);
        }
        const QRect windowRect = window->geometry();
        if (windowRect != geomRect) {
            if (q->testAttribute(Qt::WA_Moved)
                || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
                window->setGeometry(geomRect);
            else
                window->resize(geomRect.size());
        }

#ifndef QT_NO_CURSOR
        qt_qpa_set_cursor(q, false); // Needed in case cursor was set before show
#endif
        invalidateBuffer(q->rect());
        window->setVisible(true);
        // Was the window moved by the Window system or QPlatformWindow::initialGeometry() ?
        if (window->isTopLevel()) {
            const QPoint crectTopLeft = q->data->crect.topLeft();
            const QPoint windowTopLeft = window->geometry().topLeft();
            if (crectTopLeft == QPoint(0, 0) && windowTopLeft != crectTopLeft)
                q->data->crect.moveTopLeft(windowTopLeft);
        }
    }
}

void QWidget::hide()
{
    setVisible(false);
}

/*!\internal
 */
void QWidgetPrivate::hide_helper()
{
    QWidget* const q = q_func();

    bool isEmbedded = false;
#if QT_CONFIG(graphicsview)
    isEmbedded = q->isWindow() && !bypassGraphicsProxyWidget(q) && nearestGraphicsProxyWidget(q->parentWidget()) != 0;
#else
    Q_UNUSED(isEmbedded);
#endif

    if (!isEmbedded && (q->windowType() == Qt::Popup))
        qApp->d_func()->closePopup(q);

    q->setAttribute(Qt::WA_Mapped, false);
    hide_sys();

    bool wasVisible = q->testAttribute(Qt::WA_WState_Visible);

    if (wasVisible) {
        q->setAttribute(Qt::WA_WState_Visible, false);

    }

    QHideEvent hideEvent;
    QApplication::sendEvent(q, &hideEvent);
    hideChildren(false);

    // next bit tries to move the focus if the focus widget is now
    // hidden.
    if (wasVisible) {
        qApp->d_func()->sendSyntheticEnterLeave(q);
        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == q) {
                q->focusNextPrevChild(true);
                break;
            }
            fw = fw->parentWidget();
        }
    }

    if (QWidgetBackingStore *bs = maybeBackingStore())
        bs->removeDirtyWidget(q);

}

void QWidgetPrivate::hide_sys()
{
    QWidget* const q = q_func();

    QWindow *window = q->windowHandle();

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        q->setAttribute(Qt::WA_Mapped, false);
        // remove our window from the modal window list (native dialogs)
        if (window && q->isWindow()
#if QT_CONFIG(graphicsview)
            && (!extra || !extra->proxyWidget)
#endif
            && q->windowModality() != Qt::NonModal) {
            QGuiApplicationPrivate::hideModalWindow(window);
        }
        // do not return here, if window non-zero, we must hide it
    }

    deactivateWidgetCleanup();

    if (!q->isWindow()) {
        QWidget *p = q->parentWidget();
        if (p &&p->isVisible()) {
            if (renderToTexture)
                p->d_func()->invalidateBuffer(q->geometry());
            else
                invalidateBuffer(q->rect());
        }
    } else {
        invalidateBuffer(q->rect());
    }

    if (window)
        window->setVisible(false);
}




void QWidget::setVisible(bool visible)
{
    if (visible) { // show
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && !testAttribute(Qt::WA_WState_Hidden))
            return;

        QWidgetPrivate * const d = d_func();

        // Designer uses a trick to make grabWidget work without showing
        if (!isWindow() && parentWidget() && parentWidget()->isVisible()
            && !parentWidget()->testAttribute(Qt::WA_WState_Created))
            parentWidget()->window()->d_func()->createRecursively();

        //create toplevels but not children of non-visible parents
        QWidget *pw = parentWidget();
        if (!testAttribute(Qt::WA_WState_Created)
            && (isWindow() || pw->testAttribute(Qt::WA_WState_Created))) {
            create();
        }

        bool wasResized = testAttribute(Qt::WA_Resized);
        Qt::WindowStates initialWindowState = windowState();

        // polish if necessary
        ensurePolished();

        // remember that show was called explicitly
        setAttribute(Qt::WA_WState_ExplicitShowHide);
        // whether we need to inform the parent widget immediately
        bool needUpdateGeometry = !isWindow() && testAttribute(Qt::WA_WState_Hidden);
        // we are no longer hidden
        setAttribute(Qt::WA_WState_Hidden, false);

        if (needUpdateGeometry)
            d->updateGeometry_helper(true);

        // activate our layout before we and our children become visible
        if (d->layout)
            d->layout->activate();

        if (!isWindow()) {
            QWidget *parent = parentWidget();
            while (parent && parent->isVisible() && parent->d_func()->layout  && !parent->data->in_show) {
                parent->d_func()->layout->activate();
                if (parent->isWindow())
                    break;
                parent = parent->parentWidget();
            }
            if (parent)
                parent->d_func()->setDirtyOpaqueRegion();
        }

        // adjust size if necessary
        if (!wasResized
            && (isWindow() || !parentWidget()->d_func()->layout))  {
            if (isWindow()) {
                adjustSize();
                if (windowState() != initialWindowState)
                    setWindowState(initialWindowState);
            } else {
                adjustSize();
            }
            setAttribute(Qt::WA_Resized, false);
        }

        setAttribute(Qt::WA_KeyboardFocusChange, false);

        if (isWindow() || parentWidget()->isVisible()) {
            d->show_helper();

            qApp->d_func()->sendSyntheticEnterLeave(this);
        }

        QEvent showToParentEvent(QEvent::ShowToParent);
        QApplication::sendEvent(this, &showToParentEvent);
    } else { // hide
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
            return;
        if (QApplicationPrivate::hidden_focus_widget == this)
            QApplicationPrivate::hidden_focus_widget = 0;

        QWidgetPrivate * const d = d_func();

        // hw: The test on getOpaqueRegion() needs to be more intelligent
        // currently it doesn't work if the widget is hidden (the region will
        // be clipped). The real check should be testing the cached region
        // (and dirty flag) directly.
        if (!isWindow() && parentWidget()) // && !d->getOpaqueRegion().isEmpty())
            parentWidget()->d_func()->setDirtyOpaqueRegion();

        setAttribute(Qt::WA_WState_Hidden);
        setAttribute(Qt::WA_WState_ExplicitShowHide);
        if (testAttribute(Qt::WA_WState_Created))
            d->hide_helper();

        // invalidate layout similar to updateGeometry()
        if (!isWindow() && parentWidget()) {
            if (parentWidget()->d_func()->layout)
                parentWidget()->d_func()->layout->invalidate();
            else if (parentWidget()->isVisible())
                QApplication::postEvent(parentWidget(), new QEvent(QEvent::LayoutRequest));
        }

        QEvent hideToParentEvent(QEvent::HideToParent);
        QApplication::sendEvent(this, &hideToParentEvent);
    }
}

/*!
    Convenience function, equivalent to setVisible(! hidden).
*/
void QWidget::setHidden(bool hidden)
{
    setVisible(!hidden);
}

void QWidgetPrivate::_q_showIfNotHidden()
{
    QWidget* const q = q_func();
    if ( !(q->isHidden() && q->testAttribute(Qt::WA_WState_ExplicitShowHide)) )
        q->setVisible(true);
}

void QWidgetPrivate::showChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (!widget
            || widget->isWindow()
            || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;
        if (spontaneous) {
            widget->setAttribute(Qt::WA_Mapped);
            widget->d_func()->showChildren(true);
            QShowEvent e;
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            if (widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
                widget->d_func()->show_recursive();
            else
                widget->show();
        }
    }
}

void QWidgetPrivate::hideChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (!widget || widget->isWindow() || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;
        if (spontaneous)
            widget->setAttribute(Qt::WA_Mapped, false);
        else
            widget->setAttribute(Qt::WA_WState_Visible, false);
        widget->d_func()->hideChildren(spontaneous);
        QHideEvent e;
        if (spontaneous) {
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            QApplication::sendEvent(widget, &e);
            if (widget->internalWinId()
                && widget->testAttribute(Qt::WA_DontCreateNativeAncestors)) {
                // hide_sys() on an ancestor won't have any affect on this
                // widget, so it needs an explicit hide_sys() of its own
                widget->d_func()->hide_sys();
            }
        }
        qApp->d_func()->sendSyntheticEnterLeave(widget);
    }
}

bool QWidgetPrivate::close_helper(CloseMode mode)
{
    if (data.is_closing)
        return true;

    QWidget* const q = q_func();
    data.is_closing = 1;

	// qpointer以若引用的形式来取得qt的UI指针,做操作
    QPointer<QWidget> that = q;  // 如果没有虚引用, 2人共同指向一块内从, a释放了,b再进去就是ACCESS_VILATION
    QPointer<QWidget> parentWidget = q->parentWidget();

    bool quitOnClose = q->testAttribute(Qt::WA_QuitOnClose);
	
    if (mode != CloseNoEvent) {
        QCloseEvent e;  // 事件驱动 send+post
        if (mode == CloseWithSpontaneousEvent)
            QApplication::sendSpontaneousEvent(q, &e);
        else
            QApplication::sendEvent(q, &e);
		// 要求关闭,但没关成功, that作为pointer的虚引用的优势就显示出来了
		// 如果没有虚引用,这里的判断会非常麻烦
        if (!that.isNull() && !e.isAccepted()) {
            data.is_closing = 0;
            return false; 
        }
    }

	// close 至少先隐藏掉
    if (!that.isNull() && !q->isHidden())
        q->hide();

    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    quitOnClose = quitOnClose && (parentWidget.isNull() || !parentWidget->isVisible());

    if (quitOnClose) {
        /* if there is no non-withdrawn primary window left (except
           the ones without QuitOnClose), we emit the lastWindowClosed
           signal */
        QWidgetList list = QApplication::topLevelWidgets();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWidget *w = list.at(i);
            if (!w->isVisible() || w->parentWidget() || !w->testAttribute(Qt::WA_QuitOnClose))
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed) {
            QGuiApplicationPrivate::emitLastWindowClosed();
            QCoreApplicationPrivate *applicationPrivate = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()));
            applicationPrivate->maybeQuit();
        }
    }


    if (!that.isNull()) {
        data.is_closing = 0;
        if (q->testAttribute(Qt::WA_DeleteOnClose)) {
            q->setAttribute(Qt::WA_DeleteOnClose, false);
            q->deleteLater();
        }
    }
    return true;
}

bool QWidget::close()
{
    return d_func()->close_helper(QWidgetPrivate::CloseWithEvent);
}

bool QWidget::isVisibleTo(const QWidget *ancestor) const
{
    if (!ancestor)
        return isVisible();
    const QWidget * w = this;
    while (!w->isHidden()
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->isHidden();
}

QRegion QWidget::visibleRegion() const
{
    QWidgetPrivate * const d = d_func();

    QRect clipRect = d->clipRect();
    if (clipRect.isEmpty())
        return QRegion();
    QRegion r(clipRect);
    d->subtractOpaqueChildren(r, clipRect);
    d->subtractOpaqueSiblings(r);
    return r;
}


QSize QWidgetPrivate::adjustedSize() const
{
    QWidget* const q = q_func();

    QSize s = q->sizeHint();

    if (q->isWindow()) {
        Qt::Orientations exp;
        if (layout) {
            if (layout->hasHeightForWidth())
                s.setHeight(layout->totalHeightForWidth(s.width()));
            exp = layout->expandingDirections();
        } else
        {
            if (q->sizePolicy().hasHeightForWidth())
                s.setHeight(q->heightForWidth(s.width()));
            exp = q->sizePolicy().expandingDirections();
        }
        if (exp & Qt::Horizontal)
            s.setWidth(qMax(s.width(), 200));
        if (exp & Qt::Vertical)
            s.setHeight(qMax(s.height(), 100));
        QRect screen = QApplication::desktop()->screenGeometry(q->pos());
        s.setWidth(qMin(s.width(), screen.width()*2/3));
        s.setHeight(qMin(s.height(), screen.height()*2/3));

        if (QTLWExtra *extra = maybeTopData())
            extra->sizeAdjusted = true;
    }

    if (!s.isValid()) {
        QRect r = q->childrenRect(); // get children rectangle
        if (r.isNull())
            return s;
        s = r.size() + QSize(2 * r.x(), 2 * r.y());
    }

    return s;
}

void QWidget::adjustSize()
{
    QWidgetPrivate * const d = d_func();
    ensurePolished();
    QSize s = d->adjustedSize();

    if (d->layout)
        d->layout->activate();

    if (s.isValid())
        resize(s);
}

QSize QWidget::sizeHint() const
{
    QWidgetPrivate * const d = d_func();
    if (d->layout)
        return d->layout->totalSizeHint();
    return QSize(-1, -1);
}

QSize QWidget::minimumSizeHint() const
{
    QWidgetPrivate * const d = d_func();
    if (d->layout)
        return d->layout->totalMinimumSize();
    return QSize(-1, -1);
}

bool QWidget::isAncestorOf(const QWidget *child) const
{
    while (child) {
        if (child == this)
            return true;
        if (child->isWindow())
            return false;
        child = child->parentWidget();
    }
    return false;
}

bool QWidget::event(QEvent *event)
{
    QWidgetPrivate * const d = d_func();

    // ignore mouse and key events when disabled
    if (!isEnabled()) {
        switch(event->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
        case QEvent::ContextMenu:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Wheel:
            return false;
        default:
            break;
        }
    }
	
    switch (event->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent((QMouseEvent*)event);
        break;
    case QEvent::Wheel:
        wheelEvent((QWheelEvent*)event);
        break;
    case QEvent::TabletMove:
        if (static_cast<QTabletEvent *>(event)->buttons() == Qt::NoButton && !testAttribute(Qt::WA_TabletTracking))
            break;
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        tabletEvent((QTabletEvent*)event);
        break;
    case QEvent::KeyPress: {
        QKeyEvent *k = (QKeyEvent *)event;
        bool res = false;
        if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {  
            if (k->key() == Qt::Key_Backtab  // oye shift + tab = back_tab
                || (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier)))
                res = focusNextPrevChild(false);
            else if (k->key() == Qt::Key_Tab)
                res = focusNextPrevChild(true);
            if (res)
                break;
        }
        keyPressEvent(k);
#ifdef QT_KEYPAD_NAVIGATION
        if (!k->isAccepted() && QApplication::keypadNavigationEnabled()
            && !(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier))) {
            if (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder) {
                if (k->key() == Qt::Key_Up)
                    res = focusNextPrevChild(false);
                else if (k->key() == Qt::Key_Down)
                    res = focusNextPrevChild(true);
            } else if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
                if (k->key() == Qt::Key_Up)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionNorth);
                else if (k->key() == Qt::Key_Right)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionEast);
                else if (k->key() == Qt::Key_Down)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionSouth);
                else if (k->key() == Qt::Key_Left)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionWest);
            }
            if (res) {
                k->accept();
                break;
            }
        }
#endif
#if QT_CONFIG(whatsthis)
        if (!k->isAccepted()
            && k->modifiers() & Qt::ShiftModifier && k->key() == Qt::Key_F1
            && d->whatsThis.size()) {
            QWhatsThis::showText(mapToGlobal(inputMethodQuery(Qt::ImCursorRectangle).toRect().center()), d->whatsThis, this);
            k->accept();
        }
#endif
    }
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent((QKeyEvent*)event);
    case QEvent::ShortcutOverride:
        break;

    case QEvent::InputMethod:
        inputMethodEvent((QInputMethodEvent *) event);
        break;

    case QEvent::InputMethodQuery:
        if (testAttribute(Qt::WA_InputMethodEnabled)) {
            QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(event);
            Qt::InputMethodQueries queries = query->queries();
            for (uint i = 0; i < 32; ++i) {
                Qt::InputMethodQuery q = (Qt::InputMethodQuery)(int)(queries & (1<<i));
                if (q) {
                    QVariant v = inputMethodQuery(q);
                    if (q == Qt::ImEnabled && !v.isValid() && isEnabled())
                        v = QVariant(true); // special case for Qt4 compatibility
                    query->setValue(q, v);
                }
            }
            query->accept();
        }
        break;

    case QEvent::PolishRequest:    // widget shoude be polished
        ensurePolished();
        break;

    case QEvent::Polish: {			// widget is polished
        style()->polish(this);	// 把widget交给style()来做着色, style有可能是QStyleSheetStyle, qss在这里起作用
        setAttribute(Qt::WA_WState_Polished);
		// style 作色时不管 font和palette吗?
        if (!QApplication::font(this).isCopyOf(QApplication::font()))
            d->resolveFont();
        if (!QApplication::palette(this).isCopyOf(QApplication::palette()))
            d->resolvePalette();
    }
        break;

    case QEvent::ApplicationWindowIconChange:
        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon)) {
            d->setWindowIcon_sys();
            d->setWindowIcon_helper();
        }
        break;
    case QEvent::FocusIn:
        focusInEvent((QFocusEvent*)event);
        d->updateWidgetTransform(event);  // 和输入法有关
        break;

    case QEvent::FocusOut:
        focusOutEvent((QFocusEvent*)event);
        break;

    case QEvent::Enter:
#if QT_CONFIG(statustip)
		// 鼠标进入,给额外发送一个status_tip消息, 只要tip_string有值
        if (d->statusTip.size()) {
            QStatusTipEvent tip(d->statusTip);
            QApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        enterEvent(event);
        break;

    case QEvent::Leave:
#if QT_CONFIG(statustip)
        if (d->statusTip.size()) {
            QString empty;
            QStatusTipEvent tip(empty);
            QApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        leaveEvent(event);
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
        update();
        break;

    case QEvent::Paint:
        // At this point the event has to be delivered, regardless
        // whether the widget isVisible() or not because it
        // already went through the filters
        paintEvent((QPaintEvent*)event);
        break;

    case QEvent::Move:
        moveEvent((QMoveEvent*)event);
        d->updateWidgetTransform(event);
        break;

    case QEvent::Resize:
        resizeEvent((QResizeEvent*)event);
        d->updateWidgetTransform(event);
        break;

    case QEvent::Close:
        closeEvent((QCloseEvent *)event);
        break;

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        switch (data->context_menu_policy) {
        case Qt::PreventContextMenu:
            break;
        case Qt::DefaultContextMenu:
            contextMenuEvent(static_cast<QContextMenuEvent *>(event));
            break;
        case Qt::CustomContextMenu:
            emit customContextMenuRequested(static_cast<QContextMenuEvent *>(event)->pos());
            break;
#if QT_CONFIG(menu)
        case Qt::ActionsContextMenu:
            if (d->actions.count()) {
                QMenu::exec(d->actions, static_cast<QContextMenuEvent *>(event)->globalPos(),
                            0, this);
                break;
            }
            Q_FALLTHROUGH();
#endif
        default:
            event->ignore();
            break;
        }
        break;
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_DRAGANDDROP
    case QEvent::Drop:
        dropEvent((QDropEvent*) event);
        break;

    case QEvent::DragEnter:
        dragEnterEvent((QDragEnterEvent*) event);
        break;

    case QEvent::DragMove:
        dragMoveEvent((QDragMoveEvent*) event);
        break;

    case QEvent::DragLeave:
        dragLeaveEvent((QDragLeaveEvent*) event);
        break;
#endif

    case QEvent::Show:
        showEvent((QShowEvent*) event);
        break;

    case QEvent::Hide:
        hideEvent((QHideEvent*) event);
        break;

    case QEvent::ShowWindowRequest:
        if (!isHidden())
            d->show_sys();
        break;

    case QEvent::ApplicationFontChange:
        d->resolveFont();
        break;
    case QEvent::ApplicationPaletteChange:
        if (!(windowType() == Qt::Desktop))
            d->resolvePalette();
        break;

    case QEvent::ToolBarChange:
    case QEvent::ActivationChange:
    case QEvent::EnabledChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:				// oye: qss style
    case QEvent::PaletteChange:
    case QEvent::WindowTitleChange:
    case QEvent::IconTextChange:
    case QEvent::ModifiedChange:
    case QEvent::MouseTrackingChange:
    case QEvent::TabletTrackingChange:
    case QEvent::ParentChange:
    case QEvent::LocaleChange:
    case QEvent::MacSizeChange:
    case QEvent::ContentsRectChange:
    case QEvent::ThemeChange:
    case QEvent::ReadOnlyChange:
        changeEvent(event);
        break;
	// 最大,最小,全屏之类
    case QEvent::WindowStateChange: {
        const bool wasMinimized = static_cast<const QWindowStateChangeEvent *>(event)->oldState() & Qt::WindowMinimized;
        if (wasMinimized != isMinimized()) {
            QWidget *widget = const_cast<QWidget *>(this);
            if (wasMinimized) {
                // Always send the spontaneous events here, otherwise it can break the application!
                if (!d->childrenShownByExpose) {
                    // Show widgets only when they are not yet shown by the expose event
                    d->showChildren(true);
                    QShowEvent showEvent;
                    QCoreApplication::sendSpontaneousEvent(widget, &showEvent);
                }
                d->childrenHiddenByWState = false; // Set it always to "false" when window is restored
            } else {
                QHideEvent hideEvent;
                QCoreApplication::sendSpontaneousEvent(widget, &hideEvent);
                d->hideChildren(true);
                d->childrenHiddenByWState = true;
            }
            d->childrenShownByExpose = false; // Set it always to "false" when window state changes
        }
        changeEvent(event);
    }
        break;

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
        if (isVisible() && !palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        QList<QObject*> childList = d->children;
        for (int i = 0; i < childList.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(childList.at(i));
            if (w && w->isVisible() && !w->isWindow())
                QApplication::sendEvent(w, event);
        }
        break; }

    case QEvent::LanguageChange:
        changeEvent(event);
        {
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QObject *o = childList.at(i);
                if (o)
                    QApplication::sendEvent(o, event);
            }
        }
        update();
        break;

    case QEvent::ApplicationLayoutDirectionChange:
        d->resolveLayoutDirection();
        break;

    case QEvent::LayoutDirectionChange:
        if (d->layout)
            d->layout->invalidate();
        update();
        changeEvent(event);
        break;
    case QEvent::UpdateRequest:
        d->syncBackingStore();
        break;
    case QEvent::UpdateLater:
        update(static_cast<QUpdateLaterEvent*>(event)->region());
        break;
    case QEvent::StyleAnimationUpdate:
        if (isVisible() && !window()->isMinimized()) {
            event->accept();
            update();
        }
        break;

    case QEvent::WindowBlocked:
    case QEvent::WindowUnblocked:
        if (!d->children.isEmpty()) {
            QWidget *modalWidget = QApplication::activeModalWidget();
            for (int i = 0; i < d->children.size(); ++i) {
                QObject *o = d->children.at(i);
                if (o && o != modalWidget && o->isWidgetType()) {
                    QWidget *w  = static_cast<QWidget *>(o);
                    // do not forward the event to child windows; QApplication does this for us
                    if (!w->isWindow())
                        QApplication::sendEvent(w, event);
                }
            }
        }
        break;
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip:
        if (!d->toolTip.isEmpty())
            QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), d->toolTip, this, QRect(), d->toolTipDuration);
        else
            event->ignore();
        break;
#endif
#if QT_CONFIG(whatsthis)
    case QEvent::WhatsThis:
        if (d->whatsThis.size())
            QWhatsThis::showText(static_cast<QHelpEvent *>(event)->globalPos(), d->whatsThis, this);
        else
            event->ignore();
        break;
    case QEvent::QueryWhatsThis:
        if (d->whatsThis.isEmpty())
            event->ignore();
        break;
#endif
    case QEvent::EmbeddingControl:
        d->topData()->frameStrut.setCoords(0 ,0, 0, 0);
        data->fstrut_dirty = false;
        break;
#ifndef QT_NO_ACTION
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
        actionEvent((QActionEvent*)event);
        break;
#endif

    case QEvent::KeyboardLayoutChange:
        {
            changeEvent(event);

            // inform children of the change
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(childList.at(i));
                if (w && w->isVisible() && !w->isWindow())
                    QApplication::sendEvent(w, event);
            }
            break;
        }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    {
        event->ignore();
        break;
    }
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        event->ignore();
        break;
#endif
    case QEvent::ScreenChangeInternal:
        if (const QTLWExtra *te = d->maybeTopData()) {
            const QWindow *win = te->window;
            d->setWinId((win && win->handle()) ? win->handle()->winId() : 0);
        }
        if (d->data.fnt.d->dpi != logicalDpiY())
            d->updateFont(d->data.fnt);
#ifndef QT_NO_OPENGL
        d->renderToTextureReallyDirty = 1;
#endif
        break;
#ifndef QT_NO_PROPERTIES
    case QEvent::DynamicPropertyChange: {
        const QByteArray &propName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        if (propName.length() == 13 && !qstrncmp(propName, "_q_customDpi", 12)) {
            uint value = property(propName.constData()).toUInt();
            if (!d->extra)
                d->createExtra();
            const char axis = propName.at(12);
            if (axis == 'X')
                d->extra->customDpiX = value;
            else if (axis == 'Y')
                d->extra->customDpiY = value;
            d->updateFont(d->data.fnt);
        }
        if (windowHandle() && !qstrncmp(propName, "_q_platform_", 12))
            windowHandle()->setProperty(propName, property(propName));
        Q_FALLTHROUGH();
    }
#endif
    default:
        return QObject::event(event);
    }
    return true;
}

void QWidget::changeEvent(QEvent * event)
{
    switch(event->type()) {
    case QEvent::EnabledChange: {
        update();
        break;
    }

    case QEvent::FontChange:
    case QEvent::StyleChange: {
        QWidgetPrivate * const d = d_func();
        update();
        updateGeometry();
        if (d->layout)
            d->layout->invalidate();
        break;
    }

    case QEvent::PaletteChange:
        update();
        break;

    case QEvent::ThemeChange:
        if (QApplication::desktopSettingsAware() && windowType() != Qt::Desktop
            && qApp && !QApplication::closingDown()) {
            if (testAttribute(Qt::WA_WState_Polished))
                QApplication::style()->unpolish(this);
            if (testAttribute(Qt::WA_WState_Polished))
                QApplication::style()->polish(this);
            QEvent styleChangedEvent(QEvent::StyleChange);
            QCoreApplication::sendEvent(this, &styleChangedEvent);
            if (isVisible())
                update();
        }
        break;
    default:
        break;
    }
}

void QWidget::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void QWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
    if ((windowType() == Qt::Popup)) {
        event->accept();
        QWidget* w;
        while ((w = QApplication::activePopupWidget()) && w != this){
            w->close();
            if (QApplication::activePopupWidget() == w) // widget does not want to disappear
                w->hide(); // hide at least
        }
        if (!rect().contains(event->pos())){
            close();
        }
    }
}

void QWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

void QWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

void QWidget::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

void QWidget::tabletEvent(QTabletEvent *event)
{
    event->ignore();
}

void QWidget::keyPressEvent(QKeyEvent *event)
{
    if ((windowType() == Qt::Popup) && event->matches(QKeySequence::Cancel)) {
        event->accept();
        close(); 		// popui-win and cancel_key, close the window
    } else
    {
        event->ignore();
    }
}

void QWidget::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

void QWidget::focusInEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow()) {
        update();
    }
}

void QWidget::focusOutEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow())
        update();

#if !defined(QT_PLATFORM_UIKIT)
    // FIXME: revisit autoSIP logic, QTBUG-42906
    if (qApp->autoSipEnabled() && testAttribute(Qt::WA_InputMethodEnabled))
        QGuiApplication::inputMethod()->hide();
#endif
}

void QWidget::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void QWidget::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
}

void QWidget::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}

QVariant QWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch(query) {
    case Qt::ImCursorRectangle:
        return QRect(width()/2, 0, 1, height());
    case Qt::ImFont:
        return font();
    case Qt::ImAnchorPosition:
        // Fallback.
        return inputMethodQuery(Qt::ImCursorPosition);
    case Qt::ImHints:
        return (int)inputMethodHints();
    case Qt::ImInputItemClipRectangle:
        return d_func()->clipRect();
    default:
        return QVariant();
    }
}

Qt::InputMethodHints QWidget::inputMethodHints() const
{
#ifndef QT_NO_IM
    const QWidgetPrivate *priv = d_func();
    while (priv->inheritsInputMethodHints) {
        priv = priv->q_func()->parentWidget()->d_func();
        Q_ASSERT(priv);
    }
    return priv->imHints;
#else //QT_NO_IM
    return 0;
#endif //QT_NO_IM
}

void QWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
#ifndef QT_NO_IM
    QWidgetPrivate * const d = d_func();
    if (d->imHints == hints)
        return;
    d->imHints = hints;
    if (this == QGuiApplication::focusObject())
        QGuiApplication::inputMethod()->update(Qt::ImHints);
#else
    Q_UNUSED(hints);
#endif //QT_NO_IM
}


#ifndef QT_NO_DRAGANDDROP

/*!
    \fn void QWidget::dragEnterEvent(QDragEnterEvent *event)

    This event handler is called when a drag is in progress and the
    mouse enters this widget. The event is passed in the  event parameter.

    If the event is ignored, the widget won't receive any {dragMoveEvent()}{drag
    move events}.

    See the {dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent(QDragEnterEvent *)
{
}

/*!
    \fn void QWidget::dragMoveEvent(QDragMoveEvent *event)

    This event handler is called if a drag is in progress, and when
    any of the following conditions occur: the cursor enters this widget,
    the cursor moves within this widget, or a modifier key is pressed on
    the keyboard while this widget has the focus. The event is passed
    in the  event parameter.

    See the {dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent(QDragMoveEvent *)
{
}

/*!
    \fn void QWidget::dragLeaveEvent(QDragLeaveEvent *event)

    This event handler is called when a drag is in progress and the
    mouse leaves this widget. The event is passed in the  event
    parameter.

    See the {dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent(QDragLeaveEvent *)
{
}

/*!
    \fn void QWidget::dropEvent(QDropEvent *event)

    This event handler is called when the drag is dropped on this
    widget. The event is passed in the  event parameter.

    See the {dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDropEvent
*/
void QWidget::dropEvent(QDropEvent *)
{
}

#endif // QT_NO_DRAGANDDROP


void QWidget::showEvent(QShowEvent *)
{
}

void QWidget::hideEvent(QHideEvent *)
{
}

bool QWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

/*
    oye
    
    have a proper font and palette within the QStyle

    ensure this widget and its children has been polished by QStyle

*/
void QWidget::ensurePolished() const
{
    QWidgetPrivate * const d = d_func();

    const QMetaObject *m = metaObject();
    if (m == d->polished)
        return;  // prevent meaningless reentry
	
    d->polished = m;

	// 自己给自己发消息QEvent::Polish, 上色完成
    QEvent e(QEvent::Polish);
    QCoreApplication::sendEvent(const_cast<QWidget *>(this), &e);

    // polish children after 'this'
    QList<QObject*> children = d->children;
    for (int i = 0; i < children.size(); ++i) {
        QObject *o = children.at(i);
        if(!o->isWidgetType()) continue;
		
        if (QWidget *w = qobject_cast<QWidget *>(o))
            w->ensurePolished();
    }

    if (d->parent && d->sendChildEvents) {
        QChildEvent e(QEvent::ChildPolished, const_cast<QWidget *>(this));
        QCoreApplication::sendEvent(d->parent, &e);
    }
}

/*!
    Returns the mask currently set on a widget. If no mask is set the
    return value will be an empty region.

    \sa setMask(), clearMask(), QRegion::isEmpty(), {Shaped Clock Example}
*/
QRegion QWidget::mask() const
{
    QWidgetPrivate * const d = d_func();
    return d->extra ? d->extra->mask : QRegion();
}

QLayout *QWidget::layout() const{    return d_func()->layout;}
//oye
void QWidget::setLayout(QLayout *l)
{
	// oye: l's parent is another, steal it and send to this'widget
    QObject *oldParent = l->parent();
    if (oldParent && oldParent != this) {
        if (oldParent->isWidgetType()) {
            // Steal the layout off a widget parent. Takes effect when
            // morphing laid-out container widgets in Designer.
            QWidget *oldParentWidget = static_cast<QWidget *>(oldParent);
            oldParentWidget->takeLayout();
        } else {
            qWarning("QWidget::setLayout: Attempting to set QLayout \"%s\" on %s \"%s\", when the QLayout already has a parent",
                     l->objectName().toLocal8Bit().data(), metaObject()->className(),
                     objectName().toLocal8Bit().data());
            return;
        }
    }

	// oye: reparentChildWidgets and l call invalidate to repaint 
    //QWidgetPrivate * const d = d_func();
	QWidgetPrivate* const d = d_func();
    l->d_func()->topLevel = true;
    d->layout = l;
    if (oldParent != this) {
        l->setParent(this);
        l->d_func()->reparentChildWidgets(this);
        l->invalidate();
    }

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

QLayout *QWidget::takeLayout()
{
    QLayout *l =  layout();
    if (!l)
        return 0;
	
	QWidgetPrivate * const d = d_func();
    d->layout = 0;
    l->setParent(0);
    return l;
}


QSizePolicy QWidget::sizePolicy() const
{
    QWidgetPrivate * const d = d_func();
    return d->size_policy;
}

void QWidget::setSizePolicy(QSizePolicy policy)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_WState_OwnSizePolicy);
    if (policy == d->size_policy)
        return;

    if (d->size_policy.retainSizeWhenHidden() != policy.retainSizeWhenHidden())
        d->retainSizeWhenHiddenChanged = 1;

    d->size_policy = policy;

#if QT_CONFIG(graphicsview)
    if (QWExtra *extra = d->extra) {
        if (extra->proxyWidget)
            extra->proxyWidget->setSizePolicy(policy);
    }
#endif

    updateGeometry();
    d->retainSizeWhenHiddenChanged = 0;

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

int QWidget::heightForWidth(int w) const
{
    if (layout() && layout()->hasHeightForWidth())
        return layout()->totalHeightForWidth(w);
    return -1;
}


bool QWidget::hasHeightForWidth() const
{
    QWidgetPrivate * const d = d_func();
    return d->layout ? d->layout->hasHeightForWidth() : d->size_policy.hasHeightForWidth();
}

QWidget *QWidget::childAt(const QPoint &p) const
{
    return d_func()->childAt_helper(p, false);
}

QWidget *QWidgetPrivate::childAt_helper(const QPoint &p, bool ignoreChildrenInDestructor) const
{
    if (children.isEmpty())
        return 0;

    if (!pointInsideRectAndMask(p))
        return 0;
    return childAtRecursiveHelper(p, ignoreChildrenInDestructor);
}

QWidget *QWidgetPrivate::childAtRecursiveHelper(const QPoint &p, bool ignoreChildrenInDestructor) const
{
    for (int i = children.size() - 1; i >= 0; --i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isWindow() || child->isHidden() || child->testAttribute(Qt::WA_TransparentForMouseEvents)
            || (ignoreChildrenInDestructor && child->data->in_destructor)) {
            continue;
        }

        // Map the point 'p' from parent coordinates to child coordinates.
        QPoint childPoint = p;
        childPoint -= child->data->crect.topLeft();

        // Check if the point hits the child.
        if (!child->d_func()->pointInsideRectAndMask(childPoint))
            continue;

        // Do the same for the child's descendants.
        if (QWidget *w = child->d_func()->childAtRecursiveHelper(childPoint, ignoreChildrenInDestructor))
            return w;

        // We have found our target; namely the child at position 'p'.
        return child;
    }
    return 0;
}

void QWidgetPrivate::updateGeometry_helper(bool forceUpdate)
{
    QWidget* const q = q_func();
    if (widgetItem)
        widgetItem->invalidateSizeCache();
	
    QWidget *parent;
    if (forceUpdate || !extra || extra->minw != extra->maxw || extra->minh != extra->maxh) {
        const int isHidden = q->isHidden() && !size_policy.retainSizeWhenHidden() && !retainSizeWhenHiddenChanged;

        if (!q->isWindow() && !isHidden && (parent = q->parentWidget())) {
            if (parent->d_func()->layout)
                parent->d_func()->layout->invalidate();
            else if (parent->isVisible())
                QApplication::postEvent(parent, new QEvent(QEvent::LayoutRequest));
        }
    }
}

void QWidget::updateGeometry()
{
    QWidgetPrivate * const d = d_func();
    d->updateGeometry_helper(false);
}


void QWidget::setWindowFlags(Qt::WindowFlags flags)
{
    QWidgetPrivate * const d = d_func();
    d->setWindowFlags(flags);
}


void QWidget::setWindowFlag(Qt::WindowType flag, bool on)
{
    QWidgetPrivate * const d = d_func();
    if (on)
        d->setWindowFlags(data->window_flags | flag);
    else
        d->setWindowFlags(data->window_flags & ~flag);
}


void QWidgetPrivate::setWindowFlags(Qt::WindowFlags flags)
{
    QWidget* const q = q_func();
    if (q->data->window_flags == flags)
        return;

    if ((q->data->window_flags | flags) & Qt::Window) {
        // the old type was a window and/or the new type is a window
        QPoint oldPos = q->pos();
        bool visible = q->isVisible();
        const bool windowFlagChanged = (q->data->window_flags ^ flags) & Qt::Window;
        q->setParent(q->parentWidget(), flags);

        // if both types are windows or neither of them are, we restore
        // the old position
        if (!windowFlagChanged && (visible || q->testAttribute(Qt::WA_Moved)))
            q->move(oldPos);
        // for backward-compatibility we change Qt::WA_QuitOnClose attribute value only when the window was recreated.
        adjustQuitOnCloseAttribute();
    } else {
        q->data->window_flags = flags;
    }
}


void QWidget::overrideWindowFlags(Qt::WindowFlags flags)
{
    data->window_flags = flags;
}


/*!
    Sets the parent of the widget to  parent, and resets the window
    flags. The widget is moved to position (0, 0) in its new parent.

    If the new parent widget is in a different window, the
    reparented widget and its children are appended to the end of the
    {setFocusPolicy()}{tab chain} of the new parent
    widget, in the same internal order as before. If one of the moved
    widgets had keyboard focus, setParent() calls clearFocus() for that
    widget.

    If the new parent widget is in the same window as the
    old parent, setting the parent doesn't change the tab order or
    keyboard focus.

    If the "new" parent widget is the old parent widget, this function
    does nothing.

    \note The widget becomes invisible as part of changing its parent,
    even if it was previously visible. You must call show() to make the
    widget visible again.

    \warning It is very unlikely that you will ever need this
    function. If you have a widget that changes its content
    dynamically, it is far easier to use  QStackedWidget.

    \sa setWindowFlags()
*/
void QWidget::setParent(QWidget *parent)
{
    if (parent == parentWidget())
        return;
    setParent((QWidget*)parent, windowFlags() & ~Qt::WindowType_Mask);
}

#ifndef QT_NO_OPENGL
static void sendWindowChangeToTextureChildrenRecursively(QWidget *widget)
{
    QWidgetPrivate *d = QWidgetPrivate::get(widget);
    if (d->renderToTexture) {
        QEvent e(QEvent::WindowChangeInternal);
        QApplication::sendEvent(widget, &e);
    }

    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && QWidgetPrivate::get(w)->textureChildSeen)
            sendWindowChangeToTextureChildrenRecursively(w);
    }
}
#endif

void QWidget::setParent(QWidget *parent, Qt::WindowFlags f)
{
    QWidgetPrivate * const d = d_func();
    bool resized = testAttribute(Qt::WA_Resized);
    bool wasCreated = testAttribute(Qt::WA_WState_Created);
    QWidget *oldtlw = window();

    if (f & Qt::Window) // Frame geometry likely changes, refresh.
        d->data.fstrut_dirty = true;

    QWidget *desktopWidget = 0;
    if (parent && parent->windowType() == Qt::Desktop)
        desktopWidget = parent;
    bool newParent = (parent != parentWidget()) || !wasCreated || desktopWidget;

    if (newParent && parent && !desktopWidget) {
        if (testAttribute(Qt::WA_NativeWindow) && !qApp->testAttribute(Qt::AA_DontCreateNativeWidgetSiblings))
            parent->d_func()->enforceNativeChildren();
        else if (parent->d_func()->nativeChildrenForced() || parent->testAttribute(Qt::WA_PaintOnScreen))
            setAttribute(Qt::WA_NativeWindow);
    }

    if (wasCreated) {
        if (!testAttribute(Qt::WA_WState_Hidden)) {
            hide();
            setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        }
        if (newParent) {
            QEvent e(QEvent::ParentAboutToChange);
            QApplication::sendEvent(this, &e);
        }
    }
    if (newParent && isAncestorOf(focusWidget()))
        focusWidget()->clearFocus();

    QTLWExtra *oldTopExtra = window()->d_func()->maybeTopData();
    QWidgetBackingStoreTracker *oldBsTracker = oldTopExtra ? &oldTopExtra->backingStoreTracker : 0;

    d->setParent_sys(parent, f);

    QTLWExtra *topExtra = window()->d_func()->maybeTopData();
    QWidgetBackingStoreTracker *bsTracker = topExtra ? &topExtra->backingStoreTracker : 0;
    if (oldBsTracker && oldBsTracker != bsTracker)
        oldBsTracker->unregisterWidgetSubtree(this);

    if (desktopWidget)
        parent = 0;

#ifndef QT_NO_OPENGL
    if (d->textureChildSeen && parent) {
        // set the textureChildSeen flag up the whole parent chain
        QWidgetPrivate::get(parent)->setTextureChildSeen();
    }
#endif

    if (QWidgetBackingStore *oldBs = oldtlw->d_func()->maybeBackingStore()) {
        if (newParent)
            oldBs->removeDirtyWidget(this);
        // Move the widget and all its static children from
        // the old backing store to the new one.
        oldBs->moveStaticWidgets(this);
    }

    // ### fixme: Qt 6: Remove AA_ImmediateWidgetCreation.
    if (QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation) && !testAttribute(Qt::WA_WState_Created))
        create();

    d->reparentFocusWidgets(oldtlw);
    setAttribute(Qt::WA_Resized, resized);

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    if (!useStyleSheetPropagationInWidgetStyles && !testAttribute(Qt::WA_StyleSheet)
        && (!parent || !parent->testAttribute(Qt::WA_StyleSheet))) {
        d->resolveFont();
        d->resolvePalette();
    }
    d->resolveLayoutDirection();
    d->resolveLocale();

    // Note: GL widgets under WGL or EGL will always need a ParentChange
    // event to handle recreation/rebinding of the GL context, hence the
    // (f & Qt::MSWindowsOwnDC) clause (which is set on QGLWidgets on all
    // platforms).
    if (newParent || (f & Qt::MSWindowsOwnDC)
        ) {
        // propagate enabled updates enabled state to non-windows
        if (!isWindow()) {
            if (!testAttribute(Qt::WA_ForceDisabled))
                d->setEnabled_helper(parent ? parent->isEnabled() : true);
            if (!testAttribute(Qt::WA_ForceUpdatesDisabled))
                d->setUpdatesEnabled_helper(parent ? parent->updatesEnabled() : true);
        }
        d->inheritStyle();

        // send and post remaining QObject events
        if (parent && d->sendChildEvents) {
            QChildEvent e(QEvent::ChildAdded, this);
            QApplication::sendEvent(parent, &e);
        }

//### already hidden above ---> must probably do something smart on the mac
// #if 0 // Used to be included in Qt4 for Q_WS_MAC
//             extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
//             if(!qt_mac_is_macdrawer(q)) //special case
//                 q->setAttribute(Qt::WA_WState_Hidden);
// #else
//             q->setAttribute(Qt::WA_WState_Hidden);
//#endif

        if (parent && d->sendChildEvents && d->polished) {
            QChildEvent e(QEvent::ChildPolished, this);
            QCoreApplication::sendEvent(parent, &e);
        }

        QEvent e(QEvent::ParentChange);
        QApplication::sendEvent(this, &e);
    }
#ifndef QT_NO_OPENGL
    //renderToTexture widgets also need to know when their top-level window changes
    if (d->textureChildSeen && oldtlw != window()) {
        sendWindowChangeToTextureChildrenRecursively(this);
    }
#endif

    if (!wasCreated) {
        if (isWindow() || parentWidget()->isVisible())
            setAttribute(Qt::WA_WState_Hidden, true);
        else if (!testAttribute(Qt::WA_WState_ExplicitShowHide))
            setAttribute(Qt::WA_WState_Hidden, false);
    }

    d->updateIsOpaque();

#if QT_CONFIG(graphicsview)
    // Embed the widget into a proxy if the parent is embedded.
    // ### Doesn't handle reparenting out of an embedded widget.
    if (oldtlw->graphicsProxyWidget()) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(oldtlw))
            ancestorProxy->d_func()->unembedSubWindow(this);
    }
    if (isWindow() && parent && !graphicsProxyWidget() && !bypassGraphicsProxyWidget(this)) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(parent))
            ancestorProxy->d_func()->embedSubWindow(this);
    }
#endif

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasChanged(this);
}

void QWidgetPrivate::setParent_sys(QWidget *newparent, Qt::WindowFlags f)
{
    QWidget* const q = q_func();

    Qt::WindowFlags oldFlags = data.window_flags;
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);

    int targetScreen = -1;
    // Handle a request to move the widget to a particular screen
    if (newparent && newparent->windowType() == Qt::Desktop) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget
        const QDesktopScreenWidget *sw = qobject_cast<const QDesktopScreenWidget *>(newparent);
        targetScreen = sw ? sw->screenNumber() : 0;
        newparent = 0;
    }

    setWinId(0);

    if (parent != newparent) {
        QObjectPrivate::setParent_helper(newparent); //### why does this have to be done in the _sys function???
        if (q->windowHandle()) {
            q->windowHandle()->setFlags(f);
            QWidget *parentWithWindow =
                newparent ? (newparent->windowHandle() ? newparent : newparent->nativeParentWidget()) : 0;
            if (parentWithWindow) {
                QWidget *topLevel = parentWithWindow->window();
                if ((f & Qt::Window) && topLevel && topLevel->windowHandle()) {
                    q->windowHandle()->setTransientParent(topLevel->windowHandle());
                    q->windowHandle()->setParent(0);
                } else {
                    q->windowHandle()->setTransientParent(0);
                    q->windowHandle()->setParent(parentWithWindow->windowHandle());
                }
            } else {
                q->windowHandle()->setTransientParent(0);
                q->windowHandle()->setParent(0);
            }
        }
    }

    if (!newparent) {
        f |= Qt::Window;
        if (targetScreen == -1) {
            if (parent)
                targetScreen = QApplication::desktop()->screenNumber(q->parentWidget()->window());
        }
    }

    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    // Reparenting toplevel to child
    if (wasCreated && !(f & Qt::Window) && (oldFlags & Qt::Window) && !q->testAttribute(Qt::WA_NativeWindow)) {
        if (extra && extra->hasWindowContainer)
            QWindowContainer::toplevelAboutToBeDestroyed(q);

        QWindow *newParentWindow = newparent->windowHandle();
        if (!newParentWindow)
            if (QWidget *npw = newparent->nativeParentWidget())
                newParentWindow = npw->windowHandle();

        Q_FOREACH (QObject *child, q->windowHandle()->children()) {
            QWindow *childWindow = qobject_cast<QWindow *>(child);
            if (!childWindow)
                continue;

            QWidgetWindow *childWW = qobject_cast<QWidgetWindow *>(childWindow);
            QWidget *childWidget = childWW ? childWW->widget() : 0;
            if (!childWW || (childWidget && childWidget->testAttribute(Qt::WA_NativeWindow)))
                childWindow->setParent(newParentWindow);
        }
        q->destroy();
    }

    adjustFlags(f, q);
    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);

    if (newparent && wasCreated && (q->testAttribute(Qt::WA_NativeWindow) || (f & Qt::Window)))
        q->createWinId();

    if (q->isWindow() || (!newparent || newparent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    // move the window to the selected screen
    if (!newparent && targetScreen != -1) {
        // only if it is already created
        if (q->testAttribute(Qt::WA_WState_Created))
            q->windowHandle()->setScreen(QGuiApplication::screens().value(targetScreen, 0));
        else
            topData()->initialScreenIndex = targetScreen;
    }
}

/*!
    Scrolls the widget including its children  dx pixels to the
    right and  dy downward. Both  dx and  dy may be negative.

    After scrolling, the widgets will receive paint events for
    the areas that need to be repainted. For widgets that Qt knows to
    be opaque, this is only the newly exposed parts.
    For example, if an opaque widget is scrolled 8 pixels to the left,
    only an 8-pixel wide stripe at the right edge needs updating.

    Since widgets propagate the contents of their parents by default,
    you need to set the  autoFillBackground property, or use
    setAttribute() to set the Qt::WA_OpaquePaintEvent attribute, to make
    a widget opaque.

    For widgets that use contents propagation, a scroll will cause an
    update of the entire scroll area.

    \sa {Transparency and Double Buffering}
*/

void QWidget::scroll(int dx, int dy)
{
    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    QWidgetPrivate * const d = d_func();
#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        for (const QRect &rect : d->dirty)
            proxy->update(rect.translated(dx, dy));
        proxy->scroll(dx, dy, proxy->subWidgetRect(this));
        return;
    }
#endif
    d->setDirtyOpaqueRegion();
    d->scroll_sys(dx, dy);
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    QWidget* const q = q_func();
    scrollChildren(dx, dy);
    scrollRect(q->rect(), dx, dy);
}

/*!
    \overload

    This version only scrolls  r and does not move the children of
    the widget.

    If  r is empty or invalid, the result is undefined.

    \sa QScrollArea
*/
void QWidget::scroll(int dx, int dy, const QRect &r)
{

    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    QWidgetPrivate * const d = d_func();
#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        if (!d->dirty.isEmpty()) {
            for (const QRect &rect : d->dirty.translated(dx, dy) & r)
                proxy->update(rect);
        }
        proxy->scroll(dx, dy, r.translated(proxy->subWidgetRect(this).topLeft().toPoint()));
        return;
    }
#endif
    d->scroll_sys(dx, dy, r);
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    scrollRect(r, dx, dy);
}

void QWidget::repaint()
{
    repaint(rect());
}


void QWidget::repaint(int x, int y, int w, int h)
{
    if (x > data->crect.width() || y > data->crect.height())
        return;

    if (w < 0)
        w = data->crect.width()  - x;
    if (h < 0)
        h = data->crect.height() - y;

    repaint(QRect(x, y, w, h));
}

void QWidget::repaint(const QRect &rect)
{
    QWidgetPrivate * const d = d_func();
    d->repaint(rect);
}

void QWidget::repaint(const QRegion &rgn)
{
    QWidgetPrivate * const d = d_func();
    d->repaint(rgn);
}

template <typename T>
void QWidgetPrivate::repaint(T r)
{
    QWidget* const q = q_func();

    if (!q->isVisible() || !q->updatesEnabled() || r.isEmpty())
        return;

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore) {
        tlwExtra->inRepaint = true;
        tlwExtra->backingStoreTracker->markDirty(r, q, QWidgetBackingStore::UpdateNow);
        tlwExtra->inRepaint = false;
    }
}


void QWidget::update()
{
    update(rect());
}


void QWidget::update(const QRect &rect)
{
    QWidgetPrivate * const d = d_func();
    d->update(rect);
}


void QWidget::update(const QRegion &rgn)
{
    QWidgetPrivate * const d = d_func();
    d->update(rgn);
}

template <typename T>
void QWidgetPrivate::update(T r)
{
    QWidget* const q = q_func();

    if (!q->isVisible() || !q->updatesEnabled())
        return;

    T clipped = r & q->rect();

    if (clipped.isEmpty())
        return;

    if (q->testAttribute(Qt::WA_WState_InPaintEvent)) {
        QApplication::postEvent(q, new QUpdateLaterEvent(clipped)); // post ->  not handle immediately
        return;
    }

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore)
        tlwExtra->backingStoreTracker->markDirty(clipped, q);
}

 /*!
  \internal

  This just sets the corresponding attribute bit to 1 or 0
 */
static void setAttribute_internal(Qt::WidgetAttribute attribute, bool on, QWidgetData *data,
                                  QWidgetPrivate *d)
{
    if (attribute < int(8*sizeof(uint))) {
        if (on)
            data->widget_attributes |= (1<<attribute);
        else
            data->widget_attributes &= ~(1<<attribute);
    } else {
        const int x = attribute - 8*sizeof(uint);
        const int int_off = x / (8*sizeof(uint));
        if (on)
            d->high_attributes[int_off] |= (1<<(x-(int_off*8*sizeof(uint))));
        else
            d->high_attributes[int_off] &= ~(1<<(x-(int_off*8*sizeof(uint))));
    }
}
// 也是一个大的分发类, 不同的attri 会引起不同的操作
void QWidget::setAttribute(Qt::WidgetAttribute attribute, bool on)
{
    if (testAttribute(attribute) == on)
        return;

    QWidgetPrivate * const d = d_func();

    // ### Don't use PaintOnScreen+paintEngine() to do native painting in some future release
    if (attribute == Qt::WA_PaintOnScreen && on && windowType() != Qt::Desktop && !inherits("QGLWidget")) {
        // see ::paintEngine for details
        paintEngine();
        if (d->noPaintOnScreen)
            return;
    }

    // Don't set WA_NativeWindow on platforms that don't support it -- except for QGLWidget, which depends on it
    if (attribute == Qt::WA_NativeWindow && !d->mustHaveWindowHandle) {
        QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        if (!platformIntegration->hasCapability(QPlatformIntegration::NativeWidgets))
            return;
    }

    setAttribute_internal(attribute, on, data, d);

    switch (attribute) {
    case Qt::WA_AcceptDrops:  {
        if (on && !testAttribute(Qt::WA_DropSiteRegistered))
            setAttribute(Qt::WA_DropSiteRegistered, true);
        else if (!on && (isWindow() || !parentWidget() || !parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
            setAttribute(Qt::WA_DropSiteRegistered, false);
        QEvent e(QEvent::AcceptDropsChange);
        QApplication::sendEvent(this, &e);
        break;
    }
    case Qt::WA_DropSiteRegistered:  {
        d->registerDropSite(on);
        for (int i = 0; i < d->children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_AcceptDrops) && w->testAttribute(Qt::WA_DropSiteRegistered) != on)
                w->setAttribute(Qt::WA_DropSiteRegistered, on);
        }
        break;
    }
    case Qt::WA_NoChildEventsForParent:
        d->sendChildEvents = !on;
        break;
    case Qt::WA_NoChildEventsFromChildren:
        d->receiveChildEvents = !on;
        break;
    case Qt::WA_ShowModal:
        if (!on) {
            // reset modality type to NonModal when clearing WA_ShowModal
            data->window_modality = Qt::NonModal;
        } else if (data->window_modality == Qt::NonModal) {
            // determine the modality type if it hasn't been set prior
            // to setting WA_ShowModal. set the default to WindowModal
            // if we are the child of a group leader; otherwise use
            // ApplicationModal.
            QWidget *w = parentWidget();
            if (w)
                w = w->window();
            while (w && !w->testAttribute(Qt::WA_GroupLeader)) {
                w = w->parentWidget();
                if (w)
                    w = w->window();
            }
            data->window_modality = (w && w->testAttribute(Qt::WA_GroupLeader))
                                    ? Qt::WindowModal
                                    : Qt::ApplicationModal;
            // Some window managers do not allow us to enter modality after the
            // window is visible.The window must be hidden before changing the
            // windowModality property and then reshown.
        }
        if (testAttribute(Qt::WA_WState_Created)) {
            // don't call setModal_sys() before create_sys()
            d->setModal_sys();
        }
        break;
    case Qt::WA_MouseTracking: {
        QEvent e(QEvent::MouseTrackingChange);
        QApplication::sendEvent(this, &e);
        break; }
    case Qt::WA_TabletTracking: {
        QEvent e(QEvent::TabletTrackingChange);
        QApplication::sendEvent(this, &e);
        break; }
    case Qt::WA_NativeWindow: {
        d->createTLExtra();
        if (on)
            d->createTLSysExtra();
#ifndef QT_NO_IM
        QWidget *focusWidget = d->effectiveFocusWidget();
        if (on && !internalWinId() && this == QGuiApplication::focusObject()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
        if (!qApp->testAttribute(Qt::AA_DontCreateNativeWidgetSiblings) && parentWidget())
            parentWidget()->d_func()->enforceNativeChildren();
        if (on && !internalWinId() && testAttribute(Qt::WA_WState_Created))
            d->createWinId();
        if (isEnabled() && focusWidget->isEnabled() && this == QGuiApplication::focusObject()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_PaintOnScreen:
        d->updateIsOpaque();
        Q_FALLTHROUGH();
    case Qt::WA_OpaquePaintEvent:
        d->updateIsOpaque();
        break;
    case Qt::WA_NoSystemBackground:
        d->updateIsOpaque();
        Q_FALLTHROUGH();
    case Qt::WA_UpdatesDisabled:
        d->updateSystemBackground();
        break;
    case Qt::WA_TransparentForMouseEvents:
        break;
    case Qt::WA_InputMethodEnabled: {
#ifndef QT_NO_IM
        if (QGuiApplication::focusObject() == this) {
            if (!on)
                QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_WindowPropagation:
        d->resolvePalette();
        d->resolveFont();
        d->resolveLocale();
        break;

    case Qt::WA_DontShowOnScreen: {
        if (on && isVisible()) {
            // Make sure we keep the current state and only hide the widget
            // from the desktop. show_sys will only update platform specific
            // attributes at this point.
            d->hide_sys();
            d->show_sys();
        }
        break;
    }

    case Qt::WA_StaticContents:
        if (QWidgetBackingStore *bs = d->maybeBackingStore()) {
            if (on)
                bs->addStaticWidget(this);
            else
                bs->removeStaticWidget(this);
        }
        break;
    case Qt::WA_TranslucentBackground:
        if (on) {
            setAttribute(Qt::WA_NoSystemBackground);
            d->updateIsTranslucent();
        }

        break;
    case Qt::WA_AcceptTouchEvents:
        break;
    default:
        break;
    }
}


bool QWidget::testAttribute_helper(Qt::WidgetAttribute attribute) const
{
    QWidgetPrivate * const d = d_func();
    const int x = attribute - 8*sizeof(uint);
    const int int_off = x / (8*sizeof(uint));
    return (d->high_attributes[int_off] & (1<<(x-(int_off*8*sizeof(uint)))));
}

qreal QWidget::windowOpacity() const
{
    QWidgetPrivate * const d = d_func();
    return (isWindow() && d->maybeTopData()) ? d->maybeTopData()->opacity / 255. : 1.0;
}

void QWidget::setWindowOpacity(qreal opacity)
{
    QWidgetPrivate * const d = d_func();
    if (!isWindow())  return;

    opacity = qBound(qreal(0.0), opacity, qreal(1.0));
    QTLWExtra *extra = d->topData();
    extra->opacity = uint(opacity * 255);
    setAttribute(Qt::WA_WState_WindowOpacitySet);
    d->setWindowOpacity_sys(opacity);

    if (!testAttribute(Qt::WA_WState_Created))
        return;

#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = graphicsProxyWidget()) {
        // Avoid invalidating the cache if set.
        if (proxy->cacheMode() == QGraphicsItem::NoCache)
            proxy->update();
        else if (QGraphicsScene *scene = proxy->scene())
            scene->update(proxy->sceneBoundingRect());
        return;
    }
#endif
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    QWidget* const q = q_func();
    if (q->windowHandle())
        q->windowHandle()->setOpacity(level);
}

bool QWidget::isWindowModified() const
{
    return testAttribute(Qt::WA_WindowModified);
}

void QWidget::setWindowModified(bool mod)
{
    QWidgetPrivate * const d = d_func();
    setAttribute(Qt::WA_WindowModified, mod);

    d->setWindowModified_helper();

    QEvent e(QEvent::ModifiedChange);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::setWindowModified_helper()
{
    QWidget* const q = q_func();
    QWindow *window = q->windowHandle();
    if (!window)
        return;
    QPlatformWindow *platformWindow = window->handle();
    if (!platformWindow)
        return;
    bool on = q->testAttribute(Qt::WA_WindowModified);
    if (!platformWindow->setWindowModified(on)) {
        if (Q_UNLIKELY(on && !q->windowTitle().contains(QLatin1String("[*]"))))
            qWarning("QWidget::setWindowModified: The window title does not contain a '[*]' placeholder");
        setWindowTitle_helper(q->windowTitle());
        setWindowIconText_helper(q->windowIconText());
    }
}


void QWidget::setToolTip(const QString &s)
{
    QWidgetPrivate * const d = d_func();
    d->toolTip = s;

    QEvent event(QEvent::ToolTipChange);
    QApplication::sendEvent(this, &event);
}

QString QWidget::toolTip() const
{
    QWidgetPrivate * const d = d_func();
    return d->toolTip;
}

void QWidget::setToolTipDuration(int msec)
{
    QWidgetPrivate * const d = d_func();
    d->toolTipDuration = msec;
}

int QWidget::toolTipDuration() const
{
    QWidgetPrivate * const d = d_func();
    return d->toolTipDuration;
}



#if QT_CONFIG(statustip)
/*!
  \property QWidget::statusTip
  \brief the widget's status tip

  By default, this property contains an empty string.

  \sa toolTip, whatsThis
*/
void QWidget::setStatusTip(const QString &s)
{
    QWidgetPrivate * const d = d_func();
    d->statusTip = s;
}

QString QWidget::statusTip() const
{
    QWidgetPrivate * const d = d_func();
    return d->statusTip;
}
#endif // QT_CONFIG(statustip)

#if QT_CONFIG(whatsthis)
/*!
  \property QWidget::whatsThis

  \brief the widget's What's This help text.

  By default, this property contains an empty string.

  \sa QWhatsThis, QWidget::toolTip, QWidget::statusTip
*/
void QWidget::setWhatsThis(const QString &s)
{
    QWidgetPrivate * const d = d_func();
    d->whatsThis = s;
}

QString QWidget::whatsThis() const
{
    QWidgetPrivate * const d = d_func();
    return d->whatsThis;
}
#endif // QT_CONFIG(whatsthis)

#ifndef QT_NO_SHORTCUT
/*!
    Adds a shortcut to Qt's shortcut system that watches for the given
     key sequence in the given  context. If the  context is
    Qt::ApplicationShortcut, the shortcut applies to the application as a
    whole. Otherwise, it is either local to this widget, Qt::WidgetShortcut,
    or to the window itself, Qt::WindowShortcut.

    If the same  key sequence has been grabbed by several widgets,
    when the  key sequence occurs a QEvent::Shortcut event is sent
    to all the widgets to which it applies in a non-deterministic
    order, but with the ``ambiguous'' flag set to true.

    \warning You should not normally need to use this function;
    instead create {QAction}s with the shortcut key sequences you
    require (if you also want equivalent menu options and toolbar
    buttons), or create {QShortcut}s if you just need key sequences.
    Both QAction and QShortcut handle all the event filtering for you,
    and provide signals which are triggered when the user triggers the
    key sequence, so are much easier to use than this low-level
    function.

    \sa releaseShortcut(), setShortcutEnabled()
*/
int QWidget::grabShortcut(const QKeySequence &key, Qt::ShortcutContext context)
{
    Q_ASSERT(qApp);
    if (key.isEmpty())
        return 0;
    setAttribute(Qt::WA_GrabbedShortcut);
    return qApp->d_func()->shortcutMap.addShortcut(this, key, context, qWidgetShortcutContextMatcher);
}

/*!
    Removes the shortcut with the given  id from Qt's shortcut
    system. The widget will no longer receive QEvent::Shortcut events
    for the shortcut's key sequence (unless it has other shortcuts
    with the same key sequence).

    \warning You should not normally need to use this function since
    Qt's shortcut system removes shortcuts automatically when their
    parent widget is destroyed. It is best to use QAction or
    QShortcut to handle shortcuts, since they are easier to use than
    this low-level function. Note also that this is an expensive
    operation.

    \sa grabShortcut(), setShortcutEnabled()
*/
void QWidget::releaseShortcut(int id)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.removeShortcut(id, this, 0);
}

/*!
    If  enable is true, the shortcut with the given  id is
    enabled; otherwise the shortcut is disabled.

    \warning You should not normally need to use this function since
    Qt's shortcut system enables/disables shortcuts automatically as
    widgets become hidden/visible and gain or lose focus. It is best
    to use QAction or QShortcut to handle shortcuts, since they are
    easier to use than this low-level function.

    \sa grabShortcut(), releaseShortcut()
*/
void QWidget::setShortcutEnabled(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutEnabled(enable, id, this, 0);
}

/*!
    \since 4.2

    If  enable is true, auto repeat of the shortcut with the
    given  id is enabled; otherwise it is disabled.

    \sa grabShortcut(), releaseShortcut()
*/
void QWidget::setShortcutAutoRepeat(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutAutoRepeat(enable, id, this, 0);
}
#endif // QT_NO_SHORTCUT

/*!
    Updates the widget's micro focus.
*/
void QWidget::updateMicroFocus()
{
    // updating everything since this is currently called for any kind of state change
    if (this == QGuiApplication::focusObject())
        QGuiApplication::inputMethod()->update(Qt::ImQueryAll);
}

void QWidget::raise()
{
    QWidgetPrivate * const d = d_func();
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != parentChildCount -1)
            p->d_func()->children.move(from, parentChildCount - 1);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == parentChildCount - 1)
            return;

        QRegion region(rect());
        d->subtractOpaqueSiblings(region);
        d->invalidateBuffer(region);
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->raise_sys();

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasRaised(this);

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::raise_sys()
{
    QWidget* const q = q_func();
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        q->windowHandle()->raise();
    } else if (renderToTexture) {
        if (QWidget *p = q->parentWidget()) {
            setDirtyOpaqueRegion();
            p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
        }
    }
}



void QWidget::lower()
{
    QWidgetPrivate * const d = d_func();
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != 0)
            p->d_func()->children.move(from, 0);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == 0)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->lower_sys();

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasLowered(this);

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::lower_sys()
{
    QWidget* const q = q_func();
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        q->windowHandle()->lower();
    } else if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
    }
}

void QWidget::stackUnder(QWidget* w)
{
    QWidgetPrivate * const d = d_func();
    QWidget *p = parentWidget();
    if (!w || isWindow() || p != w->parentWidget() || this == w)
        return;
    if (p) {
        int from = p->d_func()->children.indexOf(this);
        int to = p->d_func()->children.indexOf(w);
        Q_ASSERT(from >= 0);
        Q_ASSERT(to >= 0);
        if (from < to)
            --to;
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != to)
            p->d_func()->children.move(from, to);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == to)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->stackUnder_sys(w);

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::stackUnder_sys(QWidget*)
{
    QWidget* const q = q_func();
    if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
    }
}

QRect QWidgetPrivate::frameStrut() const
{
    QWidget* const q = q_func();
    if (!q->isWindow() || (q->windowType() == Qt::Desktop) || q->testAttribute(Qt::WA_DontShowOnScreen)) {
        // x2 = x1 + w - 1, so w/h = 1
        return QRect(0, 0, 1, 1);
    }

    if (data.fstrut_dirty  && q->isVisible()  && q->testAttribute(Qt::WA_WState_Created))
        const_cast<QWidgetPrivate *>(this)->updateFrameStrut();

    return maybeTopData() ? maybeTopData()->frameStrut : QRect();
}

void QWidgetPrivate::updateFrameStrut()
{
    QWidget* const q = q_func();
    if (q->data->fstrut_dirty) {
        if (QTLWExtra *te = maybeTopData()) {
            if (te->window && te->window->handle()) {
                const QMargins margins = te->window->frameMargins();
                if (!margins.isNull()) {
                    te->frameStrut.setCoords(margins.left(), margins.top(), margins.right(), margins.bottom());
                    q->data->fstrut_dirty = false;
                }
            }
        }
    }
}

#ifdef QT_KEYPAD_NAVIGATION
/*!
    \internal

    Changes the focus  from the current focusWidget to a widget in
    the  direction.

    Returns  true, if there was a widget in that direction
*/
bool QWidgetPrivate::navigateToDirection(Direction direction)
{
    QWidget *targetWidget = widgetInNavigationDirection(direction);
    if (targetWidget)
        targetWidget->setFocus();
    return (targetWidget != 0);
}

/*!
    \internal

    Searches for a widget that is positioned in the  direction, starting
    from the current focusWidget.

    Returns the pointer to a found widget or 0, if there was no widget in
    that direction.
*/
QWidget *QWidgetPrivate::widgetInNavigationDirection(Direction direction)
{
    const QWidget *sourceWidget = QApplication::focusWidget();
    if (!sourceWidget)
        return 0;
    const QRect sourceRect = sourceWidget->rect().translated(sourceWidget->mapToGlobal(QPoint()));
    const int sourceX =
            (direction == DirectionNorth || direction == DirectionSouth) ?
                (sourceRect.left() + (sourceRect.right() - sourceRect.left()) / 2)
                :(direction == DirectionEast ? sourceRect.right() : sourceRect.left());
    const int sourceY =
            (direction == DirectionEast || direction == DirectionWest) ?
                (sourceRect.top() + (sourceRect.bottom() - sourceRect.top()) / 2)
                :(direction == DirectionSouth ? sourceRect.bottom() : sourceRect.top());
    const QPoint sourcePoint(sourceX, sourceY);
    const QPoint sourceCenter = sourceRect.center();
    const QWidget *sourceWindow = sourceWidget->window();

    QWidget *targetWidget = 0;
    int shortestDistance = INT_MAX;

    const auto targetCandidates = QApplication::allWidgets();
    for (QWidget *targetCandidate : targetCandidates) {

        const QRect targetCandidateRect = targetCandidate->rect().translated(targetCandidate->mapToGlobal(QPoint()));

        // For focus proxies, the child widget handling the focus can have keypad navigation focus,
        // but the owner of the proxy cannot.
        // Additionally, empty widgets should be ignored.
        if (targetCandidate->focusProxy() || targetCandidateRect.isEmpty())
            continue;

        // Only navigate to a target widget that...
        if (       targetCandidate != sourceWidget
                   // ...takes the focus,
                && targetCandidate->focusPolicy() & Qt::TabFocus
                   // ...is above if DirectionNorth,
                && !(direction == DirectionNorth && targetCandidateRect.bottom() > sourceRect.top())
                   // ...is on the right if DirectionEast,
                && !(direction == DirectionEast  && targetCandidateRect.left()   < sourceRect.right())
                   // ...is below if DirectionSouth,
                && !(direction == DirectionSouth && targetCandidateRect.top()    < sourceRect.bottom())
                   // ...is on the left if DirectionWest,
                && !(direction == DirectionWest  && targetCandidateRect.right()  > sourceRect.left())
                   // ...is enabled,
                && targetCandidate->isEnabled()
                   // ...is visible,
                && targetCandidate->isVisible()
                   // ...is in the same window,
                && targetCandidate->window() == sourceWindow) {
            const int targetCandidateDistance = pointToRect(sourcePoint, targetCandidateRect);
            if (targetCandidateDistance < shortestDistance) {
                shortestDistance = targetCandidateDistance;
                targetWidget = targetCandidate;
            }
        }
    }
    return targetWidget;
}

/*!
    \internal

    Tells us if it there is currently a reachable widget by keypad navigation in
    a certain  orientation.
    If no navigation is possible, occurring key events in that  orientation may
    be used to interact with the value in the focused widget, even though it
    currently has not the editFocus.

    \sa QWidgetPrivate::widgetInNavigationDirection(), QWidget::hasEditFocus()
*/
bool QWidgetPrivate::canKeypadNavigate(Qt::Orientation orientation)
{
    return orientation == Qt::Horizontal?
            (QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionEast)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionWest))
            :(QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionNorth)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionSouth));
}
/*!
    \internal

    Checks, if the  widget is inside a QTabWidget. If is is inside
    one, left/right key events will be used to switch between tabs in keypad
    navigation. If there is no QTabWidget, the horizontal key events can be used
to
    interact with the value in the focused widget, even though it currently has
    not the editFocus.

    \sa QWidget::hasEditFocus()
*/
bool QWidgetPrivate::inTabWidget(QWidget *widget)
{
    for (QWidget *tabWidget = widget; tabWidget; tabWidget = tabWidget->parentWidget())
        if (qobject_cast<const QTabWidget*>(tabWidget))
            return true;
    return false;
}
#endif


void QWidget::setBackingStore(QBackingStore *store)
{
    // ### createWinId() ??

    if (!isTopLevel())
        return;

    QWidgetPrivate * const d = d_func();

    QTLWExtra *topData = d->topData();
    if (topData->backingStore == store)
        return;

    QBackingStore *oldStore = topData->backingStore;
    deleteBackingStore(d);
    topData->backingStore = store;

    QWidgetBackingStore *bs = d->maybeBackingStore();
    if (!bs)
        return;

    if (isTopLevel()) {
        if (bs->store != oldStore && bs->store != store)
            delete bs->store;
        bs->store = store;
    }
}


QBackingStore *QWidget::backingStore() const
{
    QWidgetPrivate * const d = d_func();
    QTLWExtra *extra = d->maybeTopData();
    if (extra && extra->backingStore)
        return extra->backingStore;

    QWidgetBackingStore *bs = d->maybeBackingStore();

    return bs ? bs->store : 0;
}

void QWidgetPrivate::getLayoutItemMargins(int *left, int *top, int *right, int *bottom) const
{
    if (left)
        *left = (int)leftLayoutItemMargin;
    if (top)
        *top = (int)topLayoutItemMargin;
    if (right)
        *right = (int)rightLayoutItemMargin;
    if (bottom)
        *bottom = (int)bottomLayoutItemMargin;
}

void QWidgetPrivate::setLayoutItemMargins(int left, int top, int right, int bottom)
{
    if (leftLayoutItemMargin == left
        && topLayoutItemMargin == top
        && rightLayoutItemMargin == right
        && bottomLayoutItemMargin == bottom)
        return;

    QWidget* const q = q_func();
    leftLayoutItemMargin = (signed char)left;
    topLayoutItemMargin = (signed char)top;
    rightLayoutItemMargin = (signed char)right;
    bottomLayoutItemMargin = (signed char)bottom;
    q->updateGeometry();
}

void QWidgetPrivate::setLayoutItemMargins(QStyle::SubElement element, const QStyleOption *opt)
{
    QWidget* const q = q_func();
    QStyleOption myOpt;
    if (!opt) {
        myOpt.initFrom(q);
        myOpt.rect.setRect(0, 0, 32768, 32768);     // arbitrary
        opt = &myOpt;
    }

    QRect liRect = q->style()->subElementRect(element, opt, q);
    if (liRect.isValid()) {
        leftLayoutItemMargin = (signed char)(opt->rect.left() - liRect.left());
        topLayoutItemMargin = (signed char)(opt->rect.top() - liRect.top());
        rightLayoutItemMargin = (signed char)(liRect.right() - opt->rect.right());
        bottomLayoutItemMargin = (signed char)(liRect.bottom() - opt->rect.bottom());
    } else {
        leftLayoutItemMargin = 0;
        topLayoutItemMargin = 0;
        rightLayoutItemMargin = 0;
        bottomLayoutItemMargin = 0;
    }
}
// resets the Qt::WA_QuitOnClose attribute to the default value for transient widgets.
void QWidgetPrivate::adjustQuitOnCloseAttribute()
{
    QWidget* const q = q_func();

    if (!q->parentWidget()) {
        Qt::WindowType type = q->windowType();
        if (type == Qt::Widget || type == Qt::SubWindow)
            type = Qt::Window;
        if (type != Qt::Widget && type != Qt::Window && type != Qt::Dialog)
            q->setAttribute(Qt::WA_QuitOnClose, false);
    }
}

QOpenGLContext *QWidgetPrivate::shareContext() const
{
#ifdef QT_NO_OPENGL
    return 0;
#else
    if (Q_UNLIKELY(!extra || !extra->topextra || !extra->topextra->window)) {
        qWarning("Asking for share context for widget that does not have a window handle");
        return 0;
    }
    QWidgetPrivate *that = const_cast<QWidgetPrivate *>(this);
    if (!extra->topextra->shareContext) {
        QOpenGLContext *ctx = new QOpenGLContext;
        ctx->setShareContext(qt_gl_global_share_context());
        ctx->setFormat(extra->topextra->window->format());
        ctx->setScreen(extra->topextra->window->screen());
        ctx->create();
        that->extra->topextra->shareContext = ctx;
    }
    return that->extra->topextra->shareContext;
#endif // QT_NO_OPENGL
}

#ifndef QT_NO_OPENGL
void QWidgetPrivate::sendComposeStatus(QWidget *w, bool end)
{
    QWidgetPrivate *wd = QWidgetPrivate::get(w);
    if (!wd->textureChildSeen)
        return;
    if (end)
        wd->endCompose();
    else
        wd->beginCompose();
    for (int i = 0; i < wd->children.size(); ++i) {
        w = qobject_cast<QWidget *>(wd->children.at(i));
        if (w && !w->isWindow() && !w->isHidden() && QWidgetPrivate::get(w)->textureChildSeen)
            sendComposeStatus(w, end);
    }
}
#endif // QT_NO_OPENGL




#if QT_CONFIG(graphicsview)
/*!
   \since 4.5

   Returns the proxy widget for the corresponding embedded widget in a graphics
   view; otherwise returns 0.

   \sa QGraphicsProxyWidget::createProxyForChildWidget(),
       QGraphicsScene::addWidget()
 */
QGraphicsProxyWidget *QWidget::graphicsProxyWidget() const
{
    QWidgetPrivate * const d = d_func();
    if (d->extra) {
        return d->extra->proxyWidget;
    }
    return 0;
}
#endif

void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    QWidgetPrivate * const d = d_func();

    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();

    if ((windowType() == Qt::Popup) && qApp)
        qApp->d_func()->closePopup(this);

    if (this == QApplicationPrivate::active_window)
        QApplication::setActiveWindow(0);
    if (QWidget::mouseGrabber() == this)
        releaseMouse();
    if (QWidget::keyboardGrabber() == this)
        releaseKeyboard();

    setAttribute(Qt::WA_WState_Created, false);

    if (windowType() != Qt::Desktop) {
        if (destroySubWindows) {
            QObjectList childList(children());
            for (int i = 0; i < childList.size(); i++) {
                QWidget *widget = qobject_cast<QWidget *>(childList.at(i));
                if (widget && widget->testAttribute(Qt::WA_NativeWindow)) {
                    if (widget->windowHandle()) {
                        widget->destroy();
                    }
                }
            }
        }
        if (destroyWindow) {
            d->deleteTLSysExtra();
        } else {
            if (parentWidget() && parentWidget()->testAttribute(Qt::WA_WState_Created)) {
                d->hide_sys();
            }
        }

        d->setWinId(0);
    }
}


QPaintEngine *QWidget::paintEngine() const
{
    qWarning("QWidget::paintEngine: Should no longer be called");
    const_cast<QWidgetPrivate *>(d_func())->noPaintOnScreen = 1;
    return 0; //##### @@@
}

// Do not call QWindow::mapToGlobal() until QPlatformWindow is properly showing.
static inline bool canMapPosition(QWindow *window)
{
    return window->handle() && !qt_window_private(window)->resizeEventPending;
}

#if QT_CONFIG(graphicsview)
static inline QGraphicsProxyWidget *graphicsProxyWidget(const QWidget *w)
{
    QGraphicsProxyWidget *result = Q_NULLPTR;
    const QWidgetPrivate *d = qt_widget_private(const_cast<QWidget *>(w));
    if (d->extra)
        result = d->extra->proxyWidget;
    return result;
}
#endif // QT_CONFIG(graphicsview)

struct MapToGlobalTransformResult {
    QTransform transform;
    QWindow *window;
};

static MapToGlobalTransformResult mapToGlobalTransform(const QWidget *w)
{
    MapToGlobalTransformResult result;
    result.window = Q_NULLPTR;
    for ( ; w ; w = w->parentWidget()) {
        if (QGraphicsProxyWidget *qgpw = graphicsProxyWidget(w)) {
            if (const QGraphicsScene *scene = qgpw->scene()) {
                const QList <QGraphicsView *> views = scene->views();
                if (!views.isEmpty()) {
                    result.transform *= qgpw->sceneTransform();
                    result.transform *= views.first()->viewportTransform();
                    w = views.first()->viewport();
                }
            }
        }
        QWindow *window = w->windowHandle();
        if (window && canMapPosition(window)) {
            result.window = window;
            break;
        }

        const QPoint topLeft = w->geometry().topLeft();
        result.transform.translate(topLeft.x(), topLeft.y());
        if (w->isWindow())
            break;
    }
    return result;
}


QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    const MapToGlobalTransformResult t = mapToGlobalTransform(this);
    const QPoint g = t.transform.map(pos);
    return t.window ? t.window->mapToGlobal(g) : g;
}

/*!
    \fn QPoint QWidget::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate  pos to widget
    coordinates.

    \sa mapToGlobal(), mapFrom(), mapFromParent()
*/
QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
   const MapToGlobalTransformResult t = mapToGlobalTransform(this);
   const QPoint windowLocal = t.window ? t.window->mapFromGlobal(pos) : pos;
   return t.transform.inverted().map(windowLocal);
}

QWidget *qt_pressGrab = 0;
QWidget *qt_mouseGrb = 0;
static bool mouseGrabWithCursor = false;
static QWidget *keyboardGrb = 0;

static inline QWindow *grabberWindow(const QWidget *w)
{
    QWindow *window = w->windowHandle();
    if (!window)
        if (const QWidget *nativeParent = w->nativeParentWidget())
            window = nativeParent->windowHandle();
    return window;
}

#ifndef QT_NO_CURSOR
static void grabMouseForWidget(QWidget *widget, const QCursor *cursor = 0)
#else
static void grabMouseForWidget(QWidget *widget)
#endif
{
    if (qt_mouseGrb)
        qt_mouseGrb->releaseMouse();

    mouseGrabWithCursor = false;
    if (QWindow *window = grabberWindow(widget)) {
#ifndef QT_NO_CURSOR
        if (cursor) {
            mouseGrabWithCursor = true;
            QGuiApplication::setOverrideCursor(*cursor);
        }
#endif // !QT_NO_CURSOR
        window->setMouseGrabEnabled(true);
    }

    qt_mouseGrb = widget;
    qt_pressGrab = 0;
}

static void releaseMouseGrabOfWidget(QWidget *widget)
{
    if (qt_mouseGrb == widget) {
        if (QWindow *window = grabberWindow(widget)) {
#ifndef QT_NO_CURSOR
            if (mouseGrabWithCursor) {
                QGuiApplication::restoreOverrideCursor();
                mouseGrabWithCursor = false;
            }
#endif // !QT_NO_CURSOR
            window->setMouseGrabEnabled(false);
        }
    }
    qt_mouseGrb = 0;
}

/*!
    \fn void QWidget::grabMouse()

    Grabs the mouse input.

    This widget receives all mouse events until releaseMouse() is
    called; other widgets get no mouse events at all. Keyboard
    events are not affected. Use grabKeyboard() if you want to grab
    that.

    \warning Bugs in mouse-grabbing applications very often lock the
    terminal. Use this function with extreme caution, and consider
    using the  -nograb command line option while debugging.

    It is almost never necessary to grab the mouse when using Qt, as
    Qt grabs and releases it sensibly. In particular, Qt grabs the
    mouse when a mouse button is pressed and keeps it until the last
    button is released.

    \note Only visible widgets can grab mouse input. If isVisible()
    returns  false for a widget, that widget cannot call grabMouse().

    \note On Windows, grabMouse() only works when the mouse is inside a window
    owned by the process.
    On \macos, grabMouse() only works when the mouse is inside the frame of that widget.

    \sa releaseMouse(), grabKeyboard(), releaseKeyboard()
*/
void QWidget::grabMouse()
{
    grabMouseForWidget(this);
}

/*!
    \fn void QWidget::grabMouse(const QCursor &cursor)
    \overload grabMouse()

    Grabs the mouse input and changes the cursor shape.

    The cursor will assume shape  cursor (for as long as the mouse
    focus is grabbed) and this widget will be the only one to receive
    mouse events until releaseMouse() is called().

    \warning Grabbing the mouse might lock the terminal.

    \note See the note in QWidget::grabMouse().

    \sa releaseMouse(), grabKeyboard(), releaseKeyboard(), setCursor()
*/
#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    grabMouseForWidget(this, &cursor);
}
#endif

bool QWidgetPrivate::stealMouseGrab(bool grab)
{
    // This is like a combination of grab/releaseMouse() but with error checking
    // and it has no effect on the result of mouseGrabber().
    QWidget* const q = q_func();
    QWindow *window = grabberWindow(q);
    return window ? window->setMouseGrabEnabled(grab) : false;
}

/*!
    \fn void QWidget::releaseMouse()

    Releases the mouse grab.

    \sa grabMouse(), grabKeyboard(), releaseKeyboard()
*/
void QWidget::releaseMouse()
{
    releaseMouseGrabOfWidget(this);
}

/*!
    \fn void QWidget::grabKeyboard()

    Grabs the keyboard input.

    This widget receives all keyboard events until releaseKeyboard()
    is called; other widgets get no keyboard events at all. Mouse
    events are not affected. Use grabMouse() if you want to grab that.

    The focus widget is not affected, except that it doesn't receive
    any keyboard events. setFocus() moves the focus as usual, but the
    new focus widget receives keyboard events only after
    releaseKeyboard() is called.

    If a different widget is currently grabbing keyboard input, that
    widget's grab is released first.

    \sa releaseKeyboard(), grabMouse(), releaseMouse(), focusWidget()
*/
void QWidget::grabKeyboard()
{
    if (keyboardGrb)
        keyboardGrb->releaseKeyboard();
    if (QWindow *window = grabberWindow(this))
        window->setKeyboardGrabEnabled(true);
    keyboardGrb = this;
}

bool QWidgetPrivate::stealKeyboardGrab(bool grab)
{
    // This is like a combination of grab/releaseKeyboard() but with error
    // checking and it has no effect on the result of keyboardGrabber().
    QWidget* const q = q_func();
    QWindow *window = grabberWindow(q);
    return window ? window->setKeyboardGrabEnabled(grab) : false;
}

/*!
    \fn void QWidget::releaseKeyboard()

    Releases the keyboard grab.

    \sa grabKeyboard(), grabMouse(), releaseMouse()
*/
void QWidget::releaseKeyboard()
{
    if (keyboardGrb == this) {
        if (QWindow *window = grabberWindow(this))
            window->setKeyboardGrabEnabled(false);
        keyboardGrb = 0;
    }
}

/*!
    \fn QWidget *QWidget::mouseGrabber()

    Returns the widget that is currently grabbing the mouse input.

    If no widget in this application is currently grabbing the mouse,
    0 is returned.

    \sa grabMouse(), keyboardGrabber()
*/
QWidget *QWidget::mouseGrabber()
{
    if (qt_mouseGrb)
        return qt_mouseGrb;
    return qt_pressGrab;
}

/*!
    \fn QWidget *QWidget::keyboardGrabber()

    Returns the widget that is currently grabbing the keyboard input.

    If no widget in this application is currently grabbing the
    keyboard, 0 is returned.

    \sa grabMouse(), mouseGrabber()
*/
QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}


void QWidget::activateWindow()
{
    QWindow *const wnd = window()->windowHandle();

    if (wnd)
        wnd->requestActivate();
}


int QWidget::metric(PaintDeviceMetric m) const
{
    QWindow *topLevelWindow = 0;
    QScreen *screen = 0;
    if (QWidget *topLevel = window()) {
        topLevelWindow = topLevel->windowHandle();
        if (topLevelWindow)
            screen = topLevelWindow->screen();
    }
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    if (!screen) {
        if (m == PdmDpiX || m == PdmDpiY)
              return 72;
        return QPaintDevice::metric(m);
    }
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmWidthMM) {
        val = data->crect.width() * screen->physicalSize().width() / screen->geometry().width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else if (m == PdmHeightMM) {
        val = data->crect.height() * screen->physicalSize().height() / screen->geometry().height();
    } else if (m == PdmDepth) {
        return screen->depth();
    } else if (m == PdmDpiX) {
        for (const QWidget *p = this; p; p = p->parentWidget()) {
            if (p->d_func()->extra && p->d_func()->extra->customDpiX)
                return p->d_func()->extra->customDpiX;
        }
        return qRound(screen->logicalDotsPerInchX());
    } else if (m == PdmDpiY) {
        for (const QWidget *p = this; p; p = p->parentWidget()) {
            if (p->d_func()->extra && p->d_func()->extra->customDpiY)
                return p->d_func()->extra->customDpiY;
        }
        return qRound(screen->logicalDotsPerInchY());
    } else if (m == PdmPhysicalDpiX) {
        return qRound(screen->physicalDotsPerInchX());
    } else if (m == PdmPhysicalDpiY) {
        return qRound(screen->physicalDotsPerInchY());
    } else if (m == PdmDevicePixelRatio) {
        return topLevelWindow ? topLevelWindow->devicePixelRatio() : qApp->devicePixelRatio();
    } else if (m == PdmDevicePixelRatioScaled) {
        return (QPaintDevice::devicePixelRatioFScale() *
                (topLevelWindow ? topLevelWindow->devicePixelRatio() : qApp->devicePixelRatio()));
    } else {
        val = QPaintDevice::metric(m);// XXX
    }
    return val;
}


void QWidget::initPainter(QPainter *painter) const
{
    const QPalette &pal = palette();
    painter->d_func()->state->pen = QPen(pal.brush(foregroundRole()), 1);
    painter->d_func()->state->bgBrush = pal.brush(backgroundRole());
    QFont f(font(), const_cast<QWidget *>(this));
    painter->d_func()->state->deviceFont = f;
    painter->d_func()->state->font = f;
}


QPaintDevice *QWidget::redirected(QPoint *offset) const
{
    return d_func()->redirected(offset);
}


QPainter *QWidget::sharedPainter() const
{
    // Someone sent a paint event directly to the widget
    if (!d_func()->redirectDev)
        return 0;

    QPainter *sp = d_func()->sharedPainter();
    if (!sp || !sp->isActive())
        return 0;

    if (sp->paintEngine()->paintDevice() != d_func()->redirectDev)
        return 0;

    return sp;
}

void QWidget::setMask(const QRegion &newMask)
{
    QWidgetPrivate * const d = d_func();

    d->createExtra();
    if (newMask == d->extra->mask)
        return;

#ifndef QT_NO_BACKINGSTORE
    const QRegion oldMask(d->extra->mask);
#endif

    d->extra->mask = newMask;
    d->extra->hasMask = !newMask.isEmpty();

#if 1 // Used to be excluded in Qt4 for Q_WS_MAC
    if (!testAttribute(Qt::WA_WState_Created))
        return;
#endif

    d->setMask_sys(newMask);

#ifndef QT_NO_BACKINGSTORE
    if (!isVisible())
        return;

    if (!d->extra->hasMask) {
        // Mask was cleared; update newly exposed area.
        QRegion expose(rect());
        expose -= oldMask;
        if (!expose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            update(expose);
        }
        return;
    }

    if (!isWindow()) {
        // Update newly exposed area on the parent widget.
        QRegion parentExpose(rect());
        parentExpose -= newMask;
        if (!parentExpose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            parentExpose.translate(data->crect.topLeft());
            parentWidget()->update(parentExpose);
        }

        // Update newly exposed area on this widget
        if (!oldMask.isEmpty())
            update(newMask - oldMask);
    }
#endif
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    QWidget* const q = q_func();
    if (QWindow *window = q->windowHandle())
        window->setMask(region);
}

/*!
    \fn void QWidget::setMask(const QBitmap &bitmap)

    Causes only the pixels of the widget for which  bitmap has a
    corresponding 1 bit to be visible. If the region includes pixels
    outside the rect() of the widget, window system controls in that
    area may or may not be visible, depending on the platform.

    Note that this effect can be slow if the region is particularly
    complex.

    The following code shows how an image with an alpha channel can be
    used to generate a mask for a widget:

    \snippet widget-mask/main.cpp 0

    The label shown by this code is masked using the image it contains,
    giving the appearance that an irregularly-shaped image is being drawn
    directly onto the screen.

    Masked widgets receive mouse events only on their visible
    portions.

    \sa clearMask(), windowOpacity(), {Shaped Clock Example}
*/
void QWidget::setMask(const QBitmap &bitmap)
{
    setMask(QRegion(bitmap));
}

/*!
    \fn void QWidget::clearMask()

    Removes any mask set by setMask().

    \sa setMask()
*/
void QWidget::clearMask()
{
    QWidgetPrivate * const d = d_func();
    if (!d->extra || !d->extra->hasMask)
        return;
    setMask(QRegion());
}

void QWidgetPrivate::setWidgetParentHelper(QObject *widgetAsObject, QObject *newParent)
{
    Q_ASSERT(widgetAsObject->isWidgetType());
    Q_ASSERT(!newParent || newParent->isWidgetType());
    QWidget *widget = static_cast<QWidget*>(widgetAsObject);
    widget->setParent(static_cast<QWidget*>(newParent));
}



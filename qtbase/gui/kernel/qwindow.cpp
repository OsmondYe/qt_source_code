
QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), 0)
    , QSurface(QSurface::Window)
{
    QWindowPrivate * const d = d_func();
    d->init(targetScreen);
}

static QWindow *nonDesktopParent(QWindow *parent)
{
    if (parent && parent->type() == Qt::Desktop) {
        qWarning("QWindows can not be reparented into desktop windows");
        return nullptr;
    }

    return parent;
}


QWindow::QWindow(QWindow *parent)
    : QWindow(*new QWindowPrivate(), parent)
{
}


QWindow::QWindow(QWindowPrivate &dd, QWindow *parent)
    : QObject(dd, nonDesktopParent(parent))
    , QSurface(QSurface::Window)
{
    QWindowPrivate * const d = d_func();
    d->init();
}

/*!
    Destroys the window.
*/
QWindow::~QWindow()
{
    QWindowPrivate * const d = d_func();
    d->destroy();
    QGuiApplicationPrivate::window_list.removeAll(this);
    if (!QGuiApplicationPrivate::is_app_closing)
        QGuiApplicationPrivate::instance()->modalWindowList.removeOne(this);
}

void QWindowPrivate::init(QScreen *targetScreen)
{
    Q_Q(QWindow);

    parentWindow = static_cast<QWindow *>(q->QObject::parent());

    if (!parentWindow)
        connectToScreen(targetScreen ? targetScreen : QGuiApplication::primaryScreen());

    // If your application aborts here, you are probably creating a QWindow
    // before the screen list is populated.
    if (Q_UNLIKELY(!parentWindow && !topLevelScreen)) {
        qFatal("Cannot create window: no screens available");
        exit(1);
    }
    QGuiApplicationPrivate::window_list.prepend(q);

    requestedFormat = QSurfaceFormat::defaultFormat();
}

QWindow::Visibility QWindow::visibility() const
{
    Q_D(const QWindow);
    return d->visibility;
}

void QWindow::setVisibility(Visibility v)
{
    switch (v) {
    case Hidden:
        hide();
        break;
    case AutomaticVisibility:
        show();
        break;
    case Windowed:
        showNormal();
        break;
    case Minimized:
        showMinimized();
        break;
    case Maximized:
        showMaximized();
        break;
    case FullScreen:
        showFullScreen();
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

void QWindowPrivate::updateVisibility()
{
    Q_Q(QWindow);

    QWindow::Visibility old = visibility;

    if (visible) {
        switch (windowState) {
        case Qt::WindowMinimized:
            visibility = QWindow::Minimized;
            break;
        case Qt::WindowMaximized:
            visibility = QWindow::Maximized;
            break;
        case Qt::WindowFullScreen:
            visibility = QWindow::FullScreen;
            break;
        case Qt::WindowNoState:
            visibility = QWindow::Windowed;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    } else {
        visibility = QWindow::Hidden;
    }

    if (visibility != old)
        emit q->visibilityChanged(visibility);
}

void QWindowPrivate::updateSiblingPosition(SiblingPosition position)
{
    Q_Q(QWindow);

    if (!q->parent())
        return;

    QObjectList &siblings = q->parent()->d_ptr->children;

    const int siblingCount = siblings.size() - 1;
    if (siblingCount == 0)
        return;

    const int currentPosition = siblings.indexOf(q);
    Q_ASSERT(currentPosition >= 0);

    const int targetPosition = position == PositionTop ? siblingCount : 0;

    if (currentPosition == targetPosition)
        return;

    siblings.move(currentPosition, targetPosition);
}

inline bool QWindowPrivate::windowRecreationRequired(QScreen *newScreen) const
{
    Q_Q(const QWindow);
    const QScreen *oldScreen = q->screen();
    return oldScreen != newScreen && (platformWindow || !oldScreen)
        && !(oldScreen && oldScreen->virtualSiblings().contains(newScreen));
}

inline void QWindowPrivate::disconnectFromScreen()
{
    if (topLevelScreen)
        topLevelScreen = 0;
}

void QWindowPrivate::connectToScreen(QScreen *screen)
{
    disconnectFromScreen();
    topLevelScreen = screen;
}

void QWindowPrivate::emitScreenChangedRecursion(QScreen *newScreen)
{
    Q_Q(QWindow);
    emit q->screenChanged(newScreen);
    for (QObject *child : q->children()) {
        if (child->isWindowType())
            static_cast<QWindow *>(child)->d_func()->emitScreenChangedRecursion(newScreen);
    }
}

void QWindowPrivate::setTopLevelScreen(QScreen *newScreen, bool recreate)
{
    Q_Q(QWindow);
    if (parentWindow) {
        qWarning() << q << '(' << newScreen << "): Attempt to set a screen on a child window.";
        return;
    }
    if (newScreen != topLevelScreen) {
        const bool shouldRecreate = recreate && windowRecreationRequired(newScreen);
        const bool shouldShow = visibilityOnDestroy && !topLevelScreen;
        if (shouldRecreate && platformWindow)
            q->destroy();
        connectToScreen(newScreen);
        if (shouldShow)
            q->setVisible(true);
        else if (newScreen && shouldRecreate)
            create(true);
        emitScreenChangedRecursion(newScreen);
    }
}

void QWindowPrivate::create(bool recursive, WId nativeHandle)
{
    Q_Q(QWindow);
    if (platformWindow)
        return;

    if (q->parent())
        q->parent()->create();

    QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    platformWindow = nativeHandle ? platformIntegration->createForeignWindow(q, nativeHandle)
        : platformIntegration->createPlatformWindow(q);
    Q_ASSERT(platformWindow);

    if (!platformWindow) {
        qWarning() << "Failed to create platform window for" << q << "with flags" << q->flags();
        return;
    }

    QObjectList childObjects = q->children();
    for (int i = 0; i < childObjects.size(); i ++) {
        QObject *object = childObjects.at(i);
        if (!object->isWindowType())
            continue;

        QWindow *childWindow = static_cast<QWindow *>(object);
        if (recursive)
            childWindow->d_func()->create(recursive);

        // The child may have had deferred creation due to this window not being created
        // at the time setVisible was called, so we re-apply the visible state, which
        // may result in creating the child, and emitting the appropriate signals.
        if (childWindow->isVisible())
            childWindow->setVisible(true);

        if (QPlatformWindow *childPlatformWindow = childWindow->d_func()->platformWindow)
            childPlatformWindow->setParent(this->platformWindow);
    }

    QPlatformSurfaceEvent e(QPlatformSurfaceEvent::SurfaceCreated);
    QGuiApplication::sendEvent(q, &e);
}

void QWindowPrivate::clearFocusObject()
{
}

// Allows for manipulating the suggested geometry before a resize/move
// event in derived classes for platforms that support it, for example to
// implement heightForWidth().
QRectF QWindowPrivate::closestAcceptableGeometry(const QRectF &rect) const
{
    Q_UNUSED(rect)
    return QRectF();
}


void QWindow::setSurfaceType(SurfaceType surfaceType)
{
    QWindowPrivate * const d = d_func();
    d->surfaceType = surfaceType;
}


QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
}


void QWindow::setVisible(bool visible)
{
    QWindowPrivate * const d = d_func();

    if (d->visible != visible) {
        d->visible = visible;
        emit visibleChanged(visible);
        d->updateVisibility();
    } else if (d->platformWindow) {
        // Visibility hasn't changed, and the platform window is in sync
        return;
    }

    if (!d->platformWindow) {
        // If we have a parent window, but the parent hasn't been created yet, we
        // can defer creation until the parent is created or we're re-parented.
        if (parent() && !parent()->handle())
            return;

        // We only need to create the window if it's being shown
        if (visible)
            create();
    }

    if (visible) {
        // remove posted quit events when showing a new window
        QCoreApplication::removePostedEvents(qApp, QEvent::Quit);

        if (type() == Qt::Window) {
            QGuiApplicationPrivate *app_priv = QGuiApplicationPrivate::instance();
            QString &firstWindowTitle = app_priv->firstWindowTitle;
            if (!firstWindowTitle.isEmpty()) {
                setTitle(firstWindowTitle);
                firstWindowTitle = QString();
            }
            if (!app_priv->forcedWindowIcon.isNull())
                setIcon(app_priv->forcedWindowIcon);

            // Handling of the -qwindowgeometry, -geometry command line arguments
            static bool geometryApplied = false;
            if (!geometryApplied) {
                geometryApplied = true;
                QGuiApplicationPrivate::applyWindowGeometrySpecificationTo(this);
            }
        }

        QShowEvent showEvent;
        QGuiApplication::sendEvent(this, &showEvent);
    }

    if (isModal()) {
        if (visible)
            QGuiApplicationPrivate::showModalWindow(this);
        else
            QGuiApplicationPrivate::hideModalWindow(this);
    // QShapedPixmapWindow is used on some platforms for showing a drag pixmap, so don't block
    // input to this window as it is performing a drag - QTBUG-63846
    } else if (visible && QGuiApplication::modalWindow() && !qobject_cast<QShapedPixmapWindow *>(this)) {
        QGuiApplicationPrivate::updateBlockedStatus(this);
    }

#ifndef QT_NO_CURSOR
    if (visible && (d->hasCursor || QGuiApplication::overrideCursor()))
        d->applyCursor();
#endif

    if (d->platformWindow)
        d->platformWindow->setVisible(visible);

    if (!visible) {
        QHideEvent hideEvent;
        QGuiApplication::sendEvent(this, &hideEvent);
    }
}

bool QWindow::isVisible() const
{
    Q_D(const QWindow);

    return d->visible;
}


void QWindow::create()
{
    QWindowPrivate * const d = d_func();
    d->create(false);
}


WId QWindow::winId() const
{
    Q_D(const QWindow);

    if(!d->platformWindow)
        const_cast<QWindow *>(this)->create();

    return d->platformWindow->winId();
}


QWindow *QWindow::parent(AncestorMode mode) const
{
    Q_D(const QWindow);
    return d->parentWindow ? d->parentWindow : (mode == IncludeTransients ? transientParent() : nullptr);
}


QWindow *QWindow::parent() const
{
    Q_D(const QWindow);
    return d->parentWindow;
}


void QWindow::setParent(QWindow *parent)
{
    parent = nonDesktopParent(parent);

    QWindowPrivate * const d = d_func();
    if (d->parentWindow == parent)
        return;

    QScreen *newScreen = parent ? parent->screen() : screen();
    if (d->windowRecreationRequired(newScreen)) {
        qWarning() << this << '(' << parent << "): Cannot change screens (" << screen() << newScreen << ')';
        return;
    }

    QObject::setParent(parent);
    d->parentWindow = parent;

    if (parent)
        d->disconnectFromScreen();
    else
        d->connectToScreen(newScreen);

    // If we were set visible, but not created because we were a child, and we're now
    // re-parented into a created parent, or to being a top level, we need re-apply the
    // visibility state, which will also create.
    if (isVisible() && (!parent || parent->handle()))
        setVisible(true);

    if (d->platformWindow) {
        if (parent)
            parent->create();

        d->platformWindow->setParent(parent ? parent->d_func()->platformWindow : 0);
    }

    QGuiApplicationPrivate::updateBlockedStatus(this);
}


bool QWindow::isTopLevel() const
{
    Q_D(const QWindow);
    return d->parentWindow == 0;
}


bool QWindow::isModal() const
{
    Q_D(const QWindow);
    return d->modality != Qt::NonModal;
}


Qt::WindowModality QWindow::modality() const
{
    Q_D(const QWindow);
    return d->modality;
}

void QWindow::setModality(Qt::WindowModality modality)
{
    QWindowPrivate * const d = d_func();
    if (d->modality == modality)
        return;
    d->modality = modality;
    emit modalityChanged(modality);
}

void QWindow::setFormat(const QSurfaceFormat &format)
{
    QWindowPrivate * const d = d_func();
    d->requestedFormat = format;
}


QSurfaceFormat QWindow::requestedFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
}


QSurfaceFormat QWindow::format() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->format();
    return d->requestedFormat;
}


void QWindow::setFlags(Qt::WindowFlags flags)
{
    QWindowPrivate * const d = d_func();
    if (d->windowFlags == flags)
        return;

    if (d->platformWindow)
        d->platformWindow->setWindowFlags(flags);
    d->windowFlags = flags;
}

Qt::WindowFlags QWindow::flags() const
{
    Q_D(const QWindow);
    Qt::WindowFlags flags = d->windowFlags;

    if (d->platformWindow && d->platformWindow->isForeignWindow())
        flags |= Qt::ForeignWindow;

    return flags;
}


void QWindow::setFlag(Qt::WindowType flag, bool on)
{
    QWindowPrivate * const d = d_func();
    if (on)
        setFlags(d->windowFlags | flag);
    else
        setFlags(d->windowFlags & ~flag);
}


Qt::WindowType QWindow::type() const
{
    Q_D(const QWindow);
    return static_cast<Qt::WindowType>(int(d->windowFlags & Qt::WindowType_Mask));
}


void QWindow::setTitle(const QString &title)
{
    QWindowPrivate * const d = d_func();
    bool changed = false;
    if (d->windowTitle != title) {
        d->windowTitle = title;
        changed = true;
    }
    if (d->platformWindow && type() != Qt::Desktop)
        d->platformWindow->setWindowTitle(title);
    if (changed)
        emit windowTitleChanged(title);
}

QString QWindow::title() const
{
    Q_D(const QWindow);
    return d->windowTitle;
}


void QWindow::setFilePath(const QString &filePath)
{
    QWindowPrivate * const d = d_func();
    d->windowFilePath = filePath;
    if (d->platformWindow)
        d->platformWindow->setWindowFilePath(filePath);
}


QString QWindow::filePath() const
{
    Q_D(const QWindow);
    return d->windowFilePath;
}


void QWindow::setIcon(const QIcon &icon)
{
    QWindowPrivate * const d = d_func();
    d->windowIcon = icon;
    if (d->platformWindow)
        d->platformWindow->setWindowIcon(icon);
    QEvent e(QEvent::WindowIconChange);
    QCoreApplication::sendEvent(this, &e);
}


QIcon QWindow::icon() const
{
    Q_D(const QWindow);
    if (d->windowIcon.isNull())
        return QGuiApplication::windowIcon();
    return d->windowIcon;
}


void QWindow::raise()
{
    QWindowPrivate * const d = d_func();

    d->updateSiblingPosition(QWindowPrivate::PositionTop);

    if (d->platformWindow)
        d->platformWindow->raise();
}


void QWindow::lower()
{
    QWindowPrivate * const d = d_func();

    d->updateSiblingPosition(QWindowPrivate::PositionBottom);

    if (d->platformWindow)
        d->platformWindow->lower();
}


void QWindow::setOpacity(qreal level)
{
    QWindowPrivate * const d = d_func();
    if (level == d->opacity)
        return;
    d->opacity = level;
    if (d->platformWindow) {
        d->platformWindow->setOpacity(level);
        emit opacityChanged(level);
    }
}

qreal QWindow::opacity() const
{
    Q_D(const QWindow);
    return d->opacity;
}


void QWindow::setMask(const QRegion &region)
{
    QWindowPrivate * const d = d_func();
    if (!d->platformWindow)
        return;
    d->platformWindow->setMask(QHighDpi::toNativeLocalRegion(region, this));
    d->mask = region;
}


QRegion QWindow::mask() const
{
    Q_D(const QWindow);
    return d->mask;
}


void QWindow::requestActivate()
{
    QWindowPrivate * const d = d_func();
    if (flags() & Qt::WindowDoesNotAcceptFocus) {
        qWarning() << "requestActivate() called for " << this << " which has Qt::WindowDoesNotAcceptFocus set.";
        return;
    }
    if (d->platformWindow)
        d->platformWindow->requestActivateWindow();
}


bool QWindow::isExposed() const
{
    Q_D(const QWindow);
    return d->exposed;
}


bool QWindow::isActive() const
{
    Q_D(const QWindow);
    if (!d->platformWindow)
        return false;

    QWindow *focus = QGuiApplication::focusWindow();

    // Means the whole application lost the focus
    if (!focus)
        return false;

    if (focus == this)
        return true;

    if (QWindow *p = parent(IncludeTransients))
        return p->isActive();
    else
        return isAncestorOf(focus);
}


void QWindow::reportContentOrientationChange(Qt::ScreenOrientation orientation)
{
    QWindowPrivate * const d = d_func();
    if (d->contentOrientation == orientation)
        return;
    if (d->platformWindow)
        d->platformWindow->handleContentOrientationChange(orientation);
    d->contentOrientation = orientation;
    emit contentOrientationChanged(orientation);
}

Qt::ScreenOrientation QWindow::contentOrientation() const
{
    Q_D(const QWindow);
    return d->contentOrientation;
}


qreal QWindow::devicePixelRatio() const
{
    Q_D(const QWindow);

    // If there is no platform window use the associated screen's devicePixelRatio,
    // which typically is the primary screen and will be correct for single-display
    // systems (a very common case).
    if (!d->platformWindow)
        return screen()->devicePixelRatio();

    return d->platformWindow->devicePixelRatio() * QHighDpiScaling::factor(this);
}


void QWindow::setWindowState(Qt::WindowState state)
{
    if (state == Qt::WindowActive) {
        qWarning("QWindow::setWindowState does not accept Qt::WindowActive");
        return;
    }

    QWindowPrivate * const d = d_func();
    if (d->platformWindow)
        d->platformWindow->setWindowState(state);
    d->windowState = state;
    emit windowStateChanged(d->windowState);
    d->updateVisibility();
}


Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return d->windowState;
}


void QWindow::setTransientParent(QWindow *parent)
{
    QWindowPrivate * const d = d_func();
    if (parent && !parent->isTopLevel()) {
        qWarning() << parent << "must be a top level window.";
        return;
    }
    if (parent == this) {
        qWarning() << "transient parent" << parent << "can not be same as window";
        return;
    }

    d->transientParent = parent;

    QGuiApplicationPrivate::updateBlockedStatus(this);
}


QWindow *QWindow::transientParent() const
{
    Q_D(const QWindow);
    return d->transientParent.data();
}


bool QWindow::isAncestorOf(const QWindow *child, AncestorMode mode) const
{
    if (child->parent() == this || (mode == IncludeTransients && child->transientParent() == this))
        return true;

    if (QWindow *parent = child->parent(mode)) {
        if (isAncestorOf(parent, mode))
            return true;
    } else if (handle() && child->handle()) {
        if (handle()->isAncestorOf(child->handle()))
            return true;
    }

    return false;
}


QSize QWindow::minimumSize() const
{
    Q_D(const QWindow);
    return d->minimumSize;
}


QSize QWindow::maximumSize() const
{
    Q_D(const QWindow);
    return d->maximumSize;
}


QSize QWindow::baseSize() const
{
    Q_D(const QWindow);
    return d->baseSize;
}


QSize QWindow::sizeIncrement() const
{
    Q_D(const QWindow);
    return d->sizeIncrement;
}


void QWindow::setMinimumSize(const QSize &size)
{
    QWindowPrivate * const d = d_func();
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->minimumSize == adjustedSize)
        return;
    QSize oldSize = d->minimumSize;
    d->minimumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
    if (d->minimumSize.width() != oldSize.width())
        emit minimumWidthChanged(d->minimumSize.width());
    if (d->minimumSize.height() != oldSize.height())
        emit minimumHeightChanged(d->minimumSize.height());
}


void QWindow::setX(int arg)
{
    QWindowPrivate * const d = d_func();
    if (x() != arg)
        setGeometry(QRect(arg, y(), width(), height()));
    else
        d->positionAutomatic = false;
}


void QWindow::setY(int arg)
{
    QWindowPrivate * const d = d_func();
    if (y() != arg)
        setGeometry(QRect(x(), arg, width(), height()));
    else
        d->positionAutomatic = false;
}


void QWindow::setWidth(int arg)
{
    if (width() != arg)
        resize(arg, height());
}


void QWindow::setHeight(int arg)
{
    if (height() != arg)
        resize(width(), arg);
}


void QWindow::setMinimumWidth(int w)
{
    setMinimumSize(QSize(w, minimumHeight()));
}


void QWindow::setMinimumHeight(int h)
{
    setMinimumSize(QSize(minimumWidth(), h));
}


void QWindow::setMaximumSize(const QSize &size)
{
    QWindowPrivate * const d = d_func();
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->maximumSize == adjustedSize)
        return;
    QSize oldSize = d->maximumSize;
    d->maximumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
    if (d->maximumSize.width() != oldSize.width())
        emit maximumWidthChanged(d->maximumSize.width());
    if (d->maximumSize.height() != oldSize.height())
        emit maximumHeightChanged(d->maximumSize.height());
}


void QWindow::setMaximumWidth(int w)
{
    setMaximumSize(QSize(w, maximumHeight()));
}

void QWindow::setMaximumHeight(int h)
{
    setMaximumSize(QSize(maximumWidth(), h));
}


void QWindow::setBaseSize(const QSize &size)
{
    QWindowPrivate * const d = d_func();
    if (d->baseSize == size)
        return;
    d->baseSize = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}


void QWindow::setSizeIncrement(const QSize &size)
{
    QWindowPrivate * const d = d_func();
    if (d->sizeIncrement == size)
        return;
    d->sizeIncrement = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}


void QWindow::setGeometry(int posx, int posy, int w, int h)
{
    setGeometry(QRect(posx, posy, w, h));
}


void QWindow::setGeometry(const QRect &rect)
{
    QWindowPrivate * const d = d_func();
    d->positionAutomatic = false;
    const QRect oldRect = geometry();
    if (rect == oldRect)
        return;

    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        QRect nativeRect;
        QScreen *newScreen = d->screenForGeometry(rect);
        if (newScreen && isTopLevel())
            nativeRect = QHighDpi::toNativePixels(rect, newScreen);
        else
            nativeRect = QHighDpi::toNativePixels(rect, this);
        d->platformWindow->setGeometry(nativeRect);
    } else {
        d->geometry = rect;

        if (rect.x() != oldRect.x())
            emit xChanged(rect.x());
        if (rect.y() != oldRect.y())
            emit yChanged(rect.y());
        if (rect.width() != oldRect.width())
            emit widthChanged(rect.width());
        if (rect.height() != oldRect.height())
            emit heightChanged(rect.height());
    }
}


QScreen *QWindowPrivate::screenForGeometry(const QRect &newGeometry)
{
    Q_Q(QWindow);
    QScreen *currentScreen = q->screen();
    QScreen *fallback = currentScreen;
    QPoint center = newGeometry.center();
    if (!q->parent() && currentScreen && !currentScreen->geometry().contains(center)) {
        const auto screens = currentScreen->virtualSiblings();
        for (QScreen* screen : screens) {
            if (screen->geometry().contains(center))
                return screen;
            if (screen->geometry().intersects(newGeometry))
                fallback = screen;
        }
    }
    return fallback;
}



QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return QHighDpi::fromNativePixels(d->platformWindow->geometry(), this);
    return d->geometry;
}


QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return QHighDpi::fromNativePixels(d->platformWindow->frameMargins(), this);
    return QMargins();
}


QRect QWindow::frameGeometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins m = frameMargins();
        return QHighDpi::fromNativePixels(d->platformWindow->geometry(), this).adjusted(-m.left(), -m.top(), m.right(), m.bottom());
    }
    return d->geometry;
}


QPoint QWindow::framePosition() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins margins = frameMargins();
        return QHighDpi::fromNativePixels(d->platformWindow->geometry().topLeft(), this) - QPoint(margins.left(), margins.top());
    }
    return d->geometry.topLeft();
}


void QWindow::setFramePosition(const QPoint &point)
{
    QWindowPrivate * const d = d_func();
    d->positionPolicy = QWindowPrivate::WindowFrameInclusive;
    d->positionAutomatic = false;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QHighDpi::toNativePixels(QRect(point, size()), this));
    } else {
        d->geometry.moveTopLeft(point);
    }
}


void QWindow::setPosition(const QPoint &pt)
{
    setGeometry(QRect(pt, size()));
}


void QWindow::setPosition(int posx, int posy)
{
    setPosition(QPoint(posx, posy));
}


void QWindow::resize(int w, int h)
{
    resize(QSize(w, h));
}


void QWindow::resize(const QSize &newSize)
{
    QWindowPrivate * const d = d_func();
    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QHighDpi::toNativePixels(QRect(position(), newSize), this));
    } else {
        const QSize oldSize = d->geometry.size();
        d->geometry.setSize(newSize);
        if (newSize.width() != oldSize.width())
            emit widthChanged(newSize.width());
        if (newSize.height() != oldSize.height())
            emit heightChanged(newSize.height());
    }
}


void QWindow::destroy()
{
    QWindowPrivate * const d = d_func();
    if (!d->platformWindow)
        return;

    if (d->platformWindow->isForeignWindow())
        return;

    d->destroy();
}

void QWindowPrivate::destroy()
{
    if (!platformWindow)
        return;

    Q_Q(QWindow);
    QObjectList childrenWindows = q->children();
    for (int i = 0; i < childrenWindows.size(); i++) {
        QObject *object = childrenWindows.at(i);
        if (object->isWindowType()) {
            QWindow *w = static_cast<QWindow*>(object);
            qt_window_private(w)->destroy();
        }
    }

    if (QGuiApplicationPrivate::focus_window == q)
        QGuiApplicationPrivate::focus_window = q->parent();
    if (QGuiApplicationPrivate::currentMouseWindow == q)
        QGuiApplicationPrivate::currentMouseWindow = q->parent();
    if (QGuiApplicationPrivate::currentMousePressWindow == q)
        QGuiApplicationPrivate::currentMousePressWindow = q->parent();

    for (int i = 0; i < QGuiApplicationPrivate::tabletDevicePoints.size(); ++i)
        if (QGuiApplicationPrivate::tabletDevicePoints.at(i).target == q)
            QGuiApplicationPrivate::tabletDevicePoints[i].target = q->parent();

    bool wasVisible = q->isVisible();
    visibilityOnDestroy = wasVisible && platformWindow;

    q->setVisible(false);

    // Let subclasses act, typically by doing graphics resource cleaup, when
    // the window, to which graphics resource may be tied, is going away.
    //
    // NB! This is disfunctional when destroy() is invoked from the dtor since
    // a reimplemented event() will not get called in the subclasses at that
    // stage. However, the typical QWindow cleanup involves either close() or
    // going through QWindowContainer, both of which will do an explicit, early
    // destroy(), which is good here.

    QPlatformSurfaceEvent e(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
    QGuiApplication::sendEvent(q, &e);

    delete platformWindow;
    resizeEventPending = true;
    receivedExpose = false;
    exposed = false;
    platformWindow = 0;

    if (wasVisible)
        maybeQuitOnLastWindowClosed();
}


QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}


QPlatformSurface *QWindow::surfaceHandle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}


bool QWindow::setKeyboardGrabEnabled(bool grab)
{
    QWindowPrivate * const d = d_func();
    if (d->platformWindow)
        return d->platformWindow->setKeyboardGrabEnabled(grab);
    return false;
}


bool QWindow::setMouseGrabEnabled(bool grab)
{
    QWindowPrivate * const d = d_func();
    if (d->platformWindow)
        return d->platformWindow->setMouseGrabEnabled(grab);
    return false;
}


QScreen *QWindow::screen() const
{
    Q_D(const QWindow);
    return d->parentWindow ? d->parentWindow->screen() : d->topLevelScreen.data();
}


void QWindow::setScreen(QScreen *newScreen)
{
    QWindowPrivate * const d = d_func();
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    d->setTopLevelScreen(newScreen, newScreen != 0);
}


QAccessibleInterface *QWindow::accessibleRoot() const
{
    return 0;
}


QObject *QWindow::focusObject() const
{
    return const_cast<QWindow *>(this);
}

// oye
void QWindow::show()
{
    Qt::WindowState defaultState = QGuiApplicationPrivate::platformIntegration()->defaultWindowState(d_func()->windowFlags);
    if (defaultState == Qt::WindowFullScreen)
        showFullScreen();
    else if (defaultState == Qt::WindowMaximized)
        showMaximized();
    else
        showNormal();
}


void QWindow::hide()
{
    setVisible(false);
}


void QWindow::showMinimized()
{
    setWindowState(Qt::WindowMinimized);
    setVisible(true);
}


void QWindow::showMaximized()
{
    setWindowState(Qt::WindowMaximized);
    setVisible(true);
}


void QWindow::showFullScreen()
{
    setWindowState(Qt::WindowFullScreen);
    setVisible(true);
    requestActivate();
}


void QWindow::showNormal()
{
    setWindowState(Qt::WindowNoState);
    setVisible(true);
}


bool QWindow::close()
{
    QWindowPrivate * const d = d_func();

    // Do not close non top level windows
    if (parent())
        return false;

    if (!d->platformWindow)
        return true;

    bool accepted = false;
    QWindowSystemInterface::handleCloseEvent(this, &accepted);
    QWindowSystemInterface::flushWindowSystemEvents();
    return accepted;
}


void QWindow::exposeEvent(QExposeEvent *ev)
{
    ev->ignore();
}


void QWindow::moveEvent(QMoveEvent *ev)
{
    ev->ignore();
}


void QWindow::resizeEvent(QResizeEvent *ev)
{
    ev->ignore();
}


void QWindow::showEvent(QShowEvent *ev)
{
    ev->ignore();
}


void QWindow::hideEvent(QHideEvent *ev)
{
    ev->ignore();
}


bool QWindow::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        touchEvent(static_cast<QTouchEvent *>(ev));
        break;

    case QEvent::Move:
        moveEvent(static_cast<QMoveEvent*>(ev));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(ev));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::FocusIn: {
        focusInEvent(static_cast<QFocusEvent *>(ev));
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
        break; }

    case QEvent::FocusOut: {
        focusOutEvent(static_cast<QFocusEvent *>(ev));
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
        break; }

    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
        break;

    case QEvent::Close:
        if (ev->isAccepted())
            destroy();
        break;

    case QEvent::Expose:
        exposeEvent(static_cast<QExposeEvent *>(ev));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(ev));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(ev));
        break;

    case QEvent::ApplicationWindowIconChange:
        setIcon(icon());
        break;

    case QEvent::WindowStateChange: {
        QWindowPrivate * const d = d_func();
        emit windowStateChanged(d->windowState);
        d->updateVisibility();
        break;
    }

#if QT_CONFIG(tabletevent)
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        tabletEvent(static_cast<QTabletEvent *>(ev));
        break;
#endif

    case QEvent::Timer: {
        QWindowPrivate * const d = d_func();
        if (static_cast<QTimerEvent *>(ev)->timerId() == d->updateTimer) {
            killTimer(d->updateTimer);
            d->updateTimer = 0;
            d->deliverUpdateRequest();
        } else {
            QObject::event(ev);
        }
        break;
    }

    case QEvent::PlatformSurface: {
        if ((static_cast<QPlatformSurfaceEvent *>(ev))->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
#ifndef QT_NO_OPENGL
            QOpenGLContext *context = QOpenGLContext::currentContext();
            if (context && context->surface() == static_cast<QSurface *>(this))
                context->doneCurrent();
#endif
        }
        break;
    }

    default:
        return QObject::event(ev);
    }
    return true;
}

void QWindowPrivate::deliverUpdateRequest()
{
    Q_Q(QWindow);
    updateRequestPending = false;
    QEvent request(QEvent::UpdateRequest);
    QCoreApplication::sendEvent(q, &request);
}


void QWindow::requestUpdate()
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread(),
        "QWindow", "Updates can only be scheduled from the GUI (main) thread");

    QWindowPrivate * const d = d_func();
    if (d->updateRequestPending || !d->platformWindow)
        return;
    d->updateRequestPending = true;
    d->platformWindow->requestUpdate();
}

/*!
    Override this to handle key press events (\a ev).

    \sa keyReleaseEvent()
*/
void QWindow::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle key release events (\a ev).

    \sa keyPressEvent()
*/
void QWindow::keyReleaseEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus in events (\a ev).

    Focus in events are sent when the window receives keyboard focus.

    \sa focusOutEvent()
*/
void QWindow::focusInEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus out events (\a ev).

    Focus out events are sent when the window loses keyboard focus.

    \sa focusInEvent()
*/
void QWindow::focusOutEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse press events (\a ev).

    \sa mouseReleaseEvent()
*/
void QWindow::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse release events (\a ev).

    \sa mousePressEvent()
*/
void QWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse double click events (\a ev).

    \sa mousePressEvent(), QStyleHints::mouseDoubleClickInterval()
*/
void QWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse move events (\a ev).
*/
void QWindow::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
}

#if QT_CONFIG(wheelevent)
/*!
    Override this to handle mouse wheel or other wheel events (\a ev).
*/
void QWindow::wheelEvent(QWheelEvent *ev)
{
    ev->ignore();
}
#endif // QT_CONFIG(wheelevent)

/*!
    Override this to handle touch events (\a ev).
*/
void QWindow::touchEvent(QTouchEvent *ev)
{
    ev->ignore();
}

#if QT_CONFIG(tabletevent)
/*!
    Override this to handle tablet press, move, and release events (\a ev).

    Proximity enter and leave events are not sent to windows, they are
    delivered to the application instance.
*/
void QWindow::tabletEvent(QTabletEvent *ev)
{
    ev->ignore();
}
#endif


bool QWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}


QPoint QWindow::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWindow);
    // QTBUG-43252, prefer platform implementation for foreign windows.
    if (d->platformWindow
        && (d->platformWindow->isForeignWindow() || d->platformWindow->isEmbedded())) {
        return QHighDpi::fromNativeLocalPosition(d->platformWindow->mapToGlobal(QHighDpi::toNativeLocalPosition(pos, this)), this);
    }
    return pos + d->globalPosition();
}


QPoint QWindow::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWindow);
    // QTBUG-43252, prefer platform implementation for foreign windows.
    if (d->platformWindow
        && (d->platformWindow->isForeignWindow() || d->platformWindow->isEmbedded())) {
        return QHighDpi::fromNativeLocalPosition(d->platformWindow->mapFromGlobal(QHighDpi::toNativeLocalPosition(pos, this)), this);
    }
    return pos - d->globalPosition();
}

QPoint QWindowPrivate::globalPosition() const
{
    Q_Q(const QWindow);
    QPoint offset = q->position();
    for (const QWindow *p = q->parent(); p; p = p->parent()) {
        QPlatformWindow *pw = p->handle();
        if (pw && pw->isForeignWindow()) {
            // Use mapToGlobal() for foreign windows
            offset += p->mapToGlobal(QPoint(0, 0));
            break;
        } else {
            offset += p->position();
        }
    }
    return offset;
}

Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window)
{
    return window->d_func();
}

void QWindowPrivate::maybeQuitOnLastWindowClosed()
{
    if (!QCoreApplication::instance())
        return;

    Q_Q(QWindow);
    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    bool quitOnClose = QGuiApplication::quitOnLastWindowClosed() && !q->parent();
    QWindowList list = QGuiApplication::topLevelWindows();
    bool lastWindowClosed = true;
    for (int i = 0; i < list.size(); ++i) {
        QWindow *w = list.at(i);
        if (!w->isVisible() || w->transientParent() || w->type() == Qt::ToolTip)
            continue;
        lastWindowClosed = false;
        break;
    }
    if (lastWindowClosed) {
        QGuiApplicationPrivate::emitLastWindowClosed();
        if (quitOnClose) {
            QCoreApplicationPrivate *applicationPrivate = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()));
            applicationPrivate->maybeQuit();
        }
    }
}

QWindow *QWindowPrivate::topLevelWindow() const
{
    Q_Q(const QWindow);

    QWindow *window = const_cast<QWindow *>(q);

    while (window) {
        QWindow *parent = window->parent(QWindow::IncludeTransients);
        if (!parent)
            break;

        window = parent;
    }

    return window;
}


QWindow *QWindow::fromWinId(WId id)
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ForeignWindows)) {
        qWarning("QWindow::fromWinId(): platform plugin does not support foreign windows.");
        return 0;
    }

    QWindow *window = new QWindow;
    qt_window_private(window)->create(false, id);

    if (!window->handle()) {
        delete window;
        return nullptr;
    }

    return window;
}



void QWindow::alert(int msec)
{
    QWindowPrivate * const d = d_func();
    if (!d->platformWindow || d->platformWindow->isAlertState() || isActive())
        return;
    d->platformWindow->setAlertState(true);
    if (d->platformWindow->isAlertState() && msec)
        QTimer::singleShot(msec, this, SLOT(_q_clearAlert()));
}

void QWindowPrivate::_q_clearAlert()
{
    if (platformWindow && platformWindow->isAlertState())
        platformWindow->setAlertState(false);
}

void QWindow::setCursor(const QCursor &cursor)
{
    QWindowPrivate * const d = d_func();
    d->setCursor(&cursor);
}

void QWindow::unsetCursor()
{
    QWindowPrivate * const d = d_func();
    d->setCursor(0);
}

QCursor QWindow::cursor() const
{
    Q_D(const QWindow);
    return d->cursor;
}
void QWindowPrivate::setCursor(const QCursor *newCursor)
{

    Q_Q(QWindow);
    if (newCursor) {
        const Qt::CursorShape newShape = newCursor->shape();
        if (newShape <= Qt::LastCursor && hasCursor && newShape == cursor.shape())
            return; // Unchanged and no bitmap/custom cursor.
        cursor = *newCursor;
        hasCursor = true;
    } else {
        if (!hasCursor)
            return;
        cursor = QCursor(Qt::ArrowCursor);
        hasCursor = false;
    }
    // Only attempt to emit signal if there is an actual platform cursor
    if (applyCursor()) {
        QEvent event(QEvent::CursorChange);
        QGuiApplication::sendEvent(q, &event);
    }
}

bool QWindowPrivate::applyCursor()
{
    Q_Q(QWindow);
    if (QScreen *screen = q->screen()) {
        if (QPlatformCursor *platformCursor = screen->handle()->cursor()) {
            if (!platformWindow)
                return true;
            QCursor *c = QGuiApplication::overrideCursor();
            if (!c && hasCursor)
                c = &cursor;
            platformCursor->changeCursor(c, q);
            return true;
        }
    }
    return false;
}

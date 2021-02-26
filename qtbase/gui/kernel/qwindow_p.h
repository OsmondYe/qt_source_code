

#ifndef QWINDOW_P_H
#define QWINDOW_P_H


QT_BEGIN_NAMESPACE


class QWindowPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWindow)

public:
    enum PositionPolicy
    {
        WindowFrameInclusive,
        WindowFrameExclusive
    };

    QWindowPrivate()
        : QObjectPrivate()
        , surfaceType(QWindow::RasterSurface)
        , windowFlags(Qt::Window)
        , parentWindow(0)
        , platformWindow(0)
        , visible(false)
        , visibilityOnDestroy(false)
        , exposed(false)
        , windowState(Qt::WindowNoState)
        , visibility(QWindow::Hidden)
        , resizeEventPending(true)
        , receivedExpose(false)
        , positionPolicy(WindowFrameExclusive)
        , positionAutomatic(true)
        , contentOrientation(Qt::PrimaryOrientation)
        , opacity(qreal(1.0))
        , minimumSize(0, 0)
        , maximumSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX)
        , modality(Qt::NonModal)
        , blockedByModalWindow(false)
        , updateRequestPending(false)
        , updateTimer(0)
        , transientParent(0)
        , topLevelScreen(0)
#ifndef QT_NO_CURSOR
        , cursor(Qt::ArrowCursor)
        , hasCursor(false)
#endif
        , compositing(false)
    {
        isWindow = true;
    }

    ~QWindowPrivate()
    {
    }

    void init(QScreen *targetScreen = nullptr);

    void maybeQuitOnLastWindowClosed();
#ifndef QT_NO_CURSOR
    void setCursor(const QCursor *c = 0);
    bool applyCursor();
#endif

    void deliverUpdateRequest();

    QPoint globalPosition() const;

    QWindow *topLevelWindow() const;

    virtual QWindow *eventReceiver() { Q_Q(QWindow); return q; }

    void updateVisibility();
    void _q_clearAlert();

    enum SiblingPosition { PositionTop, PositionBottom };
    void updateSiblingPosition(SiblingPosition);

    bool windowRecreationRequired(QScreen *newScreen) const;
    void create(bool recursive, WId nativeHandle = 0);
    void destroy();
    void setTopLevelScreen(QScreen *newScreen, bool recreate);
    void connectToScreen(QScreen *topLevelScreen);
    void disconnectFromScreen();
    void emitScreenChangedRecursion(QScreen *newScreen);
    QScreen *screenForGeometry(const QRect &rect);

    virtual void clearFocusObject();
    virtual QRectF closestAcceptableGeometry(const QRectF &rect) const;

    virtual void processSafeAreaMarginsChanged() {};

    bool isPopup() const { return (windowFlags & Qt::WindowType_Mask) == Qt::Popup; }

    static QWindowPrivate *get(QWindow *window) { return window->d_func(); }

    QWindow::SurfaceType surfaceType;
    Qt::WindowFlags windowFlags;
    QWindow *parentWindow;
    QPlatformWindow *platformWindow;
    bool visible;
    bool visibilityOnDestroy;
    bool exposed;
    QSurfaceFormat requestedFormat;
    QString windowTitle;
    QString windowFilePath;
    QIcon windowIcon;
    QRect geometry;
    Qt::WindowState windowState;
    QWindow::Visibility visibility;
    bool resizeEventPending;
    bool receivedExpose;
    PositionPolicy positionPolicy;
    bool positionAutomatic;
    Qt::ScreenOrientation contentOrientation;
    qreal opacity;
    QRegion mask;

    QSize minimumSize;
    QSize maximumSize;
    QSize baseSize;
    QSize sizeIncrement;

    Qt::WindowModality modality;
    bool blockedByModalWindow;

    bool updateRequestPending;
    int updateTimer;

    QPointer<QWindow> transientParent;
    QPointer<QScreen> topLevelScreen;

#ifndef QT_NO_CURSOR
    QCursor cursor;
    bool hasCursor;
#endif

    bool compositing;
    QElapsedTimer lastComposeTime;
};


QT_END_NAMESPACE

#endif // QWINDOW_P_H

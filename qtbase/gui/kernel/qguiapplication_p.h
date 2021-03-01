#ifndef QGUIAPPLICATION_P_H
#define QGUIAPPLICATION_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qguiapplication.h>

#include <QtCore/QPointF>
#include <QtCore/private/qcoreapplication_p.h>

#include <QtCore/private/qthread_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include "private/qshortcutmap_p.h"
#include <qicon.h>

QT_BEGIN_NAMESPACE

class QColorProfile;
class QPlatformIntegration;
class QPlatformTheme;
class QPlatformDragQtResponse;

class QDrag;

class QInputDeviceManager;

class  QGuiApplicationPrivate : public QCoreApplicationPrivate
{
    //Q_DECLARE_PUBLIC(QGuiApplication)
public:

	static QPlatformTheme *platform_theme;
    static Qt::KeyboardModifiers modifier_buttons;
    static Qt::MouseButtons mouse_buttons;

    static QPlatformIntegration *platform_integration;
    static QIcon *app_icon;
    static QString *platform_name;
    static QString *displayName;
    static QString *desktopFileName;
	static Qt::MouseButtons buttons;
    static ulong mousePressTime;
    static Qt::MouseButton mousePressButton;
    static int mousePressX;
    static int mousePressY;
    static int mouse_double_click_distance;
    static QPointF lastCursorPosition;
    static QWindow *currentMouseWindow;
    static QWindow *currentMousePressWindow;
    static Qt::ApplicationState applicationState;
    static bool highDpiScalingUpdated;
	static QClipboard *qt_clipboard;
    static QPalette *app_pal;
    static QWindowList window_list;
    static QWindow *focus_window;

    QList<QCursor> cursor_list;
    static QList<QScreen *> screen_list;
    static QFont *app_font;
    static QString styleOverride;
    static QStyleHints *styleHints;
    static bool obey_desktop_settings;
    QInputMethod *inputMethod;

    QString firstWindowTitle;
    QIcon forcedWindowIcon;

    static QList<QObject *> generic_plugin_list;

    QShortcutMap shortcutMap;

    static bool is_fallback_session_management_enabled;
    QSessionManager *session_manager;
    bool is_session_restored;
    bool is_saving_session;

	static QGuiApplicationPrivate *self;
    static QTouchDevice *m_fakeTouchDevice;
    static int m_fakeMouseSourcePointId;
    QAtomicPointer<QColorProfile> m_a8ColorProfile;
    QAtomicPointer<QColorProfile> m_a32ColorProfile;

    bool ownGlobalShareContext;

    static QInputDeviceManager *m_inputDeviceManager;
	QWindowList modalWindowList;
	static QVector<TabletPointData> tabletDevicePoints;
	QHash<ActiveTouchPointsKey, ActiveTouchPointsValue> activeTouchPoints;
    QEvent::Type lastTouchType;
	QHash<QWindow *, SynthesizedMouseData> synthesizedMousePoints;


    QGuiApplicationPrivate(int &argc, char **argv, int flags);
    ~QGuiApplicationPrivate();

    void init();

    void createPlatformIntegration();
    void createEventDispatcher() ;
    void eventDispatcherReady() ;

    virtual void notifyLayoutDirectionChange();
    virtual void notifyActiveWindowChange(QWindow *previous);

    virtual bool shouldQuit() Q_DECL_OVERRIDE;

    bool shouldQuitInternal(const QWindowList &processedWindows);
    virtual bool tryCloseAllWindows();

    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }
    
    static QPlatformTheme *platformTheme()
    { return platform_theme; }

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    {
        if (QCoreApplication::instance())
            return QCoreApplication::instance()->d_func()->threadData->eventDispatcher.load();
        else
            return 0;
    }

    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);
    static void processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *e);
    static void processWindowScreenChangedEvent(QWindowSystemInterfacePrivate::WindowScreenChangedEvent *e);

    static void processSafeAreaMarginsChangedEvent(QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void updateFilteredScreenOrientation(QScreen *screen);
    static void reportScreenOrientationChange(QScreen *screen);
    static void reportScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e);
    static void reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
    static void reportLogicalDotsPerInchChange(QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e);
    static void reportRefreshRateChange(QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e);
    static void processThemeChanged(QWindowSystemInterfacePrivate::ThemeChangeEvent *tce);

    static void processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e);

    static void processFileOpenEvent(QWindowSystemInterfacePrivate::FileOpenEvent *e);

    static void processTabletEvent(QWindowSystemInterfacePrivate::TabletEvent *e);
    static void processTabletEnterProximityEvent(QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e);
    static void processTabletLeaveProximityEvent(QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e);


    static void processGestureEvent(QWindowSystemInterfacePrivate::GestureEvent *e);


    static void processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e);

    static void processContextMenuEvent(QWindowSystemInterfacePrivate::ContextMenuEvent *e);



    static QPlatformDragQtResponse processDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);
    static QPlatformDropQtResponse processDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);


    static bool processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result);

    static void sendQWindowEventToQPlatformWindow(QWindow *window, QEvent *event);

    static inline Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        if (!(alignment & Qt::AlignHorizontal_Mask))
            alignment |= Qt::AlignLeft;
        if (!(alignment & Qt::AlignAbsolute) && (alignment & (Qt::AlignLeft | Qt::AlignRight))) {
            if (direction == Qt::RightToLeft)
                alignment ^= (Qt::AlignLeft | Qt::AlignRight);
            alignment |= Qt::AlignAbsolute;
        }
        return alignment;
    }

    static void emitLastWindowClosed();

    QPixmap getPixmapCursor(Qt::CursorShape cshape);

    void _q_updateFocusObject(QObject *object);

    static QGuiApplicationPrivate *instance() { return self; }

    static void showModalWindow(QWindow *window);
    static void hideModalWindow(QWindow *window);
    static void updateBlockedStatus(QWindow *window);
    virtual bool isWindowBlocked(QWindow *window, QWindow **blockingWindow = 0) const;
    virtual bool popupActive() { return false; }

    struct TabletPointData {
        TabletPointData(qint64 devId = 0) : deviceId(devId), state(Qt::NoButton), target(Q_NULLPTR) {}
        qint64 deviceId;
        Qt::MouseButtons state;
        QWindow *target;
    };
    
    static TabletPointData &tabletDevicePoint(qint64 deviceId);

    void commitData();
    void saveState();

    struct ActiveTouchPointsKey {
        ActiveTouchPointsKey(QTouchDevice *dev, int id) : device(dev), touchPointId(id) { }
        QTouchDevice *device;
        int touchPointId;
    };
    struct ActiveTouchPointsValue {
        QPointer<QWindow> window;
        QPointer<QObject> target;
        QTouchEvent::TouchPoint touchPoint;
    };

    struct SynthesizedMouseData {
        SynthesizedMouseData(const QPointF &p, const QPointF &sp, QWindow *w)
            : pos(p), screenPos(sp), window(w) { }
        QPointF pos;
        QPointF screenPos;
        QPointer<QWindow> window;
    };

    static int mouseEventCaps(QMouseEvent *event);
    static QVector2D mouseEventVelocity(QMouseEvent *event);
    static void setMouseEventCapsAndVelocity(QMouseEvent *event, int caps, const QVector2D &velocity);

    static Qt::MouseEventSource mouseEventSource(const QMouseEvent *event);
    static void setMouseEventSource(QMouseEvent *event, Qt::MouseEventSource source);

    static Qt::MouseEventFlags mouseEventFlags(const QMouseEvent *event);
    static void setMouseEventFlags(QMouseEvent *event, Qt::MouseEventFlags flags);

    static QInputDeviceManager *inputDeviceManager();

    const QColorProfile *colorProfileForA8Text();
    const QColorProfile *colorProfileForA32Text();

    // hook reimplemented in QApplication to apply the QStyle function on the QIcon
    virtual QPixmap applyQIconStyleHelper(QIcon::Mode, const QPixmap &basePixmap) const { return basePixmap; }

    virtual void notifyWindowIconChanged();

    static void applyWindowGeometrySpecificationTo(QWindow *window);

    static void setApplicationState(Qt::ApplicationState state, bool forcePropagate = false);

protected:
    virtual void notifyThemeChanged();
    bool tryCloseRemainingWindows(QWindowList processedWindows);

    virtual void notifyDragStarted(const QDrag *);

private:
    friend class QDragManager;


};

 uint qHash(const QGuiApplicationPrivate::ActiveTouchPointsKey &k);

 bool operator==(const QGuiApplicationPrivate::ActiveTouchPointsKey &a,
                             const QGuiApplicationPrivate::ActiveTouchPointsKey &b);

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_P_H

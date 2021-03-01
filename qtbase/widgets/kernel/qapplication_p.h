#ifndef QAPPLICATION_P_H
#define QAPPLICATION_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qapplication.h"
#include "QtGui/qevent.h"
#include "QtGui/qfont.h"
#include "QtGui/qcursor.h"
#include "QtGui/qregion.h"
#include "QtGui/qwindow.h"
#include "qwidget.h"
#include <qpa/qplatformnativeinterface.h>
#include "QtCore/qmutex.h"
#include "QtCore/qtranslator.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qhash.h"
#include "QtCore/qpointer.h"
#include "private/qcoreapplication_p.h"
#include "QtCore/qpoint.h"
#include <QTime>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <qpa/qplatformintegration.h>
#include "private/qguiapplication_p.h"

QT_BEGIN_NAMESPACE

class QClipboard;
class QGraphicsScene;
class QObject;
class QWidget;
class QSocketNotifier;
class QTouchDevice;
class QGestureManager;


extern  bool qt_is_gui_used;

extern QClipboard *qt_clipboard;


typedef QHash<QByteArray, QFont> FontHash;
FontHash *qt_app_fonts_hash();

typedef QHash<QByteArray, QPalette> PaletteHash;
PaletteHash *qt_app_palettes_hash();



class  QApplicationPrivate : public QGuiApplicationPrivate
{
    //Q_DECLARE_PUBLIC(QApplication)
public:
    QApplicationPrivate(int &argc, char **argv, int flags);
    ~QApplicationPrivate();

    virtual void notifyLayoutDirectionChange() ;
    virtual void notifyActiveWindowChange(QWindow *) ;

    virtual bool shouldQuit() ;
    bool tryCloseAllWindows() ;


    static bool autoSipEnabled;
    static QString desktopStyleKey();

    void createEventDispatcher() ;
    static void dispatchEnterLeave(QWidget *enter, QWidget *leave, const QPointF &globalPosF);

    void notifyWindowIconChanged() ;

    //modality
    bool isWindowBlocked(QWindow *window, QWindow **blockingWindow = 0) const Q_DECL_OVERRIDE;
    static bool isBlockedByModal(QWidget *widget);
    static bool modalState();
    static bool tryModalHelper(QWidget *widget, QWidget **rettop = 0);

    //style
    static bool usesNativeStyle()
    {
        return !overrides_native_style;
    }

    bool notify_helper(QObject *receiver, QEvent * e);

    void init( );
    void initialize();
    void process_cmdline();

    static bool inPopupMode();
    bool popupActive() Q_DECL_OVERRIDE { return inPopupMode(); }
    void closePopup(QWidget *popup);
    void openPopup(QWidget *popup);
    static void setFocusWidget(QWidget *focus, Qt::FocusReason reason);
    static QWidget *focusNextPrevChild_helper(QWidget *toplevel, bool next,
                                              bool *wrappingOccurred = 0);
    // Maintain a list of all scenes to ensure font and palette propagation to
    // all scenes.
    QList<QGraphicsScene *> scene_list;


    QBasicTimer toolTipWakeUp, toolTipFallAsleep;
    QPoint toolTipPos, toolTipGlobalPos, hoverGlobalPos;
    QPointer<QWidget> toolTipWidget;

    static QSize app_strut;
    static QWidgetList *popupWidgets;
    static QStyle *app_style;
    static bool overrides_native_style;
    static QPalette *sys_pal;
    static QPalette *set_pal;

protected:
    void notifyThemeChanged() ;

    void notifyDragStarted(const QDrag *) ;
public:
    static QFont *sys_font;
    static QFont *set_font;
    static QWidget *main_widget;
    static QWidget *focus_widget;
    static QWidget *hidden_focus_widget;
    static QWidget *active_window;

    static int  wheel_scroll_lines;
    static QPointer<QWidget> wheel_widget;

    static int enabledAnimations; // Combination of QPlatformTheme::UiEffect
    static bool widgetCount; // Coupled with -widgetcount switch

    static void setSystemPalette(const QPalette &pal);
    static void setPalette_helper(const QPalette &palette, const char* className, bool clearWidgetPaletteHash);
    static void initializeWidgetPaletteHash();
    static void initializeWidgetFontHash();
    static void setSystemFont(const QFont &font);

    static QApplicationPrivate *instance() { return self; }


    static QWidget *oldEditFocus;
    static Qt::NavigationMode navigationMode;

    static QString styleSheet;

    static QPointer<QWidget> leaveAfterRelease;
    static QWidget *pickMouseReceiver(QWidget *candidate, const QPoint &windowPos, QPoint *pos,
                                      QEvent::Type type, Qt::MouseButtons buttons,
                                      QWidget *buttonDown, QWidget *alienWidget);
    static bool sendMouseEvent(QWidget *receiver, QMouseEvent *event, QWidget *alienWidget,
                               QWidget *native, QWidget **buttonDown, QPointer<QWidget> &lastMouseReceiver,
                               bool spontaneous = true);
    void sendSyntheticEnterLeave(QWidget *widget);

    static QWindow *windowForWidget(const QWidget *widget)
    {
        if (QWindow *window = widget->windowHandle())
            return window;
        if (const QWidget *nativeParent = widget->nativeParentWidget())
            return nativeParent->windowHandle();
        return 0;
    }

    static HWND getHWNDForWidget(const QWidget *widget)
    {
        if (QWindow *window = windowForWidget(widget))
            if (window->handle())
                return static_cast<HWND> (QGuiApplication::platformNativeInterface()->
                                          nativeResourceForWindow(QByteArrayLiteral("handle"), window));
        return 0;
    }



    QGestureManager *gestureManager;
    QWidget *gestureWidget;

    static bool updateTouchPointsForWidget(QWidget *widget, QTouchEvent *touchEvent);
    void initializeMultitouch();
    void initializeMultitouch_sys();
    void cleanupMultitouch();
    void cleanupMultitouch_sys();
    QWidget *findClosestTouchPointTarget(QTouchDevice *device, const QTouchEvent::TouchPoint &touchPoint);
    void appendTouchPoint(const QTouchEvent::TouchPoint &touchPoint);
    void removeTouchPoint(int touchPointId);
    void activateImplicitTouchGrab(QWidget *widget, QTouchEvent *touchBeginEvent);
    static bool translateRawTouchEvent(QWidget *widget,
                                       QTouchDevice *device,
                                       const QList<QTouchEvent::TouchPoint> &touchPoints,
                                       ulong timestamp);
    static void translateTouchCancel(QTouchDevice *device, ulong timestamp);

    QPixmap applyQIconStyleHelper(QIcon::Mode mode, const QPixmap& base) const Q_DECL_OVERRIDE;
private:
    static QApplicationPrivate *self;
    static bool tryCloseAllWidgetWindows(QWindowList *processedWindows);

    static void giveFocusAccordingToFocusPolicy(QWidget *w, QEvent *event, QPoint localPos);
    static bool shouldSetFocus(QWidget *w, Qt::FocusPolicy policy);


    static bool isAlien(QWidget *);
};

  extern void qt_qpa_set_cursor(QWidget * w, bool force);

QT_END_NAMESPACE

#endif // QAPPLICATION_P_H

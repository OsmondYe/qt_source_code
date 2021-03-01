#ifndef QWIDGETWINDOW_P_H
#define QWIDGETWINDOW_P_H



#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtGui/qwindow.h>

#include <QtCore/private/qobject_p.h>
#include <QtGui/private/qevent_p.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE


class QCloseEvent;
class QMoveEvent;

class QWidgetWindow : public QWindow
{
    Q_OBJECT
public:
    QWidgetWindow(QWidget *widget);
    ~QWidgetWindow();

    QWidget *widget() const { return m_widget; }

    QObject *focusObject() const Q_DECL_OVERRIDE;
protected:
    bool event(QEvent *) Q_DECL_OVERRIDE;

    void handleCloseEvent(QCloseEvent *);
    void handleEnterLeaveEvent(QEvent *);
    void handleFocusInEvent(QFocusEvent *);
    void handleKeyEvent(QKeyEvent *);
    void handleMouseEvent(QMouseEvent *);
    void handleNonClientAreaMouseEvent(QMouseEvent *);
    void handleTouchEvent(QTouchEvent *);
    void handleMoveEvent(QMoveEvent *);
    void handleResizeEvent(QResizeEvent *);
#if QT_CONFIG(wheelevent)
    void handleWheelEvent(QWheelEvent *);
#endif
#ifndef QT_NO_DRAGANDDROP
    void handleDragEnterMoveEvent(QDragMoveEvent *);
    void handleDragLeaveEvent(QDragLeaveEvent *);
    void handleDropEvent(QDropEvent *);
#endif
    void handleExposeEvent(QExposeEvent *);
    void handleWindowStateChangedEvent(QWindowStateChangeEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;
#if QT_CONFIG(tabletevent)
    void handleTabletEvent(QTabletEvent *);
#endif
#ifndef QT_NO_GESTURES
    void handleGestureEvent(QNativeGestureEvent *);
#endif
#ifndef QT_NO_CONTEXTMENU
    void handleContextMenuEvent(QContextMenuEvent *);
#endif

private slots:
    void updateObjectName();
    void handleScreenChange();

private:
    void repaintWindow();
    bool updateSize();
    bool updatePos();
    void updateMargins();
    void updateNormalGeometry();

    enum FocusWidgets {
        FirstFocusWidget,
        LastFocusWidget
    };
    QWidget *getFocusWidget(FocusWidgets fw);

    QPointer<QWidget> m_widget;
    QPointer<QWidget> m_implicit_mouse_grabber;
#ifndef QT_NO_DRAGANDDROP
    QPointer<QWidget> m_dragTarget;
#endif
};

QT_END_NAMESPACE

#endif // QWIDGETWINDOW_P_H

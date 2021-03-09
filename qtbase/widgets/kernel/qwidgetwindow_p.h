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

// 就是一个窗口, 但叫了widget, 持有widget对象
class QWidgetWindow : public QWindow
{
    //Q_OBJECT

	QPointer<QWidget> m_widget;
    QPointer<QWidget> m_implicit_mouse_grabber;
	// dragdrop only
    QPointer<QWidget> m_dragTarget;
public:
    QWidgetWindow(QWidget *widget);
    ~QWidgetWindow();

    QWidget *widget() const { return m_widget; }

    QObject *focusObject() const Q_DECL_OVERRIDE;
	
protected:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

    void handleCloseEvent(QCloseEvent *);
    void handleEnterLeaveEvent(QEvent *);
    void handleFocusInEvent(QFocusEvent *);
    void handleKeyEvent(QKeyEvent *);
    void handleMouseEvent(QMouseEvent *);
    void handleNonClientAreaMouseEvent(QMouseEvent *);
    void handleTouchEvent(QTouchEvent *);
    void handleMoveEvent(QMoveEvent *);
    void handleResizeEvent(QResizeEvent *);
    void handleWheelEvent(QWheelEvent *);

	// DragDrop
    void handleDragEnterMoveEvent(QDragMoveEvent *);
    void handleDragLeaveEvent(QDragLeaveEvent *);
    void handleDropEvent(QDropEvent *);
    void handleExposeEvent(QExposeEvent *);
    void handleWindowStateChangedEvent(QWindowStateChangeEvent *event);

	//CONTEXTMENU
    void handleContextMenuEvent(QContextMenuEvent *);

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


};

QT_END_NAMESPACE

#endif // QWIDGETWINDOW_P_H

#ifndef QAPPLICATION_H
#define QAPPLICATION_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtGui/qcursor.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE


class QDesktopWidget;
class QStyle;
class QEventLoop;
class QIcon;
template <typename T> class QList;
class QLocale;
class QPlatformNativeInterface;

class QApplication;
class QApplicationPrivate;


#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))

// oye:  比 QGuiApp 更进一步
class  QApplication : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    Q_PROPERTY(int cursorFlashTime READ cursorFlashTime WRITE setCursorFlashTime)
    Q_PROPERTY(int doubleClickInterval  READ doubleClickInterval WRITE setDoubleClickInterval)
    Q_PROPERTY(int keyboardInputInterval READ keyboardInputInterval WRITE setKeyboardInputInterval)
    Q_PROPERTY(int wheelScrollLines  READ wheelScrollLines WRITE setWheelScrollLines)
    Q_PROPERTY(QSize globalStrut READ globalStrut WRITE setGlobalStrut)
    Q_PROPERTY(int startDragTime  READ startDragTime WRITE setStartDragTime)
    Q_PROPERTY(int startDragDistance  READ startDragDistance WRITE setStartDragDistance)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(bool autoSipEnabled READ autoSipEnabled WRITE setAutoSipEnabled)

public:

    QApplication(int &argc, char **argv, int = ApplicationFlags);
    virtual ~QApplication();

    static QStyle *style();
    static void setStyle(QStyle*);
    static QStyle *setStyle(const QString&);
    enum ColorSpec { NormalColor=0, CustomColor=1, ManyColor=2 };


    using QGuiApplication::palette;
    static QPalette palette(const QWidget *);
    static QPalette palette(const char *className);
    static void setPalette(const QPalette &, const char* className = Q_NULLPTR);
    static QFont font();
    static QFont font(const QWidget*);
    static QFont font(const char *className);
    static void setFont(const QFont &, const char* className = Q_NULLPTR);
    static QFontMetrics fontMetrics();

    static void setWindowIcon(const QIcon &icon);
    static QIcon windowIcon();


    static QWidgetList allWidgets();
    static QWidgetList topLevelWidgets();

    static QDesktopWidget *desktop();

    static QWidget *activePopupWidget();
    static QWidget *activeModalWidget();
    static QWidget *focusWidget();

    static QWidget *activeWindow();
    static void setActiveWindow(QWidget* act);

    static QWidget *widgetAt(const QPoint &p);
    static inline QWidget *widgetAt(int x, int y) { return widgetAt(QPoint(x, y)); }
    static QWidget *topLevelAt(const QPoint &p);
    static inline QWidget *topLevelAt(int x, int y)  { return topLevelAt(QPoint(x, y)); }


    static void beep();
    static void alert(QWidget *widget, int duration = 0);

    static void setCursorFlashTime(int);
    static int cursorFlashTime();

    static void setDoubleClickInterval(int);
    static int doubleClickInterval();

    static void setKeyboardInputInterval(int);
    static int keyboardInputInterval();


    static void setWheelScrollLines(int);
    static int wheelScrollLines();

    static void setGlobalStrut(const QSize &);
    static QSize globalStrut();

    static void setStartDragTime(int ms);
    static int startDragTime();
    static void setStartDragDistance(int l);
    static int startDragDistance();

    static bool isEffectEnabled(Qt::UIEffect);
    static void setEffectEnabled(Qt::UIEffect, bool enable = true);


    static int exec();
	
	// oye isWidget -> use it,  
	//     isWindow -> use GUIAppication::notify,
	//     others   -> use QCoreAppication::notify
    bool notify(QObject *, QEvent *) override;

#ifdef QT_KEYPAD_NAVIGATION
    static Q_DECL_DEPRECATED void setKeypadNavigationEnabled(bool);
    static bool keypadNavigationEnabled();
    static void setNavigationMode(Qt::NavigationMode mode);
    static Qt::NavigationMode navigationMode();
#endif

Q_SIGNALS:
    void focusChanged(QWidget *old, QWidget *now);

public:
    QString styleSheet() const;
public Q_SLOTS:

    void setStyleSheet(const QString& sheet);
    void setAutoSipEnabled(const bool enabled);
    bool autoSipEnabled() const;
    static void closeAllWindows();
    static void aboutQt();

protected:
    bool event(QEvent *) override;
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *) override;

private:
    Q_DISABLE_COPY(QApplication)
    Q_DECLARE_PRIVATE(QApplication)

    friend class QGraphicsWidget;
    friend class QGraphicsItem;
    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QWidgetWindow;
    friend class QTranslator;
    friend class QWidgetAnimator;
    friend class QShortcut;
    friend class QLineEdit;
    friend class QWidgetTextControl;
    friend class QAction;

    friend class QGestureManager;

};

QT_END_NAMESPACE

#endif // QAPPLICATION_H

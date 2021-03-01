#ifndef QGUIAPPLICATION_H
#define QGUIAPPLICATION_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qinputmethod.h>
#include <QtCore/qlocale.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE


class QSessionManager;
class QGuiApplicationPrivate;
class QPlatformNativeInterface;
class QPlatformIntegration;
class QPalette;
class QScreen;
class QStyleHints;


#define qApp (static_cast<QGuiApplication *>(QCoreApplication::instance()))

#define qGuiApp (static_cast<QGuiApplication *>(QCoreApplication::instance()))

class  QGuiApplication : public QCoreApplication
{
    //Q_OBJECT
    //Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    //Q_PROPERTY(QString applicationDisplayName READ applicationDisplayName WRITE setApplicationDisplayName NOTIFY applicationDisplayNameChanged)
    //Q_PROPERTY(QString desktopFileName READ desktopFileName WRITE setDesktopFileName)
    //Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged)
    //Q_PROPERTY(QString platformName READ platformName STORED false)
    //Q_PROPERTY(bool quitOnLastWindowClosed  READ quitOnLastWindowClosed WRITE setQuitOnLastWindowClosed)
    //Q_PROPERTY(QScreen *primaryScreen READ primaryScreen NOTIFY primaryScreenChanged STORED false)


	// oye
	//Q_PRIVATE_SLOT(d_func(), void _q_updateFocusObject(QObject *object))
	QGuiApplicationPrivate* d_ptr ;
	QGuiApplicationPrivate*  d_func();

public:

    QGuiApplication(int &argc, char **argv, int = ApplicationFlags);
    virtual ~QGuiApplication();

    static void setApplicationDisplayName(const QString &name);
    static QString applicationDisplayName();

    static void setDesktopFileName(const QString &name);
    static QString desktopFileName();

    static QWindowList allWindows();
    static QWindowList topLevelWindows();
    static QWindow *topLevelAt(const QPoint &pos);

    static void setWindowIcon(const QIcon &icon);
    static QIcon windowIcon();

    static QString platformName();

    static QWindow *modalWindow();

    static QWindow *focusWindow();
    static QObject *focusObject();

    static QScreen *primaryScreen();
    static QList<QScreen *> screens();
    qreal devicePixelRatio() const;

#ifndef QT_NO_CURSOR
    static QCursor *overrideCursor();
    static void setOverrideCursor(const QCursor &);
    static void changeOverrideCursor(const QCursor &);
    static void restoreOverrideCursor();
#endif

    static QFont font();
    static void setFont(const QFont &);

#ifndef QT_NO_CLIPBOARD
    static QClipboard *clipboard();
#endif

    static QPalette palette();
    static void setPalette(const QPalette &pal);

    static Qt::KeyboardModifiers keyboardModifiers();
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    static Qt::MouseButtons mouseButtons();

    static void setLayoutDirection(Qt::LayoutDirection direction);
    static Qt::LayoutDirection layoutDirection();

    static inline bool isRightToLeft() { return layoutDirection() == Qt::RightToLeft; }
    static inline bool isLeftToRight() { return layoutDirection() == Qt::LeftToRight; }

    static QStyleHints *styleHints();
    static void setDesktopSettingsAware(bool on);
    static bool desktopSettingsAware();

    static QInputMethod *inputMethod();

    static QPlatformNativeInterface *platformNativeInterface();

    static QFunctionPointer platformFunction(const QByteArray &function);

    static void setQuitOnLastWindowClosed(bool quit);
    static bool quitOnLastWindowClosed();

    static Qt::ApplicationState applicationState();

    static int exec();
    bool notify(QObject *, QEvent *) Q_DECL_OVERRIDE;

#ifndef QT_NO_SESSIONMANAGER
    // session management
    bool isSessionRestored() const;
    QString sessionId() const;
    QString sessionKey() const;
    bool isSavingSession() const;

    static bool isFallbackSessionManagementEnabled();
    static void setFallbackSessionManagementEnabled(bool);
#endif

    static void sync();
Q_SIGNALS:
    void fontDatabaseChanged();
    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);
    void primaryScreenChanged(QScreen *screen);
    void lastWindowClosed();
    void focusObjectChanged(QObject *focusObject);
    void focusWindowChanged(QWindow *focusWindow);
    void applicationStateChanged(Qt::ApplicationState state);
    void layoutDirectionChanged(Qt::LayoutDirection direction);
#ifndef QT_NO_SESSIONMANAGER
    void commitDataRequest(QSessionManager &sessionManager);
    void saveStateRequest(QSessionManager &sessionManager);
#endif
    void paletteChanged(const QPalette &pal);
    void applicationDisplayNameChanged();

protected:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *) override;

    QGuiApplication(QGuiApplicationPrivate &p);

private:
    //Q_DISABLE_COPY(QGuiApplication)
    //Q_DECLARE_PRIVATE(QGuiApplication)



#ifndef QT_NO_GESTURES
    friend class QGestureManager;
#endif
    friend class QFontDatabasePrivate;
    friend class QPlatformIntegration;
#ifndef QT_NO_SESSIONMANAGER
    friend class QPlatformSessionManager;
#endif
};

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_H

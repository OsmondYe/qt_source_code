#include <QtGui/qtguiglobal.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qrect.h>
#include <QtCore/qmargins.h>
#include <QtCore/qstring.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qwindow.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformsurface.h>

class QPlatformScreen;
class QPlatformWindowPrivate;
class QWindow;
class QIcon;
class QRegion;

class Q_GUI_EXPORT QPlatformWindow : public QPlatformSurface
{
    Q_DECLARE_PRIVATE(QPlatformWindow)
public:
    explicit QPlatformWindow(QWindow *window);
    virtual ~QPlatformWindow();

    QWindow *window() const;
    QPlatformWindow *parent() const;

    QPlatformScreen *screen() const;

    virtual QSurfaceFormat format() const Q_DECL_OVERRIDE;

    virtual void setGeometry(const QRect &rect);
    virtual QRect geometry() const;
    virtual QRect normalGeometry() const;

    virtual QMargins frameMargins() const;
    virtual QMargins safeAreaMargins() const;

    virtual void setVisible(bool visible);
    virtual void setWindowFlags(Qt::WindowFlags flags);
    virtual void setWindowState(Qt::WindowState state);

    virtual WId winId() const;
    virtual void setParent(const QPlatformWindow *window);

    virtual void setWindowTitle(const QString &title);
    virtual void setWindowFilePath(const QString &title);
    virtual void setWindowIcon(const QIcon &icon);
    virtual void raise();
    virtual void lower();

    virtual bool isExposed() const;
    virtual bool isActive() const;
    virtual bool isAncestorOf(const QPlatformWindow *child) const;
    virtual bool isEmbedded() const;
    virtual bool isForeignWindow() const { return window()->type() == Qt::ForeignWindow; };
    virtual QPoint mapToGlobal(const QPoint &pos) const;
    virtual QPoint mapFromGlobal(const QPoint &pos) const;

    virtual void propagateSizeHints();

    virtual void setOpacity(qreal level);
    virtual void setMask(const QRegion &region);
    virtual void requestActivateWindow();

    virtual void handleContentOrientationChange(Qt::ScreenOrientation orientation);

    virtual qreal devicePixelRatio() const;

    virtual bool setKeyboardGrabEnabled(bool grab);
    virtual bool setMouseGrabEnabled(bool grab);

    virtual bool setWindowModified(bool modified);

    virtual void windowEvent(QEvent *event);

    virtual bool startSystemResize(const QPoint &pos, Qt::Corner corner);

    virtual void setFrameStrutEventsEnabled(bool enabled);
    virtual bool frameStrutEventsEnabled() const;

    virtual void setAlertState(bool enabled);
    virtual bool isAlertState() const;

    virtual void invalidateSurface();

    static QRect initialGeometry(const QWindow *w,
        const QRect &initialGeometry, int defaultWidth, int defaultHeight);

    virtual void requestUpdate();

    // Window property accessors. Platform plugins should use these
    // instead of accessing QWindow directly.
    QSize windowMinimumSize() const;
    QSize windowMaximumSize() const;
    QSize windowBaseSize() const;
    QSize windowSizeIncrement() const;
    QRect windowGeometry() const;
    QRect windowFrameGeometry() const;
    QRectF windowClosestAcceptableGeometry(const QRectF &nativeRect) const;
    static QRectF closestAcceptableGeometry(const QWindow *w, const QRectF &nativeRect);

protected:
    static QString formatWindowTitle(const QString &title, const QString &separator);
    QPlatformScreen *screenForGeometry(const QRect &newGeometry) const;
    static QSize constrainWindowSize(const QSize &size);

    QScopedPointer<QPlatformWindowPrivate> d_ptr;
private:
    //Q_DISABLE_COPY(QPlatformWindow)
};

#ifndef QBACKINGSTORE_H
#define QBACKINGSTORE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qrect.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>

QT_BEGIN_NAMESPACE


class QRect;
class QPoint;
class QImage;
class QBackingStorePrivate;
class QPlatformBackingStore;

// oye enable QPainter可以在QWindow上直接画图 RasterSurface
class  QBackingStore
{
public:
    explicit QBackingStore(QWindow *window);
    ~QBackingStore();

    QWindow *window() const;

    QPaintDevice *paintDevice();

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface.
    void flush(const QRegion &region, QWindow *window = Q_NULLPTR, const QPoint &offset = QPoint());

    void resize(const QSize &size);
    QSize size() const;

    bool scroll(const QRegion &area, int dx, int dy);

    void beginPaint(const QRegion &);
    void endPaint();

    void setStaticContents(const QRegion &region);
    QRegion staticContents() const;
    bool hasStaticContents() const;

    QPlatformBackingStore *handle() const;

private:
    QScopedPointer<QBackingStorePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QBACKINGSTORE_H

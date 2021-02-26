#ifndef QSHAPEDPIXMAPDNDWINDOW_H
#define QSHAPEDPIXMAPDNDWINDOW_H



#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/QRasterWindow>
#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE

class QShapedPixmapWindow : public QRasterWindow
{
    Q_OBJECT
public:
    explicit QShapedPixmapWindow(QScreen *screen = 0);
    ~QShapedPixmapWindow();

    void setUseCompositing(bool on) { m_useCompositing = on; }
    void setPixmap(const QPixmap &pixmap);
    void setHotspot(const QPoint &hotspot);

    void updateGeometry(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    QPixmap m_pixmap;
    QPoint m_hotSpot;
    bool m_useCompositing;
};

QT_END_NAMESPACE

#endif // QSHAPEDPIXMAPDNDWINDOW_H

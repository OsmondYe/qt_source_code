#ifndef QBITMAP_H
#define QBITMAP_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qpixmap.h>



class QVariant;

class  QBitmap : public QPixmap
{
public:
    QBitmap();
    QBitmap(const QPixmap &);
    QBitmap(int w, int h);
    explicit QBitmap(const QSize &);
    explicit QBitmap(const QString &fileName, const char *format = Q_NULLPTR);
    // ### Qt 6: don't inherit QPixmap
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QBitmap(const QBitmap &other) : QPixmap(other) {}
    // QBitmap(QBitmap &&other) : QPixmap(std::move(other)) {} // QPixmap doesn't, yet, have a move ctor
    QBitmap &operator=(const QBitmap &other) { QPixmap::operator=(other); return *this; }
    QBitmap &operator=(QBitmap &&other) Q_DECL_NOTHROW { QPixmap::operator=(std::move(other)); return *this; }
    ~QBitmap();
#endif

    QBitmap &operator=(const QPixmap &);
    inline void swap(QBitmap &other) { QPixmap::swap(other); } // prevent QBitmap<->QPixmap swaps
    operator QVariant() const;

    inline void clear() { fill(Qt::color0); }

    static QBitmap fromImage(const QImage &image, Qt::ImageConversionFlags flags = Qt::AutoColor);
    static QBitmap fromData(const QSize &size, const uchar *bits,
                            QImage::Format monoFormat = QImage::Format_MonoLSB);

    QBitmap transformed(const QMatrix &) const;
    QBitmap transformed(const QTransform &matrix) const;

    typedef QExplicitlySharedDataPointer<QPlatformPixmap> DataPtr;
};
//Q_DECLARE_SHARED(QBitmap)



#endif // QBITMAP_H

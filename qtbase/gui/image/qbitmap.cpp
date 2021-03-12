#include "qbitmap.h"
#include <qpa/qplatformpixmap.h>
#include <qpa/qplatformintegration.h>
#include "qimage.h"
#include "qscreen.h"
#include "qvariant.h"
#include <qpainter.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBitmap
    \inmodule QtGui
    \brief The QBitmap class provides monochrome (1-bit depth) pixmaps.

    \ingroup painting
    \ingroup shared

    The QBitmap class is a monochrome off-screen paint device used
    mainly for creating custom QCursor and QBrush objects,
    constructing QRegion objects, and for setting masks for pixmaps
    and widgets.

    QBitmap is a QPixmap subclass ensuring a depth of 1, except for
    null objects which have a depth of 0. If a pixmap with a depth
    greater than 1 is assigned to a bitmap, the bitmap will be
    dithered automatically.

    Use the QColor objects Qt::color0 and Qt::color1 when drawing on a
    QBitmap object (or a QPixmap object with depth 1).

    Painting with Qt::color0 sets the bitmap bits to 0, and painting
    with Qt::color1 sets the bits to 1. For a bitmap, 0-bits indicate
    background (or transparent pixels) and 1-bits indicate foreground
    (or opaque pixels). Use the clear() function to set all the bits
    to Qt::color0. Note that using the Qt::black and Qt::white colors
    make no sense because the QColor::pixel() value is not necessarily
    0 for black and 1 for white.

    The QBitmap class provides the transformed() function returning a
    transformed copy of the bitmap; use the QTransform argument to
    translate, scale, shear, and rotate the bitmap. In addition,
    QBitmap provides the static fromData() function which returns a
    bitmap constructed from the given \c uchar data, and the static
    fromImage() function returning a converted copy of a QImage
    object.

    Just like the QPixmap class, QBitmap is optimized by the use of
    implicit data sharing. For more information, see the \l {Implicit
    Data Sharing} documentation.

    \sa QPixmap, QImage, QImageReader, QImageWriter
*/

/*! \typedef QBitmap::DataPtr
  \internal
 */


QBitmap::QBitmap()    : QPixmap(QSize(0, 0), QPlatformPixmap::BitmapType)
{
}


QBitmap::QBitmap(int w, int h)    : QPixmap(QSize(w, h), QPlatformPixmap::BitmapType)
{
}

QBitmap::QBitmap(const QSize &size)    : QPixmap(size, QPlatformPixmap::BitmapType)
{
}

QBitmap::QBitmap(const QPixmap &pixmap)
{
    QBitmap::operator=(pixmap);
}

QBitmap::QBitmap(const QString& fileName, const char *format)
    : QPixmap(QSize(0, 0), QPlatformPixmap::BitmapType)
{
    load(fileName, format, Qt::MonoOnly);
}


QBitmap &QBitmap::operator=(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {                        // a null pixmap
        QBitmap(0, 0).swap(*this);
    } else if (pixmap.depth() == 1) {                // 1-bit pixmap
        QPixmap::operator=(pixmap);                // shallow assignment
    } else {                                        // n-bit depth pixmap
        QImage image;
        image = pixmap.toImage();                                // convert pixmap to image
        *this = fromImage(image);                                // will dither image
    }
    return *this;
}


QBitmap::operator QVariant() const
{
    return QVariant(QVariant::Bitmap, this);
}

QBitmap QBitmap::fromImage(const QImage &image, Qt::ImageConversionFlags flags)
{
    if (image.isNull())
        return QBitmap();

    QImage img = image.convertToFormat(QImage::Format_MonoLSB, flags);

    // make sure image.color(0) == Qt::color0 (white)
    // and image.color(1) == Qt::color1 (black)
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (img.color(0) == c0 && img.color(1) == c1) {
        img.invertPixels();
        img.setColor(0, c1);
        img.setColor(1, c0);
    }

    QScopedPointer<QPlatformPixmap> data(QGuiApplicationPrivate::platformIntegration()->createPlatformPixmap(QPlatformPixmap::BitmapType));

    data->fromImage(img, flags | Qt::MonoOnly);
    return QPixmap(data.take());
}

/*!
    Constructs a bitmap with the given \a size, and sets the contents to
    the \a bits supplied.

    The bitmap data has to be byte aligned and provided in in the bit
    order specified by \a monoFormat. The mono format must be either
    QImage::Format_Mono or QImage::Format_MonoLSB. Use
    QImage::Format_Mono to specify data on the XBM format.

    \sa fromImage()

*/
QBitmap QBitmap::fromData(const QSize &size, const uchar *bits, QImage::Format monoFormat)
{
    Q_ASSERT(monoFormat == QImage::Format_Mono || monoFormat == QImage::Format_MonoLSB);

    QImage image(size, monoFormat);
    image.setColor(0, QColor(Qt::color0).rgb());
    image.setColor(1, QColor(Qt::color1).rgb());

    // Need to memcpy each line separatly since QImage is 32bit aligned and
    // this data is only byte aligned...
    int bytesPerLine = (size.width() + 7) / 8;
    for (int y = 0; y < size.height(); ++y)
        memcpy(image.scanLine(y), bits + bytesPerLine * y, bytesPerLine);
    return QBitmap::fromImage(image);
}

/*!
    Returns a copy of this bitmap, transformed according to the given
    \a matrix.

    \sa QPixmap::transformed()
 */
QBitmap QBitmap::transformed(const QTransform &matrix) const
{
    QBitmap bm = QPixmap::transformed(matrix);
    return bm;
}

/*!
  \overload
  \obsolete

  This convenience function converts the \a matrix to a QTransform
  and calls the overloaded function.
*/
QBitmap QBitmap::transformed(const QMatrix &matrix) const
{
    return transformed(QTransform(matrix));
}

QT_END_NAMESPACE

#ifndef QPLATFORMPIXMAP_H
#define QPLATFORMPIXMAP_H


#include <QtGui/qtguiglobal.h>
#include <QtGui/qpixmap.h>
#include <QtCore/qatomic.h>

class QImageReader;

class  QPlatformPixmap
{
public:
    enum PixelType {
        // WARNING: Do not change the first two
        // Must match QPixmap::Type
        PixmapType, BitmapType
    };

    enum ClassId { RasterClass, DirectFBClass,
                   BlitterClass, Direct2DClass,
                   CustomClass = 1024 };

    QPlatformPixmap(PixelType pixelType, int classId);
    virtual ~QPlatformPixmap();

    virtual QPlatformPixmap *createCompatiblePlatformPixmap() const;

    virtual void resize(int width, int height) = 0;
    virtual void fromImage(const QImage &image,
                           Qt::ImageConversionFlags flags) = 0;
    virtual void fromImageInPlace(QImage &image,
                                  Qt::ImageConversionFlags flags)
    {
        fromImage(image, flags);
    }

    virtual void fromImageReader(QImageReader *imageReader,
                                 Qt::ImageConversionFlags flags);

    virtual bool fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags);
    virtual bool fromData(const uchar *buffer, uint len, const char *format,
                          Qt::ImageConversionFlags flags);

    virtual void copy(const QPlatformPixmap *data, const QRect &rect);
    virtual bool scroll(int dx, int dy, const QRect &rect);

    virtual int metric(QPaintDevice::PaintDeviceMetric metric) const = 0;
    virtual void fill(const QColor &color) = 0;

    virtual bool hasAlphaChannel() const = 0;
    virtual QPixmap transformed(const QTransform &matrix,
                                Qt::TransformationMode mode) const;

    virtual QImage toImage() const = 0;
    virtual QImage toImage(const QRect &rect) const;
    virtual QPaintEngine* paintEngine() const = 0;

    inline int serialNumber() const { return ser_no; }

    inline PixelType pixelType() const { return type; }
    inline ClassId classId() const { return static_cast<ClassId>(id); }

    virtual qreal devicePixelRatio() const = 0;
    virtual void setDevicePixelRatio(qreal scaleFactor) = 0;

    virtual QImage* buffer();

    inline int width() const { return w; }
    inline int height() const { return h; }
    inline int colorCount() const { return metric(QPaintDevice::PdmNumColors); }
    inline int depth() const { return d; }
    inline bool isNull() const { return is_null; }
    inline qint64 cacheKey() const {
        int classKey = id;
        if (classKey >= 1024)
            classKey = -(classKey >> 10);
        return ((((qint64) classKey) << 56)
                | (((qint64) ser_no) << 32)
                | ((qint64) detach_no));
    }

    static QPlatformPixmap *create(int w, int h, PixelType type);

protected:

    void setSerialNumber(int serNo);
    void setDetachNumber(int detNo);
    int w;
    int h;
    int d;
    bool is_null;

private:
    friend class QPixmap;
    friend class QImagePixmapCleanupHooks; // Needs to set is_cached
    friend class QOpenGLTextureCache; //Needs to check the reference count
    friend class QExplicitlySharedDataPointer<QPlatformPixmap>;

    QAtomicInt ref;
    int detach_no;

    PixelType type;
    int id;
    int ser_no;
    uint is_cached;
};

#  define QT_XFORM_TYPE_MSBFIRST 0
#  define QT_XFORM_TYPE_LSBFIRST 1
extern bool qt_xForm_helper(const QTransform&, int, int, int, uchar*, int, int, int, const uchar*, int, int, int);


#endif // QPLATFORMPIXMAP_H

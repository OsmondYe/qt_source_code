#ifndef QIMAGEWRITER_H
#define QIMAGEWRITER_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtGui/qimageiohandler.h>


class QIODevice;
class QImage;

class QImageWriterPrivate;
class  QImageWriter
{
    Q_DECLARE_TR_FUNCTIONS(QImageWriter)
public:
    enum ImageWriterError {
        UnknownError,
        DeviceError,
        UnsupportedFormatError
    };

    QImageWriter();
    explicit QImageWriter(QIODevice *device, const QByteArray &format);
    explicit QImageWriter(const QString &fileName, const QByteArray &format = QByteArray());
    ~QImageWriter();

    void setFormat(const QByteArray &format);
    QByteArray format() const;

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    void setQuality(int quality);
    int quality() const;

    void setCompression(int compression);
    int compression() const;

    void setGamma(float gamma);
    float gamma() const;

    void setSubType(const QByteArray &type);
    QByteArray subType() const;
    QList<QByteArray> supportedSubTypes() const;

    void setOptimizedWrite(bool optimize);
    bool optimizedWrite() const;

    void setProgressiveScanWrite(bool progressive);
    bool progressiveScanWrite() const;

    QImageIOHandler::Transformations transformation() const;
    void setTransformation(QImageIOHandler::Transformations orientation);

    // Obsolete as of 4.1
    void setDescription(const QString &description);
    QString description() const;

    void setText(const QString &key, const QString &text);

    bool canWrite() const;
    bool write(const QImage &image);

    ImageWriterError error() const;
    QString errorString() const;

    bool supportsOption(QImageIOHandler::ImageOption option) const;

    static QList<QByteArray> supportedImageFormats();
    static QList<QByteArray> supportedMimeTypes();

private:
    Q_DISABLE_COPY(QImageWriter)
    QImageWriterPrivate *d;
};


#endif // QIMAGEWRITER_H

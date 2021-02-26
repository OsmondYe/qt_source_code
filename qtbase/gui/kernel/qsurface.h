#ifndef QSURFACE_H
#define QSURFACE_H

QT_BEGIN_NAMESPACE


class QSurface
{
public:
    enum SurfaceClass {
        Window,
        Offscreen
    };

    enum SurfaceType {
        RasterSurface,
        OpenGLSurface,
        RasterGLSurface,
        OpenVGSurface,
    };

    virtual ~QSurface();

    SurfaceClass surfaceClass() const;

    virtual QSurfaceFormat format() const = 0;
    virtual QPlatformSurface *surfaceHandle() const = 0;

    virtual SurfaceType surfaceType() const = 0;
    bool supportsOpenGL() const;

    virtual QSize size() const = 0;

protected:
    explicit QSurface(SurfaceClass type);

    SurfaceClass m_type;

    QSurfacePrivate *m_reserved;
};

QT_END_NAMESPACE


#endif //QSURFACE_H

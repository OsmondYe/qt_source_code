class QSurface  // 可以有window和buffer2中
{
protected:
	SurfaceClass m_type;
	QSurfacePrivate *m_reserved;
public:
    virtual ~QSurface();
    SurfaceClass surfaceClass() const;
    virtual QSurfaceFormat format() const = 0;
	virtual QPlatformSurface *surfaceHandle() const = 0;
    virtual SurfaceType surfaceType() const = 0;
    bool supportsOpenGL() const;
    virtual QSize size() const = 0;
protected:
    explicit QSurface(SurfaceClass type);

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
};

class  QPlatformSurface
{
	QSurface *m_surface;
public:
    virtual ~QPlatformSurface(){}
    virtual QSurfaceFormat format() const = 0;

    QSurface *surface() const{return m_surface;}

private:
    explicit QPlatformSurface(QSurface *surface): m_surface(surface){}


    friend class QPlatformWindow;
    friend class QPlatformOffscreenSurface;
};



#ifndef QSURFACEFORMAT_H
#define QSURFACEFORMAT_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qpair.h>
#include <QtCore/qobjectdefs.h>

class QOpenGLContext;
class QSurfaceFormatPrivate;

class  QSurfaceFormat
{
    //Q_GADGET
public:
    enum FormatOption {
        StereoBuffers            = 0x0001,
        DebugContext             = 0x0002,
        DeprecatedFunctions      = 0x0004,
        ResetNotification        = 0x0008
    };
    //Q_ENUM(FormatOption)
    //Q_DECLARE_FLAGS(FormatOptions, FormatOption)

    enum SwapBehavior {
        DefaultSwapBehavior,
        SingleBuffer,
        DoubleBuffer,
        TripleBuffer
    };
    //Q_ENUM(SwapBehavior)

    enum RenderableType {
        DefaultRenderableType = 0x0,
        OpenGL                = 0x1,
        OpenGLES              = 0x2,
        OpenVG                = 0x4
    };
    //Q_ENUM(RenderableType)

    enum OpenGLContextProfile {
        NoProfile,
        CoreProfile,
        CompatibilityProfile
    };
    //Q_ENUM(OpenGLContextProfile)

    QSurfaceFormat();
    /*implicit*/ QSurfaceFormat(FormatOptions options);
    QSurfaceFormat(const QSurfaceFormat &other);
    QSurfaceFormat &operator=(const QSurfaceFormat &other);
    ~QSurfaceFormat();

    void setDepthBufferSize(int size);
    int depthBufferSize() const;

    void setStencilBufferSize(int size);
    int stencilBufferSize() const;

    void setRedBufferSize(int size);
    int redBufferSize() const;
    void setGreenBufferSize(int size);
    int greenBufferSize() const;
    void setBlueBufferSize(int size);
    int blueBufferSize() const;
    void setAlphaBufferSize(int size);
    int alphaBufferSize() const;

    void setSamples(int numSamples);
    int samples() const;

    void setSwapBehavior(SwapBehavior behavior);
    SwapBehavior swapBehavior() const;

    bool hasAlpha() const;

    void setProfile(OpenGLContextProfile profile);
    OpenGLContextProfile profile() const;

    void setRenderableType(RenderableType type);
    RenderableType renderableType() const;

    void setMajorVersion(int majorVersion);
    int majorVersion() const;

    void setMinorVersion(int minorVersion);
    int minorVersion() const;

    QPair<int, int> version() const;
    void setVersion(int major, int minor);


	inline bool stereo() const{   return testOption(QSurfaceFormat::StereoBuffers);}
    void setStereo(bool enable);

    void setOptions(QSurfaceFormat::FormatOptions options);
    void setOption(FormatOption option, bool on = true);
    bool testOption(FormatOption option) const;
    QSurfaceFormat::FormatOptions options() const;

    int swapInterval() const;
    void setSwapInterval(int interval);

    static void setDefaultFormat(const QSurfaceFormat &format);
    static QSurfaceFormat defaultFormat();

private:
    QSurfaceFormatPrivate *d;

    void detach();
};


//Q_DECLARE_OPERATORS_FOR_FLAGS(QSurfaceFormat::FormatOptions)



#endif //QSURFACEFORMAT_H

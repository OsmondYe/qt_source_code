#include "qsurface.h"
#include "qopenglcontext.h"



bool QSurface::supportsOpenGL() const
{
    SurfaceType type = surfaceType();
    return type == OpenGLSurface || type == RasterGLSurface;
}

QSurface::QSurface(SurfaceClass type)
    : m_type(type), m_reserved(0)
{
}


QSurface::~QSurface()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (context && context->surface() == this)
        context->doneCurrent();
}

QSurface::SurfaceClass QSurface::surfaceClass() const
{
    return m_type;
}



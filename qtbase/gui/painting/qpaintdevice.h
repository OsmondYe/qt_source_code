#ifndef QPAINTDEVICE_H
#define QPAINTDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE



class QPaintEngine;

class QPaintDevicePrivate;

/*
	有纯虚函数,virtual QPaintEngine *paintEngine() const = 0;
		派生类自己必须实现一个引擎出来, 比如QWidget
*/
class QPaintDevice                                // device for QPainter
{
	QPaintDevicePrivate *reserved;
    ushort        painters;                        // refcount

public:
    enum PaintDeviceMetric {
        PdmWidth = 1,
        PdmHeight,
        PdmWidthMM,
        PdmHeightMM,
        PdmNumColors,
        PdmDepth,
        PdmDpiX,
        PdmDpiY,
        PdmPhysicalDpiX,
        PdmPhysicalDpiY,
        PdmDevicePixelRatio,
        PdmDevicePixelRatioScaled
    };

    virtual ~QPaintDevice(){
		if (paintingActive())
	        qWarning("QPaintDevice: Cannot destroy paint device that is being "
	                  "painted");
    }

	virtual int devType() const { return QInternal::UnknownDevice; }	
    bool paintingActive() const {return painters != 0;}
    virtual QPaintEngine *paintEngine() const = 0;


    int width() const { return metric(PdmWidth); }
    int height() const { return metric(PdmHeight); }
    int widthMM() const { return metric(PdmWidthMM); }
    int heightMM() const { return metric(PdmHeightMM); }
    int logicalDpiX() const { return metric(PdmDpiX); }
    int logicalDpiY() const { return metric(PdmDpiY); }
    int physicalDpiX() const { return metric(PdmPhysicalDpiX); }
    int physicalDpiY() const { return metric(PdmPhysicalDpiY); }
    int devicePixelRatio() const { return metric(PdmDevicePixelRatio); }
    qreal devicePixelRatioF()  const { return metric(PdmDevicePixelRatioScaled) / devicePixelRatioFScale(); }
    int colorCount() const { return metric(PdmNumColors); }
    int depth() const { return metric(PdmDepth); }

    static inline qreal devicePixelRatioFScale() { return 0x10000; }
protected:
    QPaintDevice() {   reserved = 0;   painters = 0;}
    virtual int metric(PaintDeviceMetric metric) const; // 不同Device对度量衡的理解不同	
    virtual void initPainter(QPainter *painter) const {}
    virtual QPaintDevice *redirected(QPoint *offset) const { return 0;}
    virtual QPainter *sharedPainter() const{return 0;}


private:
    //Q_DISABLE_COPY(QPaintDevice)
  
    friend class QPainter;
    friend class QPainterPrivate;
    friend class QFontEngineMac;
    friend class QX11PaintEngine;
    friend  int qt_paint_device_metric(const QPaintDevice *device, PaintDeviceMetric metric);
};

int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric)
{
    return device->metric(metric);
}


inline int QPaintDevice::metric(PaintDeviceMetric m) const
{
    // Fallback: A subclass has not implemented PdmDevicePixelRatioScaled but might
    // have implemented PdmDevicePixelRatio.
    if (m == PdmDevicePixelRatioScaled)
        return this->metric(PdmDevicePixelRatio) * devicePixelRatioFScale();

    qWarning("QPaintDevice::metrics: Device has no metric information");

    if (m == PdmDpiX) {
        return 72;
    } else if (m == PdmDpiY) {
        return 72;
    } else if (m == PdmNumColors) {
        // FIXME: does this need to be a real value?
        return 256;
    } else if (m == PdmDevicePixelRatio) {
        return 1;
    } else {
        qDebug("Unrecognised metric %d!",m);
        return 0;
    }
}



QT_END_NAMESPACE

#endif // QPAINTDEVICE_H

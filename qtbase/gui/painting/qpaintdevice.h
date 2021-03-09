#ifndef QPAINTDEVICE_H
#define QPAINTDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE



class QPaintEngine;

class QPaintDevicePrivate;

/*
	�д��麯��,virtual QPaintEngine *paintEngine() const = 0;
		�������Լ�����ʵ��һ���������
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

    virtual ~QPaintDevice();

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
    QPaintDevice() Q_DECL_NOEXCEPT;
    virtual int metric(PaintDeviceMetric metric) const;
    virtual void initPainter(QPainter *painter) const;
    virtual QPaintDevice *redirected(QPoint *offset) const;
    virtual QPainter *sharedPainter() const;


private:
    //Q_DISABLE_COPY(QPaintDevice)
  
    friend class QPainter;
    friend class QPainterPrivate;
    friend class QFontEngineMac;
    friend class QX11PaintEngine;
    friend  int qt_paint_device_metric(const QPaintDevice *device, PaintDeviceMetric metric);
};


QT_END_NAMESPACE

#endif // QPAINTDEVICE_H

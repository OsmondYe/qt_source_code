#ifndef QFRAME_H
#define QFRAME_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>


class QFrame : public QWidget
{
//    Q_OBJECT
//
//    Q_PROPERTY(Shape frameShape READ frameShape WRITE setFrameShape)
//    Q_PROPERTY(Shadow frameShadow READ frameShadow WRITE setFrameShadow)
//    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
//    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
//    Q_PROPERTY(int frameWidth READ frameWidth)
//    Q_PROPERTY(QRect frameRect READ frameRect WRITE setFrameRect DESIGNABLE false)

public:
    explicit QFrame(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~QFrame();

    int frameStyle() const;
    void setFrameStyle(int);

    int frameWidth() const;

    QSize sizeHint() const ;

    enum Shape {
        NoFrame  = 0, // no frame
        Box = 0x0001, // rectangular box
        Panel = 0x0002, // rectangular panel
        WinPanel = 0x0003, // rectangular panel (Windows)
        HLine = 0x0004, // horizontal line
        VLine = 0x0005, // vertical line
        StyledPanel = 0x0006 // rectangular panel depending on the GUI style
    };
		
    //Q_ENUM(Shape)

	enum Shadow {
        Plain = 0x0010, // plain line
        Raised = 0x0020, // raised shadow effect
        Sunken = 0x0030 // sunken shadow effect
    };
		
    //Q_ENUM(Shadow)

    enum StyleMask {
        Shadow_Mask = 0x00f0, // mask for the shadow
        Shape_Mask = 0x000f // mask for the shape
    };

    Shape frameShape() const;
    void setFrameShape(Shape);
    Shadow frameShadow() const;
    void setFrameShadow(Shadow);

    int lineWidth() const;
    void setLineWidth(int);

    int midLineWidth() const;
    void setMidLineWidth(int);

    QRect frameRect() const;
    void setFrameRect(const QRect &);

protected:
	
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void changeEvent(QEvent *) override;
    void drawFrame(QPainter *);


protected:
    QFrame(QFramePrivate &dd, QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    void initStyleOption(QStyleOptionFrame *option) const;

private:
    //Q_DISABLE_COPY(QFrame)
    //Q_DECLARE_PRIVATE(QFrame)
};



#endif // QFRAME_H

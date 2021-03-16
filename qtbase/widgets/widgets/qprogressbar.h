#ifndef QPROGRESSBAR_H
#define QPROGRESSBAR_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qframe.h>

class QProgressBarPrivate;
class QStyleOptionProgressBar;

class QProgressBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool textVisible READ isTextVisible WRITE setTextVisible)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE setInvertedAppearance)
    Q_PROPERTY(Direction textDirection READ textDirection WRITE setTextDirection)
    Q_PROPERTY(QString format READ format WRITE setFormat RESET resetFormat)

public:
    enum Direction { TopToBottom, BottomToTop };
    Q_ENUM(Direction)

    explicit QProgressBar(QWidget *parent = Q_NULLPTR);
    ~QProgressBar();
public Q_SLOTS:
	void reset();
	void setRange(int minimum, int maximum);
	void setMinimum(int minimum);
	void setMaximum(int maximum);
	void setValue(int value);
	void setOrientation(Qt::Orientation);

Q_SIGNALS:
	void valueChanged(int value);
public:

    int minimum() const;
    int maximum() const;

    int value() const;

    virtual QString text() const;
    void setTextVisible(bool visible);
    bool isTextVisible() const;

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    Qt::Orientation orientation() const;

    void setInvertedAppearance(bool invert);
    bool invertedAppearance() const;
    void setTextDirection(QProgressBar::Direction textDirection);
    QProgressBar::Direction textDirection() const;

    void setFormat(const QString &format);
    void resetFormat();
    QString format() const;



protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void initStyleOption(QStyleOptionProgressBar *option) const;

private:
    //Q_DECLARE_PRIVATE(QProgressBar)
    //Q_DISABLE_COPY(QProgressBar)
};

#endif // QPROGRESSBAR_H

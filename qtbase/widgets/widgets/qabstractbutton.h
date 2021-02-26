#ifndef QABSTRACTBUTTON_H
#define QABSTRACTBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qicon.h>
#include <QtGui/qkeysequence.h>
#include <QtWidgets/qwidget.h>



class QButtonGroup;
class QAbstractButtonPrivate;

class  QAbstractButton : public QWidget
{

public:
    explicit QAbstractButton(QWidget *parent = Q_NULLPTR);
    ~QAbstractButton();

    void setText(const QString &text);
    QString text() const;

    void setIcon(const QIcon &icon);
    QIcon icon() const;

    QSize iconSize() const;

    void setShortcut(const QKeySequence &key);
    QKeySequence shortcut() const;

    void setCheckable(bool);
    bool isCheckable() const;

    bool isChecked() const;

    void setDown(bool);
    bool isDown() const;

    void setAutoRepeat(bool);
    bool autoRepeat() const;

    void setAutoRepeatDelay(int);
    int autoRepeatDelay() const;

    void setAutoRepeatInterval(int);
    int autoRepeatInterval() const;

    void setAutoExclusive(bool);
    bool autoExclusive() const;

    QButtonGroup *group() const;

public Q_SLOTS:
    void setIconSize(const QSize &size);
    void animateClick(int msec = 100);
    void click();
    void toggle();
    void setChecked(bool);

Q_SIGNALS:
    void pressed();
    void released();
    void clicked(bool checked = false);
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE = 0;
    virtual bool hitButton(const QPoint &pos) const;
    virtual void checkStateSet();
    virtual void nextCheckState();

    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *e) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *e) Q_DECL_OVERRIDE;
    void changeEvent(QEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;


protected:
    QAbstractButton(QAbstractButtonPrivate &dd, QWidget* parent = Q_NULLPTR);

private:
    friend class QButtonGroup;
};


#endif // QABSTRACTBUTTON_H

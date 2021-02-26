#ifndef QLAYOUT_H
#define QLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>
#include <QtWidgets/qlayoutitem.h>
#include <QtWidgets/qsizepolicy.h>
#include <QtCore/qrect.h>
#include <QtCore/qmargins.h>

#include <limits.h>



class  QLayout : public QObject, public QLayoutItem
{

public:
    enum SizeConstraint {
        SetDefaultConstraint,
        SetNoConstraint,
        SetMinimumSize,
        SetFixedSize,
        SetMaximumSize,
        SetMinAndMaxSize
    };

    QLayout(QWidget *parent);
    QLayout();
    ~QLayout();

    int margin() const;
    int spacing() const;

    void setMargin(int);
    void setSpacing(int);

    void setContentsMargins(int left, int top, int right, int bottom);
    void setContentsMargins(const QMargins &margins);
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;
    QMargins contentsMargins() const;
    QRect contentsRect() const;

    bool setAlignment(QWidget *w, Qt::Alignment alignment);
    bool setAlignment(QLayout *l, Qt::Alignment alignment);
    using QLayoutItem::setAlignment;

    void setSizeConstraint(SizeConstraint);
    SizeConstraint sizeConstraint() const;
    void setMenuBar(QWidget *w);
    QWidget *menuBar() const;

    QWidget *parentWidget() const;

    void invalidate() ;
    QRect geometry() const ;
    bool activate();
    void update();

    void addWidget(QWidget *w);

    void removeWidget(QWidget *w);
    void removeItem(QLayoutItem *);

    Qt::Orientations expandingDirections() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;
    virtual void setGeometry(const QRect&) ;
	
    virtual void addItem(QLayoutItem *) = 0;
    virtual QLayoutItem *itemAt(int index) const = 0;
    virtual QLayoutItem *takeAt(int index) = 0;
    virtual int count() const = 0;
	
    virtual int indexOf(QWidget *) const;
    bool isEmpty() const ;
    QSizePolicy::ControlTypes controlTypes() const ;

    // ### Qt 6 make this function virtual
    QLayoutItem *replaceWidget(QWidget *from, QWidget *to, Qt::FindChildOptions options = Qt::FindChildrenRecursively);

    int totalHeightForWidth(int w) const;
    QSize totalMinimumSize() const;
    QSize totalMaximumSize() const;
    QSize totalSizeHint() const;
    QLayout *layout() ;

    void setEnabled(bool);
    bool isEnabled() const;


    static QSize closestAcceptableSize(const QWidget *w, const QSize &s);

protected:
    void widgetEvent(QEvent *);
    void childEvent(QChildEvent *e) ;
    void addChildLayout(QLayout *l);
    void addChildWidget(QWidget *w);
    bool adoptLayout(QLayout *layout);

    QRect alignmentRect(const QRect&) const;
protected:
    QLayout(QLayoutPrivate &d, QLayout*, QWidget*);

private:
    

    static void activateRecursiveHelper(QLayoutItem *item);

    friend class QApplicationPrivate;
    friend class QWidget;

};


#endif // QLAYOUT_H

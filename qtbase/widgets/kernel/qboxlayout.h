#ifndef QBOXLAYOUT_H
#define QBOXLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qwidget.h>
#include <limits.h>



class QBoxLayoutPrivate;

class  QBoxLayout : public QLayout
{

public:
    enum Direction { 
		RightToLeft, 
		LeftToRight, 
		TopToBottom, 
		BottomToTop,
        Down = TopToBottom, 
        Up = BottomToTop 
       };

    explicit QBoxLayout(Direction, QWidget *parent = Q_NULLPTR);

    ~QBoxLayout();

    Direction direction() const;
    void setDirection(Direction);

    void addSpacing(int size);
    void addStretch(int stretch = 0);
    void addSpacerItem(QSpacerItem *spacerItem);
    void addWidget(QWidget *, int stretch = 0, Qt::Alignment alignment = Qt::Alignment());
    void addLayout(QLayout *layout, int stretch = 0);
    void addStrut(int);
    void addItem(QLayoutItem *) ;

    void insertSpacing(int index, int size);
    void insertStretch(int index, int stretch = 0);
    void insertSpacerItem(int index, QSpacerItem *spacerItem);
    void insertWidget(int index, QWidget *widget, int stretch = 0, Qt::Alignment alignment = Qt::Alignment());
    void insertLayout(int index, QLayout *layout, int stretch = 0);
    void insertItem(int index, QLayoutItem *);

    int spacing() const;
    void setSpacing(int spacing);

    bool setStretchFactor(QWidget *w, int stretch);
    bool setStretchFactor(QLayout *l, int stretch);
    void setStretch(int index, int stretch);
    int stretch(int index) const;

    QSize sizeHint() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;

    bool hasHeightForWidth() const ;
    int heightForWidth(int) const ;
    int minimumHeightForWidth(int) const ;

    Qt::Orientations expandingDirections() const ;
    void invalidate() ;
    QLayoutItem *itemAt(int) const ;
    QLayoutItem *takeAt(int) ;
    int count() const ;
    void setGeometry(const QRect&) ;

};

class  QHBoxLayout : public QBoxLayout
{
    
public:
    QHBoxLayout();
    explicit QHBoxLayout(QWidget *parent);
    ~QHBoxLayout();

};

class  QVBoxLayout : public QBoxLayout
{
    
public:
    QVBoxLayout();
    explicit QVBoxLayout(QWidget *parent);
    ~QVBoxLayout();

};


#endif // QBOXLAYOUT_H

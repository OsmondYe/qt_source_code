#ifndef QGRIDLAYOUT_H
#define QGRIDLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qlayout.h>
#ifdef QT_INCLUDE_COMPAT
#include <QtWidgets/qwidget.h>
#endif

#include <limits.h>


class QGridLayoutPrivate;

/*
	oye
	以0,0位base
	add item 必须给出 raw 和 col
*/
class  QGridLayout : public QLayout
{
//    Q_OBJECT
//    Q_DECLARE_PRIVATE(QGridLayout)
//    QDOC_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing)
//    QDOC_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)
public:
	void addItem(QLayoutItem *item, int row, int column, 
				int rowSpan = 1, int columnSpan = 1, 
				Qt::Alignment = Qt::Alignment());
	
    QLayoutItem *itemAtPosition(int row, int column) const;  
	int columnCount() const;
	int rowCount() const;
	inline void addWidget(QWidget *w) { QLayout::addWidget(w); }
	void addWidget(QWidget *, int row, int column, Qt::Alignment = Qt::Alignment());
	void addWidget(QWidget *, int row, int column, int rowSpan, int columnSpan, Qt::Alignment = Qt::Alignment());
	void addLayout(QLayout *, int row, int column, Qt::Alignment = Qt::Alignment());
	void addLayout(QLayout *, int row, int column, int rowSpan, int columnSpan, Qt::Alignment = Qt::Alignment());
	
public:

    void setHorizontalSpacing(int spacing);
    int horizontalSpacing() const;
	
    void setVerticalSpacing(int spacing);
    int verticalSpacing() const;
	
    void setSpacing(int spacing);
    int spacing() const;

    void setRowStretch(int row, int stretch);
    void setColumnStretch(int column, int stretch);
	
    int rowStretch(int row) const;
    int columnStretch(int column) const;

    void setRowMinimumHeight(int row, int minSize);
    void setColumnMinimumWidth(int column, int minSize);
    int rowMinimumHeight(int row) const;
    int columnMinimumWidth(int column) const;


    QRect cellRect(int row, int column) const;

    void invalidate() override;

    void setOriginCorner(Qt::Corner);
    Qt::Corner originCorner() const;
	
  
    void setDefaultPositioning(int n, Qt::Orientation orient);
    void getItemPosition(int idx, int *row, int *column, int *rowSpan, int *columnSpan) const;

public:
	QLayoutItem *takeAt(int index) override;
	QLayoutItem *itemAt(int index) const override;
	int count() const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int minimumHeightForWidth(int) const override;
    Qt::Orientations expandingDirections() const override;
	void setGeometry(const QRect&) override;
public:
    explicit QGridLayout(QWidget *parent);
    QGridLayout();
    ~QGridLayout();
protected:
    void addItem(QLayoutItem *) override;
};
#endif // QGRIDLAYOUT_H

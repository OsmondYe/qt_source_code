#ifndef QABSTRACTSCROLLAREA_H
#define QABSTRACTSCROLLAREA_H

class QAbstractScrollAreaPrivate;

/*
	OYE
	qt, 默认定义, 抽象可滚动区域, 必然是有边框的
	- view_port:   central_widget
*/
class  QAbstractScrollArea : public QFrame
{
	// Q_OBJECT
public:
	QWidget *viewport() const;
    void setViewport(QWidget *widget);


    Qt::ScrollBarPolicy verticalScrollBarPolicy() const;
    void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy);
    QScrollBar *verticalScrollBar() const;
    void setVerticalScrollBar(QScrollBar *scrollbar);

    Qt::ScrollBarPolicy horizontalScrollBarPolicy() const;
    void setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy);
    QScrollBar *horizontalScrollBar() const;
    void setHorizontalScrollBar(QScrollBar *scrollbar);

    QWidget *cornerWidget() const;
    void setCornerWidget(QWidget *widget);

    void addScrollBarWidget(QWidget *widget, Qt::Alignment alignment);
    QWidgetList scrollBarWidgets(Qt::Alignment alignment);


    QSize maximumViewportSize() const;

    QSize minimumSizeHint() const override;

    QSize sizeHint() const override;

    virtual void setupViewport(QWidget *viewport);

    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);

protected:
    QAbstractScrollArea(QAbstractScrollAreaPrivate &dd, QWidget *parent = Q_NULLPTR);
    void setViewportMargins(int left, int top, int right, int bottom);
    void setViewportMargins(const QMargins &margins);
    QMargins viewportMargins() const;

    bool eventFilter(QObject *, QEvent *) override;
    bool event(QEvent *) override;
    virtual bool viewportEvent(QEvent *);

    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

    void keyPressEvent(QKeyEvent *) override;

    virtual void scrollContentsBy(int dx, int dy);

    virtual QSize viewportSizeHint() const;
public:
	 explicit QAbstractScrollArea(QWidget *parent = Q_NULLPTR);
    ~QAbstractScrollArea();
		enum SizeAdjustPolicy {
			AdjustIgnored,
			AdjustToContentsOnFirstShow,
			AdjustToContents
		};
private:
    Q_PRIVATE_SLOT(d_func(), void _q_hslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_vslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_showOrHideScrollBars())

    friend class QStyleSheetStyle;
    friend class QWidgetPrivate;
};


#endif // QABSTRACTSCROLLAREA_H

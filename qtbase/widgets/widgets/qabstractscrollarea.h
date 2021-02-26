#ifndef QABSTRACTSCROLLAREA_H
#define QABSTRACTSCROLLAREA_H

QT_BEGIN_NAMESPACE

class QAbstractScrollAreaPrivate;

class Q_WIDGETS_EXPORT QAbstractScrollArea : public QFrame
{

public:
    explicit QAbstractScrollArea(QWidget *parent = Q_NULLPTR);
    ~QAbstractScrollArea();

    enum SizeAdjustPolicy {
        AdjustIgnored,
        AdjustToContentsOnFirstShow,
        AdjustToContents
    };

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

    QWidget *viewport() const;
    void setViewport(QWidget *widget);
    QSize maximumViewportSize() const;

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE;

    virtual void setupViewport(QWidget *viewport);

    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);

protected:
    QAbstractScrollArea(QAbstractScrollAreaPrivate &dd, QWidget *parent = Q_NULLPTR);
    void setViewportMargins(int left, int top, int right, int bottom);
    void setViewportMargins(const QMargins &margins);
    QMargins viewportMargins() const;

    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
    bool event(QEvent *) Q_DECL_OVERRIDE;
    virtual bool viewportEvent(QEvent *);

    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;

    virtual void scrollContentsBy(int dx, int dy);

    virtual QSize viewportSizeHint() const;

private:
    Q_PRIVATE_SLOT(d_func(), void _q_hslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_vslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_showOrHideScrollBars())

    friend class QStyleSheetStyle;
    friend class QWidgetPrivate;
};


QT_END_NAMESPACE

#endif // QABSTRACTSCROLLAREA_H

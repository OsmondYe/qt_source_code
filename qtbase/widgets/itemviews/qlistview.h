#ifndef QLISTVIEW_H
#define QLISTVIEW_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractitemview.h>

QT_REQUIRE_CONFIG(listview);

QT_BEGIN_NAMESPACE

class QListViewPrivate;

class Q_WIDGETS_EXPORT QListView : public QAbstractItemView
{
    Q_OBJECT
    Q_PROPERTY(Movement movement READ movement WRITE setMovement)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow)
    Q_PROPERTY(bool isWrapping READ isWrapping WRITE setWrapping)
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
    Q_PROPERTY(LayoutMode layoutMode READ layoutMode WRITE setLayoutMode)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)
    Q_PROPERTY(QSize gridSize READ gridSize WRITE setGridSize)
    Q_PROPERTY(ViewMode viewMode READ viewMode WRITE setViewMode)
    Q_PROPERTY(int modelColumn READ modelColumn WRITE setModelColumn)
    Q_PROPERTY(bool uniformItemSizes READ uniformItemSizes WRITE setUniformItemSizes)
    Q_PROPERTY(int batchSize READ batchSize WRITE setBatchSize)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
    Q_PROPERTY(bool selectionRectVisible READ isSelectionRectVisible WRITE setSelectionRectVisible)

public:
    enum Movement { Static, Free, Snap };
    Q_ENUM(Movement)
    enum Flow { LeftToRight, TopToBottom };
    Q_ENUM(Flow)
    enum ResizeMode { Fixed, Adjust };
    Q_ENUM(ResizeMode)
    enum LayoutMode { SinglePass, Batched };
    Q_ENUM(LayoutMode)
    enum ViewMode { ListMode, IconMode };
    Q_ENUM(ViewMode)

    explicit QListView(QWidget *parent = Q_NULLPTR);
    ~QListView();

    void setMovement(Movement movement);
    Movement movement() const;

    void setFlow(Flow flow);
    Flow flow() const;

    void setWrapping(bool enable);
    bool isWrapping() const;

    void setResizeMode(ResizeMode mode);
    ResizeMode resizeMode() const;

    void setLayoutMode(LayoutMode mode);
    LayoutMode layoutMode() const;

    void setSpacing(int space);
    int spacing() const;

    void setBatchSize(int batchSize);
    int batchSize() const;

    void setGridSize(const QSize &size);
    QSize gridSize() const;

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void clearPropertyFlags();

    bool isRowHidden(int row) const;
    void setRowHidden(int row, bool hide);

    void setModelColumn(int column);
    int modelColumn() const;

    void setUniformItemSizes(bool enable);
    bool uniformItemSizes() const;

    void setWordWrap(bool on);
    bool wordWrap() const;

    void setSelectionRectVisible(bool show);
    bool isSelectionRectVisible() const;

    QRect visualRect(const QModelIndex &index) const Q_DECL_OVERRIDE;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) Q_DECL_OVERRIDE;
    QModelIndex indexAt(const QPoint &p) const Q_DECL_OVERRIDE;

    void doItemsLayout() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void setRootIndex(const QModelIndex &index) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void indexesMoved(const QModelIndexList &indexes);

protected:
    QListView(QListViewPrivate &, QWidget *parent = Q_NULLPTR);

    bool event(QEvent *e) Q_DECL_OVERRIDE;

    void scrollContentsBy(int dx, int dy) Q_DECL_OVERRIDE;

    void resizeContents(int width, int height);
    QSize contentsSize() const;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) Q_DECL_OVERRIDE;
    void rowsInserted(const QModelIndex &parent, int start, int end) Q_DECL_OVERRIDE;
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *e) Q_DECL_OVERRIDE;
#endif

    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
#ifndef QT_NO_DRAGANDDROP
    void dragMoveEvent(QDragMoveEvent *e) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *e) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *e) Q_DECL_OVERRIDE;
    void startDrag(Qt::DropActions supportedActions) Q_DECL_OVERRIDE;
#endif // QT_NO_DRAGANDDROP

    QStyleOptionViewItem viewOptions() const Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;

    int horizontalOffset() const Q_DECL_OVERRIDE;
    int verticalOffset() const Q_DECL_OVERRIDE;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;
    QRect rectForIndex(const QModelIndex &index) const;
    void setPositionForIndex(const QPoint &position, const QModelIndex &index);

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE;
    QRegion visualRegionForSelection(const QItemSelection &selection) const Q_DECL_OVERRIDE;
    QModelIndexList selectedIndexes() const Q_DECL_OVERRIDE;

    void updateGeometries() Q_DECL_OVERRIDE;

    bool isIndexHidden(const QModelIndex &index) const Q_DECL_OVERRIDE;

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) Q_DECL_OVERRIDE;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) Q_DECL_OVERRIDE;

    QSize viewportSizeHint() const Q_DECL_OVERRIDE;

private:
    int visualIndex(const QModelIndex &index) const;

    Q_DECLARE_PRIVATE(QListView)
    Q_DISABLE_COPY(QListView)
};

QT_END_NAMESPACE

#endif // QLISTVIEW_H

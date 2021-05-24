#ifndef QSCROLLBAR_H
#define QSCROLLBAR_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

#include <QtWidgets/qabstractslider.h>

//QT_REQUIRE_CONFIG(scrollbar);

class QScrollBarPrivate;
class QStyleOptionSlider;

class  QScrollBar : public QAbstractSlider
{
    Q_OBJECT
public:
    explicit QScrollBar(QWidget *parent = Q_NULLPTR);
    explicit QScrollBar(Qt::Orientation, QWidget *parent = Q_NULLPTR);
    ~QScrollBar();

    QSize sizeHint() const Q_DECL_OVERRIDE;
    bool event(QEvent *event) Q_DECL_OVERRIDE;

protected:
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
#endif
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent*) Q_DECL_OVERRIDE;
    void sliderChange(SliderChange change) Q_DECL_OVERRIDE;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *) Q_DECL_OVERRIDE;
#endif
    void initStyleOption(QStyleOptionSlider *option) const;


private:
    friend class QAbstractScrollAreaPrivate;
    friend Q_WIDGETS_EXPORT QStyleOptionSlider qt_qscrollbarStyleOption(QScrollBar *scrollBar);

    Q_DISABLE_COPY(QScrollBar)
    Q_DECLARE_PRIVATE(QScrollBar)
#if QT_CONFIG(itemviews)
    friend class QTableView;
    friend class QTreeViewPrivate;
    friend class QCommonListViewBase;
    friend class QListModeViewBase;
    friend class QAbstractItemView;
#endif
};

#endif // QSCROLLBAR_H

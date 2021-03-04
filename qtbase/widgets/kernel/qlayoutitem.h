#ifndef QLAYOUTITEM_H
#define QLAYOUTITEM_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qsizepolicy.h>
#include <QtCore/qrect.h>

#include <limits.h>


static const Q_DECL_UNUSED int QLAYOUTSIZE_MAX = INT_MAX/256/16;

class QLayout;
class QLayoutItem;
class QSpacerItem;
class QWidget;
class QSize;

// oye derived calss : QSpacerItem, QWidgetItem, QLayout
class  QLayoutItem
{
public:
    inline explicit QLayoutItem(Qt::Alignment alignment = Qt::Alignment());
    virtual ~QLayoutItem();
    virtual QSize sizeHint() const = 0;
    virtual QSize minimumSize() const = 0;
    virtual QSize maximumSize() const = 0;
    virtual Qt::Orientations expandingDirections() const = 0;
    virtual void setGeometry(const QRect&) = 0;
    virtual QRect geometry() const = 0;
    virtual bool isEmpty() const = 0;
    virtual bool hasHeightForWidth() const;
    virtual int heightForWidth(int) const;
    virtual int minimumHeightForWidth(int) const;
    virtual void invalidate();

    virtual QWidget *widget();
    virtual QLayout *layout();				// 基类定义类型探测
    virtual QSpacerItem *spacerItem();

    Qt::Alignment alignment() const { return align; }
    void setAlignment(Qt::Alignment a);
    virtual QSizePolicy::ControlTypes controlTypes() const;

protected:
    Qt::Alignment align;
};

inline QLayoutItem::QLayoutItem(Qt::Alignment aalignment)
    : align(aalignment) { }

class  QSpacerItem : public QLayoutItem
{
public:
    QSpacerItem(int w, int h,
                 QSizePolicy::Policy hData = QSizePolicy::Minimum,
                 QSizePolicy::Policy vData = QSizePolicy::Minimum)
        : width(w), height(h), sizeP(hData, vData) { }
    ~QSpacerItem();

    void changeSize(int w, int h,
                     QSizePolicy::Policy hData = QSizePolicy::Minimum,
                     QSizePolicy::Policy vData = QSizePolicy::Minimum);
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    Qt::Orientations expandingDirections() const override;
    bool isEmpty() const override;
    void setGeometry(const QRect&) override;
    QRect geometry() const override;
    QSpacerItem *spacerItem() override;
    QSizePolicy sizePolicy() const { return sizeP; }

private:
    int width;
    int height;
    QSizePolicy sizeP;
    QRect rect;
};

class  QWidgetItem : public QLayoutItem
{

public:
    explicit QWidgetItem(QWidget *w) : wid(w) { }
    ~QWidgetItem();

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    Qt::Orientations expandingDirections() const override;
    bool isEmpty() const override;
    void setGeometry(const QRect&) override;
    QRect geometry() const override;
    QWidget *widget() override;

    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    QSizePolicy::ControlTypes controlTypes() const override;
protected:
    QWidget *wid;
};

class  QWidgetItemV2 : public QWidgetItem
{
public:
    explicit QWidgetItemV2(QWidget *widget);
    ~QWidgetItemV2();

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    int heightForWidth(int width) const override;

private:
    enum { Dirty = -123, HfwCacheMaxSize = 3 };

    inline bool useSizeCache() const;
    void updateCacheIfNecessary() const;
    inline void invalidateSizeCache() {
        q_cachedMinimumSize.setWidth(Dirty);
        q_hfwCacheSize = 0;
    }

    mutable QSize q_cachedMinimumSize;
    mutable QSize q_cachedSizeHint;
    mutable QSize q_cachedMaximumSize;
    mutable QSize q_cachedHfws[HfwCacheMaxSize];
    mutable short q_firstCachedHfw;
    mutable short q_hfwCacheSize;
    void *d;

    friend class QWidgetPrivate;

    Q_DISABLE_COPY(QWidgetItemV2)
};

#endif // QLAYOUTITEM_H

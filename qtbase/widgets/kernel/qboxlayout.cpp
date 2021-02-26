#include "qboxlayout.h"
#include "qapplication.h"
#include "qwidget.h"
#include "qlist.h"
#include "qsizepolicy.h"
#include "qvector.h"

#include "qlayoutengine_p.h"
#include "qlayout_p.h"

// oye, a desigon pattern,  using shell to wrapper all base layout_items
struct QBoxLayoutItem
{
    QBoxLayoutItem(QLayoutItem *it, int stretch_ = 0)
        : item(it), stretch(stretch_), magic(false) { }
    ~QBoxLayoutItem() { delete item; }

    int hfw(int w) { // maybe widget
        if (item->hasHeightForWidth()) {
            return item->heightForWidth(w);
        } else {
            return item->sizeHint().height();
        }
    }
    int mhfw(int w) {
        if (item->hasHeightForWidth()) {
            return item->heightForWidth(w);
        } else {
            return item->minimumSize().height();
        }
    }
    int hStretch() {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().horizontalStretch();
        } else {
            return stretch;
        }
    }
    int vStretch() {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().verticalStretch();
        } else {
            return stretch;
        }
    }

    QLayoutItem *item;
    int stretch;
    bool magic;
};

// oye ,private core is setupGeom
class QBoxLayoutPrivate : public QLayoutPrivate
{
public:
    QBoxLayoutPrivate() : hfwWidth(-1), dirty(true), spacing(-1) { }
    ~QBoxLayoutPrivate();

    void setDirty() {
        geomArray.clear();
        hfwWidth = -1;
        hfwHeight = -1;
        dirty = true;
    }

    QList<QBoxLayoutItem *> list;      // upper level use  
    QVector<QLayoutStruct> geomArray;  // lower level use it to engine to decide 
    int hfwWidth;
    int hfwHeight;
    int hfwMinHeight;
    QSize sizeHint;
    QSize minSize;
    QSize maxSize;
    int leftMargin, topMargin, rightMargin, bottomMargin;
    Qt::Orientations expanding;
    uint hasHfw : 1;
    uint dirty : 1;
    QBoxLayout::Direction dir;
    int spacing;

    inline void deleteAll() { while (!list.isEmpty()) delete list.takeFirst(); }

    void setupGeom();
    void calcHfw(int);

    void effectiveMargins(int *left, int *top, int *right, int *bottom) const;
    QLayoutItem* replaceAt(int index, QLayoutItem*) ;
};

QBoxLayoutPrivate::~QBoxLayoutPrivate()
{
}

static inline bool horz(QBoxLayout::Direction dir)
{
    return dir == QBoxLayout::RightToLeft || dir == QBoxLayout::LeftToRight;
}

/**
 * The purpose of this function is to make sure that widgets are not laid out outside its layout.
 * E.g. the layoutItemRect margins are only meant to take of the surrounding margins/spacings.
 * However, if the margin is 0, it can easily cover the area of a widget above it.
 */
void QBoxLayoutPrivate::effectiveMargins(int *left, int *top, int *right, int *bottom) const
{
    int l = leftMargin;
    int t = topMargin;
    int r = rightMargin;
    int b = bottomMargin;

    if (left)
        *left = l;
    if (top)
        *top = t;
    if (right)
        *right = r;
    if (bottom)
        *bottom = b;
}


/*
    Initializes the data structure needed by qGeomCalc and
    recalculates max/min and size hint.
*/
void QBoxLayoutPrivate::setupGeom()
{
    if (!dirty)
        return;

    //Q_Q(QBoxLayout);
	QBoxLayout* const q = q_func();
    int maxw = horz(dir) ? 0 : QLAYOUTSIZE_MAX;
    int maxh = horz(dir) ? QLAYOUTSIZE_MAX : 0;  // oye  for horz, heights is max???? why
    int minw = 0;
    int minh = 0;
    int hintw = 0;
    int hinth = 0;

    bool horexp = false;
    bool verexp = false;

    hasHfw = false;

    int n = list.count();  // at run-time ,  add_item,insert_item will populate the list
    geomArray.clear();
	
    QVector<QLayoutStruct> a(n);   // oye, calc each i in a,  at last set a to  geomArray

    QSizePolicy::ControlTypes controlTypes1;
    QSizePolicy::ControlTypes controlTypes2;
	
    int fixedSpacing = q->spacing();
    int previousNonEmptyIndex = -1;

    QStyle *style = 0;
    if (fixedSpacing < 0) {
        if (QWidget *parentWidget = q->parentWidget())
            style = parentWidget->style();
    }

	// oye,  cal and set each value in QLayoutStruct to the coresponding item
    for (int i = 0; i < n; i++) {
        QBoxLayoutItem *box = list.at(i); // upperlevel 
        QSize max = box->item->maximumSize();
        QSize min = box->item->minimumSize();
        QSize hint = box->item->sizeHint();
        Qt::Orientations exp = box->item->expandingDirections();
        bool empty = box->item->isEmpty();   //oye  item, spacer is empty, widget is true if is hide. layout is the 
        // composite of spacer and widget.
        int spacing = 0;

		// !empty, get val of spacing
		// set v to QLayoutStruct::spacing
        if (!empty) {
            if (fixedSpacing >= 0) {
                spacing = (previousNonEmptyIndex >= 0) ? fixedSpacing : 0;
            } else {
                controlTypes1 = controlTypes2;
                controlTypes2 = box->item->controlTypes();
                if (previousNonEmptyIndex >= 0) {
                    QSizePolicy::ControlTypes actual1 = controlTypes1;
                    QSizePolicy::ControlTypes actual2 = controlTypes2;
                    if (dir == QBoxLayout::RightToLeft || dir == QBoxLayout::BottomToTop)
                        qSwap(actual1, actual2);

                    if (style) {
                        spacing = style->combinedLayoutSpacing(actual1, actual2,
                                             horz(dir) ? Qt::Horizontal : Qt::Vertical,
                                             0, q->parentWidget());
                        if (spacing < 0)
                            spacing = 0;
                    }
                }
            }

            if (previousNonEmptyIndex >= 0)
                a[previousNonEmptyIndex].spacing = spacing;  // set v to item's spacing
            previousNonEmptyIndex = i;
        }

        bool ignore = empty && box->item->widget(); // ignore hidden widgets
        bool dummy = true;
        if (horz(dir)) {
            bool expand = (exp & Qt::Horizontal || box->stretch > 0);
            horexp = horexp || expand;
            maxw += spacing + max.width();
            minw += spacing + min.width();
            hintw += spacing + hint.width();
            if (!ignore)
                qMaxExpCalc(maxh, verexp, dummy,
                            max.height(), exp & Qt::Vertical, box->item->isEmpty());
            minh = qMax(minh, min.height());
            hinth = qMax(hinth, hint.height());

            a[i].sizeHint = hint.width();
            a[i].maximumSize = max.width();
            a[i].minimumSize = min.width();
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->hStretch();
        } else {
            bool expand = (exp & Qt::Vertical || box->stretch > 0);
            verexp = verexp || expand;
            maxh += spacing + max.height();
            minh += spacing + min.height();
            hinth += spacing + hint.height();
            if (!ignore)
                qMaxExpCalc(maxw, horexp, dummy,
                            max.width(), exp & Qt::Horizontal, box->item->isEmpty());
            minw = qMax(minw, min.width());
            hintw = qMax(hintw, hint.width());

            a[i].sizeHint = hint.height();
            a[i].maximumSize = max.height();
            a[i].minimumSize = min.height();
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->vStretch();
        }

        a[i].empty = empty;
        a[i].spacing = 0;   // might be initialized with a non-zero value in a later iteration
        hasHfw = hasHfw || box->item->hasHeightForWidth();
    }

    geomArray = a;

    expanding = (Qt::Orientations)
                       ((horexp ? Qt::Horizontal : 0)
                         | (verexp ? Qt::Vertical : 0));

    minSize = QSize(minw, minh);
    maxSize = QSize(maxw, maxh).expandedTo(minSize);
    sizeHint = QSize(hintw, hinth).expandedTo(minSize).boundedTo(maxSize);

    q->getContentsMargins(&leftMargin, &topMargin, &rightMargin, &bottomMargin);
    int left, top, right, bottom;
    effectiveMargins(&left, &top, &right, &bottom);
    QSize extra(left + right, top + bottom);

    minSize += extra;
    maxSize += extra;
    sizeHint += extra;

    dirty = false;
}

/*
  Calculates and stores the preferred height given the width \a w.
*/
void QBoxLayoutPrivate::calcHfw(int w)
{
    QVector<QLayoutStruct> &a = geomArray;
    int n = a.count();
    int h = 0;
    int mh = 0;

    Q_ASSERT(n == list.size());

    if (horz(dir)) {
        qGeomCalc(a, 0, n, 0, w);
        for (int i = 0; i < n; i++) {
            QBoxLayoutItem *box = list.at(i);
            h = qMax(h, box->hfw(a.at(i).size));
            mh = qMax(mh, box->mhfw(a.at(i).size));
        }
    } else {
        for (int i = 0; i < n; ++i) {
            QBoxLayoutItem *box = list.at(i);
            int spacing = a.at(i).spacing;
            h += box->hfw(w);
            mh += box->mhfw(w);
            h += spacing;
            mh += spacing;
        }
    }
    hfwWidth = w;
    hfwHeight = h;
    hfwMinHeight = mh;
}

QLayoutItem* QBoxLayoutPrivate::replaceAt(int index, QLayoutItem *item)
{
    Q_Q(QBoxLayout);
    if (!item)
        return 0;
    QBoxLayoutItem *b = list.value(index);
    if (!b)
        return 0;
    QLayoutItem *r = b->item;

    b->item = item;
    q->invalidate();
    return r;
}


QBoxLayout::QBoxLayout(Direction dir, QWidget *parent)
    : con(*new QBoxLayoutPrivate, 0, parent)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    d->dir = dir;
}




QBoxLayout::~QBoxLayout()
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    d->deleteAll(); // must do it before QObject deletes children, so can't be in ~QBoxLayoutPrivate
}


int QBoxLayout::spacing() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->spacing >=0) {
        return d->spacing;
    } else {
        return qSmartSpacing(this, d->dir == LeftToRight || d->dir == RightToLeft
                                           ? QStyle::PM_LayoutHorizontalSpacing
                                           : QStyle::PM_LayoutVerticalSpacing);
    }
}


void QBoxLayout::setSpacing(int spacing)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    d->spacing = spacing;
    invalidate();
}


QSize QBoxLayout::sizeHint() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->sizeHint;
}


QSize QBoxLayout::minimumSize() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->minSize;
}


QSize QBoxLayout::maximumSize() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();

    QSize s = d->maxSize.boundedTo(QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX));

    if (alignment() & Qt::AlignHorizontal_Mask)
        s.setWidth(QLAYOUTSIZE_MAX);
    if (alignment() & Qt::AlignVertical_Mask)
        s.setHeight(QLAYOUTSIZE_MAX);
    return s;
}


bool QBoxLayout::hasHeightForWidth() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->hasHfw;
}


int QBoxLayout::heightForWidth(int w) const
{
    QBoxLayoutPrivate* const d = d_func();
    if (!hasHeightForWidth())
        return -1;

    int left, top, right, bottom;
    d->effectiveMargins(&left, &top, &right, &bottom);

    w -= left + right;
    if (w != d->hfwWidth)
        const_cast<QBoxLayout*>(this)->d_func()->calcHfw(w);

    return d->hfwHeight + top + bottom;
}


int QBoxLayout::minimumHeightForWidth(int w) const
{
    QBoxLayoutPrivate* const d = d_func();
    (void) heightForWidth(w);
    int top, bottom;
    d->effectiveMargins(0, &top, 0, &bottom);
    return d->hasHfw ? (d->hfwMinHeight + top + bottom) : -1;
}


void QBoxLayout::invalidate()
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    d->setDirty();
    QLayout::invalidate();
}


int QBoxLayout::count() const
{
    QBoxLayoutPrivate* const d = d_func();
    return d->list.count();
}


QLayoutItem *QBoxLayout::itemAt(int index) const
{
    QBoxLayoutPrivate* const d = d_func();
    return index >= 0 && index < d->list.count() ? d->list.at(index)->item : 0;
}

/*!
    \reimp
*/
QLayoutItem *QBoxLayout::takeAt(int index)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (index < 0 || index >= d->list.count())
        return 0;
    QBoxLayoutItem *b = d->list.takeAt(index);
    QLayoutItem *item = b->item;
    b->item = 0;
    delete b;

    if (QLayout *l = item->layout()) {
        // sanity check in case the user passed something weird to QObject::setParent()
        if (l->parent() == this)
            l->setParent(0);
    }

    invalidate();
    return item;
}


Qt::Orientations QBoxLayout::expandingDirections() const
{
    QBoxLayoutPrivate* const d = d_func();
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->expanding;
}

// if Geo is rect_r,  how to arrange all the child_items
void QBoxLayout::setGeometry(const QRect &r)
{
    QBoxLayoutPrivate* const d = d_func();

	// dirty or geo_changed
    if (d->dirty || r != geometry()) {
		
        QRect oldRect = geometry();
        QLayout::setGeometry(r);
	
        if (d->dirty)
            d->setupGeom();

		//
        QRect cr = alignment() ? alignmentRect(r) : r;

        int left, top, right, bottom;
        d->effectiveMargins(&left, &top, &right, &bottom);

		// oye effective area without margin
        QRect s(cr.x() + left, 
				cr.y() + top,
                cr.width() - (left + right),
                cr.height() - (top + bottom));

		// oye get geoArray	
        QVector<QLayoutStruct> a = d->geomArray;
		
        int pos = horz(d->dir) ? s.x() : s.y();
        int space = horz(d->dir) ? s.width() : s.height();
        int n = a.count();
        if (d->hasHfw && !horz(d->dir)) {

			// ammend the width for each item
            for (int i = 0; i < n; i++) {
                QBoxLayoutItem *box = d->list.at(i);
                if (box->item->hasHeightForWidth()) {
                    int width = qBound(box->item->minimumSize().width(), s.width(), box->item->maximumSize().width());
                    a[i].sizeHint = a[i].minimumSize =
                                    box->item->heightForWidth(width);
                }
            }
        }

		// amend the direction
        Direction visualDir = d->dir;
        QWidget *parent = parentWidget();
        if (parent && parent->isRightToLeft()) {
            if (d->dir == LeftToRight)
                visualDir = RightToLeft;
            else if (d->dir == RightToLeft)
                visualDir = LeftToRight;
        }

		// oye get the exact position of each item in engine
        qGeomCalc(a, 0, n, pos, space);

        bool reverse = (horz(visualDir)
                        ? ((r.right() > oldRect.right()) != (visualDir == RightToLeft))
                        : r.bottom() > oldRect.bottom());

		// oye : for each item in box,  set its geometry				
        for (int j = 0; j < n; j++) 
		{
            int i = reverse ? n-j-1 : j;
            QBoxLayoutItem *box = d->list.at(i);

            switch (visualDir) 
			{
	            case LeftToRight:
	                box->item->setGeometry(QRect(a.at(i).pos, s.y(), a.at(i).size, s.height()));
	                break;
	            case RightToLeft:
	                box->item->setGeometry(QRect(s.left() + s.right() - a.at(i).pos - a.at(i).size + 1,
	                                             s.y(), a.at(i).size, s.height()));
	                break;
	            case TopToBottom:
	                box->item->setGeometry(QRect(s.x(), a.at(i).pos, s.width(), a.at(i).size));
	                break;
	            case BottomToTop:
	                box->item->setGeometry(QRect(s.x(),
	                                             s.top() + s.bottom() - a.at(i).pos - a.at(i).size + 1,
	                                             s.width(), a.at(i).size));
					break;
            }
			
        }
		
    }
}


void QBoxLayout::addItem(QLayoutItem *item)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    QBoxLayoutItem *it = new QBoxLayoutItem(item);
    d->list.append(it);
    invalidate();
}


void QBoxLayout::insertItem(int index, QLayoutItem *item)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (index < 0)                                // append
        index = d->list.count();

    QBoxLayoutItem *it = new QBoxLayoutItem(item);
    d->list.insert(index, it);
    invalidate();
}


void QBoxLayout::insertSpacing(int index, int size)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (index < 0)                                // append
        index = d->list.count();

    QLayoutItem *b;
    if (horz(d->dir))
        b = QLayoutPrivate::createSpacerItem(this, size, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, 0, size, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QBoxLayoutItem *it = new QBoxLayoutItem(b);
    it->magic = true;
    d->list.insert(index, it);
  
    invalidate();
}


void QBoxLayout::insertStretch(int index, int stretch)
{
    /*QBoxLayoutPrivate* const d = d_func();*/
	QBoxLayoutPrivate* const d = d_func();
    if (index < 0)                                // append
        index = d->list.count();

    QLayoutItem *b;
    if (horz(d->dir))
		// h: --bV++   v: bV++,  
        b = QLayoutPrivate::createSpacerItem(this, 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QBoxLayoutItem *it = new QBoxLayoutItem(b, stretch);
    it->magic = true;
    d->list.insert(index, it);
    invalidate();
}


void QBoxLayout::insertSpacerItem(int index, QSpacerItem *spacerItem)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (index < 0)                                // append
        index = d->list.count();

    QBoxLayoutItem *it = new QBoxLayoutItem(spacerItem);
    it->magic = true;
    d->list.insert(index, it);
    invalidate();
}


void QBoxLayout::insertLayout(int index, QLayout *layout, int stretch)
{
    /*QBoxLayoutPrivate* const d = d_func();*/
	QBoxLayoutPrivate* const d = d_func();
    if (!d->checkLayout(layout))
        return;
    if (!adoptLayout(layout))
        return;
    if (index < 0)                                // append
        index = d->list.count();
    QBoxLayoutItem *it = new QBoxLayoutItem(layout, stretch);
    d->list.insert(index, it);
    invalidate();
}


void QBoxLayout::insertWidget(int index, QWidget *widget, int stretch,
                              Qt::Alignment alignment)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (!d->checkWidget(widget))
         return;
    addChildWidget(widget);
    if (index < 0)                                // append
        index = d->list.count();
    QWidgetItem *b = QLayoutPrivate::createWidgetItem(this, widget);
    b->setAlignment(alignment);

    QBoxLayoutItem *it;
	
    it = new QBoxLayoutItem(b, stretch);  
    d->list.insert(index, it);
   
    invalidate();
}


void QBoxLayout::addSpacing(int size)
{
    insertSpacing(-1, size);
}


void QBoxLayout::addStretch(int stretch)
{
    insertStretch(-1, stretch);
}

/*!
    \since 4.4

    Adds \a spacerItem to the end of this box layout.

    \sa addSpacing(), addStretch()
*/
void QBoxLayout::addSpacerItem(QSpacerItem *spacerItem)
{
    insertSpacerItem(-1, spacerItem);
}


void QBoxLayout::addWidget(QWidget *widget, int stretch, Qt::Alignment alignment)
{
    insertWidget(-1, widget, stretch, alignment);
}


void QBoxLayout::addLayout(QLayout *layout, int stretch)
{
    insertLayout(-1, layout, stretch);
}


void QBoxLayout::addStrut(int size)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    QLayoutItem *b;
    if (horz(d->dir))
        b = QLayoutPrivate::createSpacerItem(this, 0, size, QSizePolicy::Fixed, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, size, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QBoxLayoutItem *it = new QBoxLayoutItem(b);
    it->magic = true;
    d->list.append(it);
    invalidate();
}


bool QBoxLayout::setStretchFactor(QWidget *widget, int stretch)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (!widget)
        return false;
    for (int i = 0; i < d->list.size(); ++i) {
        QBoxLayoutItem *box = d->list.at(i);
        if (box->item->widget() == widget) {
            box->stretch = stretch;
            invalidate();
            return true;
        }
    }
    return false;
}


bool QBoxLayout::setStretchFactor(QLayout *layout, int stretch)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    for (int i = 0; i < d->list.size(); ++i) {
        QBoxLayoutItem *box = d->list.at(i);
        if (box->item->layout() == layout) {
            if (box->stretch != stretch) {
                box->stretch = stretch;
                invalidate();
            }
            return true;
        }
    }
    return false;
}



void QBoxLayout::setStretch(int index, int stretch)
{
    /*QBoxLayoutPrivate* const d = d_func();*/QBoxLayoutPrivate* const d = d_func();
    if (index >= 0 && index < d->list.size()) {
        QBoxLayoutItem *box = d->list.at(index);
        if (box->stretch != stretch) {
            box->stretch = stretch;
            invalidate();
        }
    }
}



int QBoxLayout::stretch(int index) const
{
    //QBoxLayoutPrivate* const d = d_func();
	QBoxLayoutPrivate* const d = d_func();
    if (index >= 0 && index < d->list.size())
        return d->list.at(index)->stretch;
    return -1;
}


void QBoxLayout::setDirection(Direction direction)
{
    /*QBoxLayoutPrivate* const d = d_func();*/
	QBoxLayoutPrivate* const d = d_func();
	// avoid identical operation
    if (d->dir == direction)
        return;

	
    if (horz(d->dir) != horz(direction)) {


        for (int i = 0; i < d->list.size(); ++i) {
            QBoxLayoutItem *box = d->list.at(i);
            if (box->magic) {
                QSpacerItem *sp = box->item->spacerItem();
                if (sp) {
                    if (sp->expandingDirections() == Qt::Orientations(0) /*No Direction*/) {
                        //spacing or strut
                        QSize s = sp->sizeHint();
                        sp->changeSize(s.height(), s.width(),
                            horz(direction) ? QSizePolicy::Fixed:QSizePolicy::Minimum,
                            horz(direction) ? QSizePolicy::Minimum:QSizePolicy::Fixed);

                    } else {
                        //stretch
                        if (horz(direction))
                            sp->changeSize(0, 0, QSizePolicy::Expanding,
                                            QSizePolicy::Minimum);
                        else
                            sp->changeSize(0, 0, QSizePolicy::Minimum,
                                            QSizePolicy::Expanding);
                    }
                }
            }
        }
    }
    d->dir = direction;
    invalidate();
}


QBoxLayout::Direction QBoxLayout::direction() const
{
    QBoxLayoutPrivate* const d = d_func();
    return d->dir;
}


QHBoxLayout::QHBoxLayout(QWidget *parent)
    : QBoxLayout(LeftToRight, parent)
{
}

QHBoxLayout::QHBoxLayout()
    : QBoxLayout(LeftToRight)
{
}


QHBoxLayout::~QHBoxLayout()
{
}

QVBoxLayout::QVBoxLayout(QWidget *parent)
    : QBoxLayout(TopToBottom, parent)
{
}


QVBoxLayout::QVBoxLayout()
    : QBoxLayout(TopToBottom)
{
}


QVBoxLayout::~QVBoxLayout()
{
}


#include "moc_qboxlayout.cpp"

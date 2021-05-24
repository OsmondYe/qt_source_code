#ifndef QREGION_H
#define QREGION_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qrect.h>
#include <QtGui/qwindowdefs.h>

#ifndef QT_NO_DATASTREAM
#include <QtCore/qdatastream.h>
#endif

QT_BEGIN_NAMESPACE


template <class T> class QVector;
class QVariant;

struct QRegionPrivate;

class QBitmap;

// 在Painter事件处理中, 把画图空间限制在具体的区域中, 矩形和椭圆2类
class  QRegion
{
public:
    enum RegionType { Rectangle, Ellipse };

    QRegion();
    QRegion(int x, int y, int w, int h, RegionType t = Rectangle);
    QRegion(const QRect &r, RegionType t = Rectangle);
    QRegion(const QPolygon &pa, Qt::FillRule fillRule = Qt::OddEvenFill);
    QRegion(const QRegion &region);
    QRegion(QRegion &&other) Q_DECL_NOTHROW
        : d(other.d) { other.d = const_cast<QRegionData*>(&shared_empty); }
    QRegion(const QBitmap &bitmap);
    ~QRegion();
    QRegion &operator=(const QRegion &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QRegion &operator=(QRegion &&other) Q_DECL_NOEXCEPT
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QRegion &other) Q_DECL_NOEXCEPT { qSwap(d, other.d); }
    bool isEmpty() const;
    bool isNull() const;

    typedef const QRect *const_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    const_iterator begin()  const Q_DECL_NOTHROW;
    const_iterator cbegin() const Q_DECL_NOTHROW { return begin(); }
    const_iterator end()    const Q_DECL_NOTHROW;
    const_iterator cend()   const Q_DECL_NOTHROW { return end(); }
    const_reverse_iterator rbegin()  const Q_DECL_NOTHROW { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const Q_DECL_NOTHROW { return rbegin(); }
    const_reverse_iterator rend()    const Q_DECL_NOTHROW { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend()   const Q_DECL_NOTHROW { return rend(); }

    bool contains(const QPoint &p) const;
    bool contains(const QRect &r) const;

    void translate(int dx, int dy);
    inline void translate(const QPoint &p) { translate(p.x(), p.y()); }
    Q_REQUIRED_RESULT QRegion translated(int dx, int dy) const;
    Q_REQUIRED_RESULT inline QRegion translated(const QPoint &p) const { return translated(p.x(), p.y()); }

    Q_REQUIRED_RESULT QRegion united(const QRegion &r) const;
    Q_REQUIRED_RESULT QRegion united(const QRect &r) const;
    Q_REQUIRED_RESULT QRegion intersected(const QRegion &r) const;
    Q_REQUIRED_RESULT QRegion intersected(const QRect &r) const;
    Q_REQUIRED_RESULT QRegion subtracted(const QRegion &r) const;
    Q_REQUIRED_RESULT QRegion xored(const QRegion &r) const;

#if QT_DEPRECATED_SINCE(5, 0)
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion unite(const QRegion &r) const { return united(r); }
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion unite(const QRect &r) const { return united(r); }
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion intersect(const QRegion &r) const { return intersected(r); }
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion intersect(const QRect &r) const { return intersected(r); }
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion subtract(const QRegion &r) const { return subtracted(r); }
    Q_REQUIRED_RESULT inline QT_DEPRECATED QRegion eor(const QRegion &r) const { return xored(r); }
#endif

    bool intersects(const QRegion &r) const;
    bool intersects(const QRect &r) const;

    QRect boundingRect() const Q_DECL_NOTHROW;
    QVector<QRect> rects() const;
    void setRects(const QRect *rect, int num);
    int rectCount() const Q_DECL_NOTHROW;
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    // ### Qt 6: remove these, they're kept for MSVC compat
    const QRegion operator|(const QRegion &r) const;
    const QRegion operator+(const QRegion &r) const;
    const QRegion operator+(const QRect &r) const;
    const QRegion operator&(const QRegion &r) const;
    const QRegion operator&(const QRect &r) const;
    const QRegion operator-(const QRegion &r) const;
    const QRegion operator^(const QRegion &r) const;
#else
    QRegion operator|(const QRegion &r) const;
    QRegion operator+(const QRegion &r) const;
    QRegion operator+(const QRect &r) const;
    QRegion operator&(const QRegion &r) const;
    QRegion operator&(const QRect &r) const;
    QRegion operator-(const QRegion &r) const;
    QRegion operator^(const QRegion &r) const;
#endif // Q_COMPILER_MANGLES_RETURN_TYPE
    QRegion& operator|=(const QRegion &r);
    QRegion& operator+=(const QRegion &r);
    QRegion& operator+=(const QRect &r);
    QRegion& operator&=(const QRegion &r);
    QRegion& operator&=(const QRect &r);
    QRegion& operator-=(const QRegion &r);
    QRegion& operator^=(const QRegion &r);

    bool operator==(const QRegion &r) const;
    inline bool operator!=(const QRegion &r) const { return !(operator==(r)); }
    operator QVariant() const;

#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QRegion &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QRegion &);
#endif
private:
    QRegion copy() const;   // helper of detach.
    void detach();
Q_GUI_EXPORT
    friend bool qt_region_strictContains(const QRegion &region,
                                         const QRect &rect);
    friend struct QRegionPrivate;

#ifndef QT_NO_DATASTREAM
    void exec(const QByteArray &ba, int ver = 0, QDataStream::ByteOrder byteOrder = QDataStream::BigEndian);
#endif
    struct QRegionData {
        QtPrivate::RefCount ref;
        QRegionPrivate *qt_rgn;
    };
    struct QRegionData *d;
    static const struct QRegionData shared_empty;
    static void cleanUp(QRegionData *x);
};

QT_END_NAMESPACE

#endif // QREGION_H

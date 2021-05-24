#ifndef QICON_H
#define QICON_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qsize.h>
#include <QtCore/qlist.h>
#include <QtGui/qpixmap.h>

class QIconPrivate;
class QIconEngine;

class  QIcon
{
public:
    enum Mode { Normal, Disabled, Active, Selected };
    enum State { On, Off };

    QIcon() Q_DECL_NOEXCEPT;
    QIcon(const QPixmap &pixmap);
    QIcon(const QIcon &other);
    explicit QIcon(const QString &fileName); // file or resource name
    explicit QIcon(QIconEngine *engine);
    ~QIcon();
    QIcon &operator=(const QIcon &other);
    inline void swap(QIcon &other) Q_DECL_NOEXCEPT
    { qSwap(d, other.d); }

    operator QVariant() const;

    QPixmap pixmap(const QSize &size, Mode mode = Normal, State state = Off) const;
	QPixmap pixmap(QWindow *window, const QSize &size, Mode mode = Normal, State state = Off) const;
	
    inline QPixmap pixmap(int w, int h, Mode mode = Normal, State state = Off) const
        { return pixmap(QSize(w, h), mode, state); }
    inline QPixmap pixmap(int extent, Mode mode = Normal, State state = Off) const
        { return pixmap(QSize(extent, extent), mode, state); }
	

    QSize actualSize(const QSize &size, Mode mode = Normal, State state = Off) const;
    QSize actualSize(QWindow *window, const QSize &size, Mode mode = Normal, State state = Off) const;

    QString name() const;

    void paint(QPainter *painter, const QRect &rect, Qt::Alignment alignment = Qt::AlignCenter, Mode mode = Normal, State state = Off) const;
    inline void paint(QPainter *painter, int x, int y, int w, int h, Qt::Alignment alignment = Qt::AlignCenter, Mode mode = Normal, State state = Off) const
        { paint(painter, QRect(x, y, w, h), alignment, mode, state); }

    bool isNull() const;
    bool isDetached() const;
    void detach();

    qint64 cacheKey() const;

    void addPixmap(const QPixmap &pixmap, Mode mode = Normal, State state = Off);
    void addFile(const QString &fileName, const QSize &size = QSize(), Mode mode = Normal, State state = Off);

    QList<QSize> availableSizes(Mode mode = Normal, State state = Off) const;

    void setIsMask(bool isMask);
    bool isMask() const;

    static QIcon fromTheme(const QString &name);
    static QIcon fromTheme(const QString &name, const QIcon &fallback);
    static bool hasThemeIcon(const QString &name);

    static QStringList themeSearchPaths();
    static void setThemeSearchPaths(const QStringList &searchpath);

    static QString themeName();
    static void setThemeName(const QString &path);

    Q_DUMMY_COMPARISON_OPERATOR(QIcon)

private:
    QIconPrivate *d;
public:
    typedef QIconPrivate * DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

//Q_DECLARE_SHARED(QIcon)


 QString qt_findAtNxFile(const QString &baseFileName, qreal targetDevicePixelRatio,
                                     qreal *sourceDevicePixelRatio = Q_NULLPTR);


#endif // QICON_H

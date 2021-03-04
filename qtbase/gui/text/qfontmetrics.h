#ifndef QFONTMETRICS_H
#define QFONTMETRICS_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qfont.h>
#include <QtCore/qsharedpointer.h>
#ifndef QT_INCLUDE_COMPAT
#include <QtCore/qrect.h>
#endif

QT_BEGIN_NAMESPACE



class QTextCodec;
class QRect;


class  QFontMetrics
{
public:
    explicit QFontMetrics(const QFont &);
    QFontMetrics(const QFont &, QPaintDevice *pd);
    ~QFontMetrics(){}


    int ascent() const;
    int capHeight() const;
    int descent() const;
    int height() const;
    int leading() const;
    int lineSpacing() const;
    int minLeftBearing() const;
    int minRightBearing() const;
    int maxWidth() const;

    int xHeight() const;
    int averageCharWidth() const;

    bool inFont(QChar) const;
    bool inFontUcs4(uint ucs4) const;

    int leftBearing(QChar) const;
    int rightBearing(QChar) const;
    int width(const QString &, int len = -1) const;
    int width(const QString &, int len, int flags) const;

    int width(QChar) const;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QT_DEPRECATED int charWidth(const QString &str, int pos) const;
#endif

	// oye Text's boundingRect
    QRect boundingRect(QChar) const;
    QRect boundingRect(const QString &text) const;
    QRect boundingRect(const QRect &r, int flags, const QString &text, 
				int tabstops = 0, int *tabarray = Q_NULLPTR) const;	
    inline QRect boundingRect(int x, int y, int w, int h, int flags, const QString &text,
                              int tabstops = 0, int *tabarray = Q_NULLPTR) const
        { return boundingRect(QRect(x, y, w, h), flags, text, tabstops, tabarray); }

							  
    QSize size(int flags, const QString& str, int tabstops = 0, int *tabarray = Q_NULLPTR) const;

    QRect tightBoundingRect(const QString &text) const;

    QString elidedText(const QString &text, Qt::TextElideMode mode, int width, int flags = 0) const;

    int underlinePos() const;
    int overlinePos() const;
    int strikeOutPos() const;
    int lineWidth() const;


private:
    friend class QFontMetricsF;
    friend class QStackTextEngine;

    QExplicitlySharedDataPointer<QFontPrivate> d;
};

//Q_DECLARE_SHARED(QFontMetrics)

class  QFontMetricsF
{
public:
    explicit QFontMetricsF(const QFont &);
    QFontMetricsF(const QFont &, QPaintDevice *pd);
    QFontMetricsF(const QFontMetrics &);
    QFontMetricsF(const QFontMetricsF &);
    ~QFontMetricsF();

    QFontMetricsF &operator=(const QFontMetricsF &);
    QFontMetricsF &operator=(const QFontMetrics &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QFontMetricsF &operator=(QFontMetricsF &&other)
    { qSwap(d, other.d); return *this; }
#endif

    void swap(QFontMetricsF &other) { qSwap(d, other.d); }

    qreal ascent() const;
    qreal capHeight() const;
    qreal descent() const;
    qreal height() const;
    qreal leading() const;
    qreal lineSpacing() const;
    qreal minLeftBearing() const;
    qreal minRightBearing() const;
    qreal maxWidth() const;

    qreal xHeight() const;
    qreal averageCharWidth() const;

    bool inFont(QChar) const;
    bool inFontUcs4(uint ucs4) const;

    qreal leftBearing(QChar) const;
    qreal rightBearing(QChar) const;
    qreal width(const QString &string) const;

    qreal width(QChar) const;

    QRectF boundingRect(const QString &string) const;
    QRectF boundingRect(QChar) const;
    QRectF boundingRect(const QRectF &r, int flags, const QString& string, int tabstops = 0, int *tabarray = Q_NULLPTR) const;
    QSizeF size(int flags, const QString& str, int tabstops = 0, int *tabarray = Q_NULLPTR) const;

    QRectF tightBoundingRect(const QString &text) const;

    QString elidedText(const QString &text, Qt::TextElideMode mode, qreal width, int flags = 0) const;

    qreal underlinePos() const;
    qreal overlinePos() const;
    qreal strikeOutPos() const;
    qreal lineWidth() const;

    bool operator==(const QFontMetricsF &other) const;
    inline bool operator !=(const QFontMetricsF &other) const { return !operator==(other); }

private:
    QExplicitlySharedDataPointer<QFontPrivate> d;
};

//Q_DECLARE_SHARED(QFontMetricsF)

QT_END_NAMESPACE

#endif // QFONTMETRICS_H

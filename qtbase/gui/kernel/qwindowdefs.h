#ifndef QWINDOWDEFS_H
#define QWINDOWDEFS_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE


// Class forward definitions

class QPaintDevice;
class QWidget;
class QWindow;
class QDialog;
class QColor;
class QPalette;
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPolygon;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QFontInfo;
class QPen;
class QBrush;
class QMatrix;
class QPixmap;
class QBitmap;
class QMovie;
class QImage;
class QPicture;
class QTimer;
class QTime;
class QClipboard;
class QString;
class QByteArray;
class QApplication;

template<typename T> class QList;
typedef QList<QWidget *> QWidgetList;
typedef QList<QWindow *> QWindowList;

QT_END_NAMESPACE

// Window system dependent definitions


#include <QtGui/qwindowdefs_win.h>

QT_BEGIN_NAMESPACE

template<class K, class V> class QHash;
typedef QHash<WId, QWidget *> QWidgetMapper;

template<class V> class QSet;
typedef QSet<QWidget *> QWidgetSet;

QT_END_NAMESPACE


// Global platform-independent types and functions

#endif // QWINDOWDEFS_H
